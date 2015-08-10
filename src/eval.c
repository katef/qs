#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "var.h"
#include "dup.h"
#include "data.h"
#include "code.h"
#include "args.h"
#include "eval.h"
#include "proc.h"
#include "pair.h"
#include "frame.h"
#include "status.h"
#include "builtin.h"
#include "signal.h"
#include "task.h"

/* push .s=NULL to data */
static int
eval_null(struct data **data, const struct pos *pos)
{
	assert(data != NULL);
	assert(pos != NULL);

	if (!data_push(data, pos, 0, NULL)) {
		return -1;
	}

	return 0;
}

/* push .s="xyz" to data */
static int
eval_data(struct data **data, const struct pos *pos, const char *s)
{
	assert(data != NULL);
	assert(pos != NULL);
	assert(s != NULL);

	if (!data_push(data, pos, strlen(s), s)) {
		return -1;
	}

	return 0;
}

static int
eval_pipe(struct task **tasks, struct task *task, struct frame *frame, const struct pos *pos, struct code **code)
{
	struct data *lhs, *rhs;
	const struct frame *f;
	const struct pair *a;

	assert(tasks != NULL);
	assert(frame != NULL);
	assert(pos != NULL);
	assert(code != NULL);

	lhs = NULL;
	rhs = NULL;

	if (!data_push(&lhs, pos, 0, NULL)) { goto error; }
	if (!data_push(&rhs, pos, 0, NULL)) { goto error; }

	for (f = frame; f != NULL; f = f->parent) {
		for (a = f->asc; a != NULL; a = a->next) {
			int fd[2];

			if (-1 == pipe(fd)) {
				perror("pipe");
				goto error;
			}
/* XXX: must close pipe in parent process. but when? maybe keep a list, and #close
this should probably be the same as for #tick
*/

			if (debug & DEBUG_FD) {
				fprintf(stderr, "pipe [%d,%d] for asc [%d|%d]\n",
					fd[0], fd[1], a->m, a->n);
			}

/* XXX: for #dup -1 to close here, who does this?
only when we #run. for #tick we're in the same process and do not want to close() */

			/* lhs: dup a->m to write end */
			{
				if (!data_int(&lhs, pos, fd[1])) { goto error; }
				if (!data_int(&lhs, pos, a->m )) { goto error; }

				/* close the opposing side */
				if (!data_push(&lhs, pos, 0, NULL)) { goto error; }
				if (!data_int (&lhs, pos,   fd[0])) { goto error; }
			}

			/* rhs: dup a->n from read end */
			{
				if (!data_int(&rhs, pos, fd[0])) { goto error; }
				if (!data_int(&rhs, pos, a->n )) { goto error; }

				/* close the opposing side */
				if (!data_push(&rhs, pos, 0, NULL)) { goto error; }
				if (!data_int (&rhs, pos,   fd[1])) { goto error; }
			}
		}
	}

	/*
	 * Although #pipe is used for pipes in general, it is implemented taking
	 * special care for tick's pipe. Specifically:
	 *
	 * The newly-created lhs task is prioritised to the head of the tasks list,
	 * which for the tick pipe must be evaulated before #tick (in the rhs)
	 * is reached, otherwise #tick would block waiting to read from programs
	 * which have not yet been #run.
	 *
	 * The #tick instruction is executed from the same task as #pipe, so that
	 * it shares the data stack used by previous (and subsequent) instructions.
	 * If this were in its own task, its data would be effectively isolated.
	 */

	/* lhs */
	{
		struct task *new;

		new = task_add(tasks, frame, *code);
		if (new == NULL) {
			goto error;
		}

		*code = NULL;

		new->data = lhs;
	}

	/* rhs is what remains in the current task after this #pipe */
	{
		struct data **tail;

		/* TODO: centralise as data_wind() */
		for (tail = &rhs; *tail != NULL; tail = &(*tail)->next)
			;

		*tail = task->data;
		task->data = rhs;
	}

	return 0;

error:

	/* TODO: close pipe fds! maybe pair_close()? */
	data_free(lhs);
	data_free(rhs);

	return -1;
}

static int
close_lhs(const struct pair *a, struct pair *d) {
	assert(a != NULL);
	assert(d != NULL);
	assert(d->m != -1);

	if (d->n != a->m) {
		return 0;
	}

	if (debug & DEBUG_FD) {
		fprintf(stderr, "close(%d)\n", d->m);
	}

	if (-1 == close(d->m)) {
		return -1;
	}

	return 0;
}

static int
close_rhs(const struct pair *a, struct pair *d) {
	assert(a != NULL);
	assert(d != NULL);
	assert(d->m != -1);

	if (d->n != a->n) {
		return 0;
	}

	if (debug & DEBUG_FD) {
		fprintf(stderr, "close(%d)\n", d->m);
	}

	if (-1 == close(d->m)) {
		return -1;
	}

	return 0;
}

static int
eval_close(struct frame *frame,
	int (*cs)(const struct pair *, struct pair *))
{
	const struct frame *f;
	const struct pair *a;
	struct pair *d;

	assert(frame != NULL);
	assert(cs != NULL);

	/*
	 * The list of associated fds is collected from the parent frame upwards,
	 * the same as for #pipe creating pipes for each of those pairs.
	 *
	 * The set of open fds for dup2() calls for #run to dup_apply() are
	 * stored (by #dup) in this frame only. So #clhs/#crhs must be in
	 * the same frame as #dup populated the .dup list.
	 */

	for (f = frame->parent; f != NULL; f = f->parent) {
		for (a = f->asc; a != NULL; a = a->next) {
			for (d = frame->dup; d != NULL; d = d->next) {
				if (d->m == -1) {
					continue;
				}

				if (-1 == cs(a, d)) {
					perror("close");
				}
			}
		}
	}

	return 0;
}

/* pop $?, push !$? to *data */
static int
eval_not(void)
{
	status_exit(!status.r);

	return 0;
}

/* pop variable name; wind from variable to *data and next */
static int
eval_call(struct code **next, struct data **data,
	struct frame *frame)
{
	const struct var *v;
	struct data *a;

	assert(next != NULL);
	assert(data != NULL);
	assert(frame != NULL);

	if (*data == NULL) {
		errno = EINVAL;
		return -1;
	}

	a = data_pop(data);

	v = frame_get(frame, strlen(a->s), a->s);
	if (v == NULL) {
		fprintf(stderr, "no such variable: $%s\n", a->s);
		errno = 0;
		return -1;
	}

	/* XXX: share guts with eval_anon */
	if (debug & DEBUG_STACK) {
		fprintf(stderr, "wind code from $%s: ", a->s);
		code_dump(stderr, v->code);
	}

	if (-1 == code_wind(next, v->code)) {
		return -1;
	}

	return 0;
}

/* eat whole data stack upto .s=NULL, make argv and execute */
static int
eval_run(struct code **next, struct data **data,
	struct frame *frame, const struct pos *pos, struct task *task)
{
	struct data *p, *pnext;
	struct var *v;
	pid_t pid;
	int argc;
	char **args;

	assert(next != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(pos != NULL);
	assert(task != NULL);
	assert(task->pid == -1);

	if (debug & DEBUG_STACK) {
		for (p = *data; p->s != NULL; p = p->next) {
			fprintf(stderr, "data <- %s\n", p->s ? p->s : "NULL");
		}
	}

	/* TODO: run pre/post-command hook here (on *data) */

	argc = count_args(*data);
	if (argc == 0) {
		goto done;
	}

	args = make_args(*data, argc + 1);
	if (args == NULL) {
		return -1;
	}

/* TODO: don't allow dup lists for variables */

	/* variable */
	v = frame_get(frame, strlen(args[0]), args[0]);
	if (v != NULL) {
		/* XXX: share guts with eval_anon */
		if (debug & DEBUG_STACK) {
			fprintf(stderr, "wind code from $%s: ", args[0]);
			code_dump(stderr, v->code);
		}

		if (-1 == code_wind(next, v->code)) {
			return -1;
		}

		if (-1 == set_args(pos, frame, args + 1)) {
			free(args);
			return -1;
		}

		goto done;
	}

/* TODO: don't allow dup lists for builtins */

	/* builtin */
	switch (builtin(task, frame, argc, args)) {
	case -1: goto error;
	case  0: goto done;
	case  1: break;
	}

	/* program */
	pid = proc_rfork(0);
	switch (pid) {
	case -1:
		goto error;

	case 0:
		assert(argc >= 1);
		assert(args != NULL);

		if (debug & DEBUG_FD) {
			dup_dump(stderr, frame);
		}

		if (-1 == dup_apply(frame)) {
			goto fail;
		}

		(void) proc_exec(args[0], args);
		perror(args[0]); /* XXX: may not be visible if stderr is redirected */
		exit(errno);

	default:
		break;
	}

	/*
	 * Cooperative yield to the next task; this task won't be called again until
	 * our child PID has raised SIGCHLD.
	 */
	task->pid = pid;

done:

	/* TODO: maybe have make_args output p, cut off the arg list, and data_free() it */
	/* TODO: maybe centralise this as data_cut() */
	for (p = *data; p->s != NULL; p = pnext) {
		assert(p != NULL);

		pnext = p->next;
		free(p);
	}

	*data = p->next;
	free(p);

	return 0;

error:

	perror(args[0]);

	free(args);

	return -1;

fail:

	/*
	 * Failures which would leave the child unable to execute as requested.
	 * We abort rather than continuing to execute in a different manner.
	 */

	abort();
}

/* read from the pipe and push to *data */
/* note will need to be able to re-enter here after EINTR */
static int
eval_tick(struct code **next, struct data **data,
	struct frame *frame, const struct pos *pos, struct tick_state *ts)
{
	int in;
	int r;

	assert(next != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(pos != NULL);
	assert(ts != NULL);

	/*
	 * #tick reads from a pipe (set up by the parser), which would be
	 * dup2()'d to stdin if this were a #run child. Here we find the pipe's
	 * read end by searching for its dup2 newfd.
	 */
/* XXX: i would prefer that this were done outside of #tick, and just the fd passed in */
	in = dup_find(frame, STDIN_FILENO);
	if (in == -1) {
		errno = EBADF;
		return -1;
	}

	/*
	 * read some data from the tick pipe; loop to keep reading until SIGCHLD.
	 * Then the evaluator will re-enter #tick to read EOF.
	 */
	do {
		r = ss_readfd(in, &ts->s, &ts->l);
	} while (r > 0);

	if (r == -1 && errno == EINTR) {
		goto yield;
	}

	if (r == -1) {
		return -1;
	}

	if (debug & DEBUG_VAR) {
		fprintf(stderr, "` read: %.*s\n", (int) ts->l, ts->s);
	}

	/* TODO: tokenise by $^ here, e.g. { echo `date | wc } */
	/* TODO: can tokenisation be shared with the lexer's guts exposed? */
	/* TODO: the lexer should use $^ */
	/* TODO: \0 should implicitly always be in the $^ set */
	/* TODO: after reading, always split by '\0'. dealing with $^ is just s//ing more '\0' in situ */
	/* TODO: push each token as a separate item to the data stack */

	if (!data_push(data, pos, ts->l, ts->s)) {
		goto error;
	}

	free(ts->s);

ts->s = NULL; /* XXX: doesn't belong here */

	return 0;

yield:

	/*
	 * This node was popped, but reading was interrupted by SIGCHILD;
	 * here we yield to run that child's task, but first push a replacement
	 * #tick for re-entry to continue reading where we left off.
	 * This means the evalulation loop does not need to have special handling
	 * for #tick being re-entrant.
	 *
	 * However the evaulation loop does need to know that read() was
	 * interrupted by SIGCHILD (and only SIGCHLD) here, so that it can
	 * wait() to reap the exited child. So we return -1 with EINTR here.
	 */

	if (!code_push(next, pos, CODE_TICK)) {
		goto error;
	}

	errno = EINTR;
	return -1;

error:

	free(ts->s);

ts->s = NULL; /* XXX: doesn't belong here */

	return -1;
}

/* TODO: explain what happens here: status.r is the predicate, u.code is a block to call */
static int
eval_if(struct code **next,
	struct code **code)
{
	assert(next != NULL);
	assert(code != NULL);

	/*
	 * When the status condition is true, we simply ignore the u.code pointer.
	 * Otherwise, the current code is pushed to the return stack, and control
	 * transfered to the code passed.
	 */

	if (status.r == EXIT_SUCCESS) {
		/* XXX: share guts with eval_anon */
		if (debug & DEBUG_STACK) {
			fprintf(stderr, "wind code from #if: ");
			code_dump(stderr, *next);
		}

		/* TODO: would prefer to transplant the code as-is,
		 * rather than copying and then code_free */
		if (!code_wind(next, *code)) {
			return -1;
		}

		code_free(*code);

		*code = NULL;
	}

	return 0;
}

/* TODO: explain what happens here */
static int
eval_set(struct data **data,
	struct frame *frame, struct code **code)
{
	struct data *a;

	assert(data != NULL);
	assert(frame != NULL);
	assert(code != NULL);

	if (*data == NULL) {
		errno = EINVAL;
		return -1;
	}

	a = data_pop(data);

	if (0 == strncmp(a->s, "sig", 3)) {
		if (-1 == sig_register(a->s)) {
			return -1;
		}
	}

	if (!frame_set(frame, strlen(a->s), a->s, *code)) {
		errno = EINVAL;
		return -1;
	}

	*code = NULL;

	free(a);

	return 0;
}

static int
eval_frame(struct frame **frame,
	struct frame *(*op)(struct frame **head))
{
	struct frame *q;

	assert(frame != NULL && *frame != NULL);
	assert(op != NULL);

	q = op(frame);
	if (q == NULL) {
		return -1;
	}

	if (op == frame_pop) {
		frame_free(q);
	}

	return 0;
}

/* TODO: explain what happens here */
static int
op_join(struct data **data, struct frame *frame, struct data *a, struct data *b)
{
	(void) data;
	(void) frame;
	(void) a;
	(void) b;

	/* TODO */
	errno = ENOSYS;
	return -1;
}

static int
eval_binop(struct data **data,
	struct frame *frame,
	int (*op)(struct data **, struct frame *, struct data *a, struct data *b))
{
	struct data *a, *b;

	assert(data != NULL);
	assert(frame != NULL);
	assert(op != NULL);

	if (*data == NULL || (*data)->next == NULL) {
		errno = EINVAL; /* TODO: arity error */
		return -1;
	}

	b = data_pop(data);
	a = data_pop(data);

	if (-1 == op(data, frame, a, b)) {
		return -1;
	}

	free(a);
	free(b);

	return 0;
}

static int
eval_pair(struct data **data, struct pair **pair)
{
	assert(data != NULL);
	assert(pair != NULL);

	/*
	 * Pairs are ordered on the stack such that n is popped first, m second.
	 * The list of pairs is NULL-terminated. m may be NULL (indicating for
	 * the dup list to close n), but n is never NULL.
	 */

	while (*data != NULL && (*data)->s != NULL) {
		struct data *a, *b;
		int m, n;

		if ((*data)->next == NULL) {
			errno = EINVAL; /* TODO: arity error */
			return -1;
		}

		b = data_pop(data);
		a = data_pop(data);

		if (-1 == pair_fd(b->s, &n)) { free(b); return -1; }
		if (-1 == pair_fd(a->s, &m)) { free(a); return -1; } /* XXX: also free b! */

		free(b);
		free(a);

		if (debug & DEBUG_EXEC) {
			fprintf(stderr, "pair [%d, %d]\n", m, n);
		}

		if (!pair_push(pair, m, n)) {
			return -1;
		}
	}

	if (*data == NULL) {
		errno = EINVAL; /* TODO: arity error */
		return -1;
	}

	{
		struct data *q;

		q = data_pop(data);
		assert(q->s == NULL);
		free(q);
	}

	return 0;
}

static int
eval_ctck(struct frame *frame)
{
	int out;

	assert(frame != NULL);

	/*
	 * Find the write end of the tick pipe, to close it. We know this to be
	 * present because the lhs has the write end in its dup list, put there
	 * by #dup inside the pipe side so that #run can dup2() it to stdout.
	 *
	 * This is inside a frame created for the pipe side, so a user's duping
	 * cannot stop that from being present.
	 *
	 * There's no need to walk frames to the topmost frame, because
	 * #dup inside the pipe side populates this frame's dup list only.
	 * So dup_find() here is overkill, but harmless.
	 */
	out = dup_find(frame, STDOUT_FILENO); /* because #tick reads from [1|0] */
	if (out == -1) {
		errno = EINVAL;
		return -1;
	}

	if (debug & DEBUG_FD) {
		fprintf(stderr, "tick pipe: close(%d)\n", out);
	}

	if (-1 == close(out)) {
		perror("close");
	}

	/* XXX: but #crhs will close() it after this */
	/* TODO: remove entry from dup list. or hackishly open a dummy */
	/* XXX: hack */
	{
		int fd[2];
		(void) pipe(fd); /* just to make an fd without assuming anything on the filesystem */
		(void) dup2(fd[1], out);
		(void) close(fd[0]);
	}

	return 0;
}

static int
eval_task(struct task **tasks, struct task *task)
{
	struct code *node;
	int r;

	assert(task != NULL);

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "code: "); code_dump(stderr, task->code);
		fprintf(stderr, "data: "); data_dump(stderr, task->data);
	}

	/*
	 * The code list may be NULL if this task was waiting for a child, but has
	 * nothing to execute after that. Its data stack will already have been
	 * checked to be NULL before sleeping, on the previous call to task_eval.
	 */
	if (task->code == NULL) {
		assert(task->data == NULL);
		assert(task->pid == -1);
		return 0;
	}

	node = code_pop(&task->code);

	assert(node != NULL);

	switch (node->type) {
	case CODE_NULL: r = eval_null(&task->data, &node->pos);                                      break;
	case CODE_DATA: r = eval_data(&task->data, &node->pos, node->u.s);                           break;
	case CODE_PIPE: r = eval_pipe(tasks, task, task->frame, &node->pos, &node->u.code);          break;
	case CODE_NOT:  r = eval_not ();                                                             break;
	case CODE_IF:   r = eval_if  (&task->code, &node->u.code);                                   break;
	case CODE_RUN:  r = eval_run (&task->code, &task->data, task->frame, &node->pos, task);      break;
	case CODE_CALL: r = eval_call(&task->code, &task->data, task->frame);                        break;
	case CODE_TICK: r = eval_tick(&task->code, &task->data, task->frame, &node->pos, &task->ts); break;
	case CODE_SET:  r = eval_set (&task->data, task->frame, &node->u.code);                      break;
	case CODE_PUSH: r = eval_frame(&task->frame, frame_push);                                    break;
	case CODE_POP:  r = eval_frame(&task->frame, frame_pop);                                     break;
	case CODE_JOIN: r = eval_binop(&task->data, task->frame, op_join);                           break;
	case CODE_DUP:  r = eval_pair(&task->data, &task->frame->dup);                               break;
	case CODE_ASC:  r = eval_pair(&task->data, &task->frame->asc);                               break;
	case CODE_CLHS: r = eval_close(task->frame, close_lhs);                                      break;
	case CODE_CRHS: r = eval_close(task->frame, close_rhs);                                      break;
	case CODE_CTCK: r = eval_ctck (task->frame);                                                 break;

	default:
		code_free(node);
		errno = EINVAL;
		return -1;
	}

	code_free(node);

	if (r == -1) {
		return -1;
	}

	if (task->code == NULL) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "eval out: ");
			data_dump(stderr, task->data);
		}

		if (task->data != NULL) {
			errno = EINVAL;
			return -1;
		}
	}

	if (task->pid != -1) {
		return 0;
	}

	return 0;
}

int
eval(struct task **tasks)
{
	struct task *t;
	int r;

	assert(tasks != NULL);

	/*
	 * Terminology: What blocking (and sleeping) mean
	 *
	 * A signal may be blocked by SIG_BLOCK, or by sigaction.sa_mask.
	 * When blocked, subsequent arrivals of the same signal number will be
	 * consolidated (effectively a queue of one element per signal; a bitmap).
	 * One instance will then be delivered when unblocked.
	 *
	 * A syscall will not return for an indefinite period.
	 * For waitpid() without the WNOHANG flag. I call this a blocking wait.
	 *
	 * A task has a child process, and its code cannot continue
	 * evaulation until that child has been waited on. I call this sleeping.
	 * A sleeping task is indicated by task.pid set to the child's PID.
	 */

	/*
	 * There are two reasons a task will cooperatively yield;
	 * either because #tick's read(2) was interrupted by SIGCHLD
	 * or because #run was evaulated, and now the task is sleeping.
	 *
	 * #tick may be re-entered to continue reading where it left off.
	 * However, a task waiting for a child will not run again
	 * until its child raises SIGCHLD.
	 *
	 * When a child is reaped (either by a blocking wait, or after SIGCHLD
	 * causing EINTR for some other running task), the associated child's
	 * task is prioritised to the head of the tasks list.
	 *
	 * For #tick interrupted when reading from its pipe, this ensures that
	 * the next tasks re-entered are to (perhaps) spawn more children
	 * to write, rather than re-entering the #tick immediately which would
	 * block forever on read(2), with no child at the write end.
	 */

	t = *tasks;

	for (;;) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "== task %p ==\n", (void *) t);
		}

		r = eval_task(tasks, t);

		if (r == -1 && errno != EINTR) {
			goto error;
		}

		if (r == -1 && errno == EINTR) {
			int n;

			/*
			 * One situation to note here: SIGCHILD is blocked (per SIG_BLOCK)
			 * while waiting (and subsequently waiting again with WNOHANG
			 * until all children are reaped), SIGCHLD will be delivered on
			 * the next SIG_UNBLOCK. On handling that, we'd find there is no
			 * child to reap, having reaped them already.
			 *
			 * So this has to be a WNOHANGing wait otherwise we'd block here
			 * forever. Alternately sigsuspend(2) could be used, but I don't
			 * want to depend on POSIX.2001 additions. So instead I'm happy
			 * to ignore "stray child" signals for this situation.
			 */
			n = task_wait(tasks, -1, WNOHANG);
			if (n == -1) {
				goto error;
			}

			/*
			 * task_wait() returns the number of newly prioritised tasks.
			 * If this is zero, then there are no more children to #run;
			 * We can close our copy of the write end of the #tick pipe,
			 * since we don't need to hold open the pipe any more.
TODO: stale comment
			 *
			 * We need waitpid() to prioritise the #run task, which only
			 * happens on SIGCHLD. When the last child for that task signals,
			 * #ctck in that task will close our end of the tick pipe.
			 *
			 * So here we expect #ctck to be the next instruction for the
			 * (already highest priority) lhs task(s), or to have already
			 * evaulated #ctck when reaching here after #tick has finished.
			 *
			 * Subsequent read() in #tick will then give EOF, causing the
			 * #tick task to no longer re-enter the same #tick instruction.
			 */
			if (n == 0) {
				if (debug & DEBUG_EVAL) {
					fprintf(stderr, "eval: no tasks prioritised\n");
				}
			}
		}

		if (t->code == NULL && t->pid == -1) {
			task_remove(tasks, t);
		}

		if (*tasks == NULL) {
			break;
		}

		t = task_next(*tasks);

		/*
		 * When task_next() returns NULL, all tasks are sleeping on children.
		 * They are waited for using a blocking wait here, because there's
		 * nothing else to do.
		 */
		if (t == NULL) {
			if (-1 == task_wait(tasks, -1, 0)) {
				goto error;
			}

			t = *tasks;
		}

		/*
		 * At this point the task list is ordered to re-enter tasks which were
		 * waiting for SIGCHLD in preference to tasks which were not waiting.
		 */
	}

	/*
	 * At this point there are no remaining tasks,
	 * and therefore no children to wait for.
	 */
	assert(*tasks == NULL);

	return 0;

error:

	{
		int e;

		e = errno;

		/* TODO: kill children? probably not. wait(2) for all? WNOHANG */

		while (*tasks != NULL) {
			task_remove(tasks, *tasks);
		}

		errno = e;
	}

	return -1;
}

