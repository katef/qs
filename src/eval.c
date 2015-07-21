#define _POSIX_SOURCE

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

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
#include "frame.h"
#include "readfd.h"
#include "status.h"
#include "builtin.h"
#include "task.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int self[2]; /* SIGCHLD self pipe */

sigset_t ss_chld; /* SIGCHLD */

struct eval {
	struct code *code;
	pid_t pid; /* -1 when not waiting */
	struct eval *next;
};

struct pipe_state {
	int fd[2];
	int usein, useout;
	int in, out;
};

static void
sigchld(int s)
{
	const char dummy = 'x';
	int r;

	assert(s == SIGCHLD);

	(void) s;

	do {
		r = write(self[1], &dummy, sizeof dummy);
		if (r == -1 && errno != EINTR) {
			abort();
		}
	} while (r != 1);
}

/* push .s=NULL to data */
static int
eval_null(struct data **data)
{
	assert(data != NULL);

	if (!data_push(data, 0, NULL)) {
		return -1;
	}

	return 0;
}

/* push .s="xyz" to data */
static int
eval_data(struct data **data, const char *s)
{
	assert(data != NULL);
	assert(s != NULL);

	if (!data_push(data, strlen(s), s)) {
		return -1;
	}

	return 0;
}

static int
eval_pipe(struct task **next, struct frame *frame, struct code **code, struct pipe_state *ps)
{
	assert(ps->in != -1);
	assert(frame != NULL);
	assert(next != NULL);
	assert(code != NULL);
	assert(ps != NULL);

	if (-1 == pipe(ps->fd)) {
		return -1;
	}

/* TODO: pipe(2) for every fd in the ancestry.
#dup will close the ones which need closing */
/* TODO: put these fds somewhere for #run to pick up */

	if (!task_add(next, frame, *code)) {
		goto error;
	}

	*code = NULL;

	if (debug & DEBUG_FD) {
		fprintf(stderr, "pipe [%d,%d]\n", ps->fd[0], ps->fd[1]);
	}

	ps->usein  = ps->in;
	ps->in     = ps->fd[0];

/* TODO: the pipe endpoint might need to overwrite either .old or .new in the dup list */

	ps->useout = ps->fd[1];

	return 0;

error:

	close(ps->fd[0]);
	close(ps->fd[1]);

	return -1;
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
		errno = 0;
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
	struct frame *frame, struct pipe_state *ps,
	struct task *task)
{
	struct data *p, *pnext;
	struct var *v;
	pid_t pid;
	int argc;
	char **args;

	assert(next != NULL);
	assert(data != NULL);
	assert(frame != NULL);
	assert(ps != NULL);
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

		if (-1 == set_args(frame, args + 1)) {
			free(args);
			return -1;
		}

		goto done;
	}

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

		/* TODO: dup(2) here for each pair in the frame.dup list */

		/* XXX: i don't like hardcoded knowledge of STDIN_FILENO and STDOUT_FILENO here */
		/* TODO: error handling for dup2, here and elsewhere */

		if (ps->usein != STDIN_FILENO) {
			dup2(ps->usein, STDIN_FILENO);

			if (debug & DEBUG_FD) {
				fprintf(stderr, "child: dup2(%d, %d)\n", ps->usein, STDIN_FILENO);
			}

			close(ps->usein);
		}

		if (ps->useout != STDOUT_FILENO) {
			dup2(ps->useout, STDOUT_FILENO);

			if (debug & DEBUG_FD) {
				fprintf(stderr, "child: dup2(%d, %d)\n", ps->useout, STDOUT_FILENO);
			}

			close(ps->useout);
		}

		(void) proc_exec(args[0], args);
		abort();

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
}

/* read from the pipe and push to *data */
/* note will need to be able to re-enter here after EINTR */
static int
eval_tick(struct code **next, struct data **data,
	struct pipe_state *ps, struct tick_state *ts)
{
	assert(next != NULL);
	assert(data != NULL);
	assert(ps != NULL);
	assert(ts = NULL);

	/*
	 * There are two reasons to select here:
	 *
	 * Firstly, to avoid blocking on read() when a signal is delivered outside
	 * the call to read(), and therefore read() will not EINTR.
	 *
	 * Secondly, if read() returns for some other reason, so that we don't block
	 * on reading from the self-pipe when there're no signals to read.
	 * This could be catered for by only reading from the self pipe when #tick's
	 * read() gives EINTR, but since we need select() for the first case, I want
	 * to use the same mechanism for both.
	 */

	{
		fd_set fds;
		int max;
		int r;

		FD_ZERO(&fds);
		FD_SET(self[0], &fds);
		FD_SET(ps->in,  &fds);

		max = MAX(self[0], ps->in);

		r = select(max + 1, &fds, NULL, NULL, NULL);
		if (r == -1) {
			return -1;
		}

		/*
		 * It doesn't matter which fd we deal with first, since the parent's
		 * self[1] will keep the pipe open, so we can't accientally read EOF
		 * before yielding to spawn the next child.
		 */

		if (FD_ISSET(self[0], &fds)) {
			char dummy;

			do {
				r = read(self[0], &dummy, sizeof dummy);
			} while (r == 0);
			if (r == -1) {
				return -1;
			}

			assert(dummy == 'x');

			/* If ps->in is also ready, we'll deal with that on re-entry. */

			goto yield;
		}

		if (FD_ISSET(ps->in, &fds)) {
			if (-1 == sigprocmask(SIG_UNBLOCK, &ss_chld, NULL)) {
				perror("sigprocmask SIG_UNBLOCK");
				goto fail;
			}

			r = readfd(ps->in, &ts->s, &ts->l);

			if (-1 == sigprocmask(SIG_BLOCK, &ss_chld, NULL)) {
				perror("sigprocmask SIG_BLOCK");
				goto fail;
			}

			if (r == -1 && errno == EINTR) {
				goto yield;
			}

			if (r == -1) {
				return -1;
			}

			if (debug & DEBUG_VAR) {
				fprintf(stderr, "` read: %.*s\n", (int) ts->l, ts->s);
			}
		}
	}

	/* TODO: tokenise by $^ here, e.g. { echo `date | wc } */
	/* TODO: can tokenisation be shared with the lexer's guts exposed? */
	/* TODO: the lexer should use $^ */
	/* TODO: \0 should implicitly always be in the $^ set */
	/* TODO: after reading, always split by '\0'. dealing with $^ is just s//ing more '\0' in situ */
	/* TODO: push each token as a separate item to the data stack */

	if (!data_push(data, ts->l, ts->s)) {
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

	if (!code_push(next, CODE_TICK)) {
		goto error;
	}

	errno = EINTR;
	return -1;

error:

	free(ts->s);

ts->s = NULL; /* XXX: doesn't belong here */

	return -1;

fail:

	abort();
}

/* TODO: explain what happens here: status.r is the predicate, u.code is a block to call */
static int
eval_if(struct code **next, struct data **data,
	struct code **code)
{
	assert(next != NULL);
	assert(data != NULL);
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
		errno = 0;
		return -1;
	}

	a = data_pop(data);

	if (!frame_set(frame, strlen(a->s), a->s, *code)) {
		errno = EINVAL;
		return -1;
	}

	*code = NULL;

	free(a);

	return 0;
}

/* TODO: explain what happens here */
static int
op_join(struct data **node, struct frame *frame, struct data *a, struct data *b)
{
	(void) node;
	(void) frame;
	(void) a;
	(void) b;

	/* TODO */
	errno = ENOSYS;
	return -1;
}

static int
eval_frame(struct frame **frame,
	struct frame *(*op)(struct frame **head))
{
	struct frame *q;

	assert(frame != NULL && *frame != NULL);
	assert(op != NULL);

	/* TODO: detect unbalanced #push/#pop here */

	q = op(frame);
	if (q == NULL) {
		return -1;
	}

	if (op == frame_pop) {
		var_free(q->var);
		dup_free(q->dup);
		free(q);
	}

	return 0;
}

static int
eval_binop(struct data **data,
	struct frame *frame,
	int (*op)(struct data **, struct frame *, struct data *a, struct data *b))
{
	struct data *a;
	struct data *b;

	assert(data != NULL);
	assert(frame != NULL);
	assert(op != NULL);

	if (*data == NULL || (*data)->next == NULL) {
		errno = EINVAL; /* TODO: arity error */
		return -1;
	}

	a = data_pop(data);
	b = data_pop(data);

	if (-1 == op(data, frame, a, b)) {
		return -1;
	}

	free(a);
	free(b);

	return 0;
}

static int
op_dup(struct data **data, struct frame *frame, struct data *a, struct data *b)
{
	int oldfd, newfd;

	assert(data != NULL);
	assert(frame != NULL);
	assert(a != NULL);
	assert(b != NULL);

	(void) data;

	if (-1 == dup_fd(b->s, &newfd)) { return -1; }
	if (-1 == dup_fd(a->s, &oldfd)) { return -1; }

	if (oldfd == -1) {
		errno = EINVAL;
		return -1;
	}

	if (debug & DEBUG_EXEC) {
		fprintf(stderr, "dup [%d=%d]\n", oldfd, newfd);
	}

	if (!dup_push(&frame->dup, oldfd, newfd)) {
		return -1;
	}

	return 0;
}

/* eat whole data stack upto .s=NULL, set dup list for this frame */
static int
eval_dup(struct data **data, struct frame *frame)
{
	struct data *a;

	assert(data != NULL);
	assert(frame != NULL);

	while ((*data)->s != NULL) {
		if (-1 == eval_binop(data, frame, op_dup)) {
			return -1;
		}
	}

	a = data_pop(data);
	assert(a->s == NULL);
	free(a);

	return 0;
}

static int
eval_task(struct task *task, struct pipe_state *ps)
{
	struct code *node;
	int r;

	assert(task != NULL);

	/*
	 * There's no need for this loop; we could just take a single instruction
	 * at a time, and round-robin across all tasks. But while we're here,
	 * it's no trouble to evaulate as much as we can before needing to sleep.
	 *
	 * The code list may be NULL if this task is waiting for a child, but has
	 * nothing to execute after that.
TODO: in which case, would it be okay to remove the task and consider the child a stray?
	 */

	while (task->code != NULL) {
		if (debug & DEBUG_EVAL) {
			fprintf(stderr, "code: "); code_dump(stderr, task->code);
			fprintf(stderr, "data: "); data_dump(stderr, task->data);
		}

		node = code_pop(&task->code);

		assert(node != NULL);

		switch (node->type) {
		case CODE_NULL: r = eval_null(&task->data);                                     break;
		case CODE_DATA: r = eval_data(&task->data, node->u.s);                          break;
		case CODE_PIPE: r = eval_pipe(&task->next, task->frame, &node->u.code, ps);     break;
		case CODE_NOT:  r = eval_not ();                                                break;
		case CODE_IF:   r = eval_if  (&task->code, &task->data, &node->u.code);         break;
		case CODE_RUN:  r = eval_run (&task->code, &task->data, task->frame, ps, task); break;
		case CODE_CALL: r = eval_call(&task->code, &task->data, task->frame);           break;
		case CODE_TICK: r = eval_tick(&task->code, &task->data, ps, &task->ts);         break;
		case CODE_SET:  r = eval_set (&task->data, task->frame, &node->u.code);         break;
		case CODE_PUSH: r = eval_frame(&task->frame, frame_push);                       break;
		case CODE_POP:  r = eval_frame(&task->frame, frame_pop);                        break;
		case CODE_JOIN: r = eval_binop(&task->data, task->frame, op_join);              break;
		case CODE_DUP:  r = eval_dup(&task->data, task->frame);                         break;

		default:
			code_free(node);
			errno = EINVAL;
			return -1;
		}

		code_free(node);

		if (r == -1) {
			return -1;
		}

		if (task->pid != -1) {
			return 0;
		}
	}

	if (debug & DEBUG_EVAL) {
		fprintf(stderr, "eval out: ");
		data_dump(stderr, task->data);
	}

	if (task->data != NULL) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
eval_main(struct frame *top, struct code *code)
{
	struct task *tasks, *t;
	struct pipe_state ps;
	int r;

	assert(top != NULL);

	tasks = NULL;

	if (!task_add(&tasks, top, code)) {
		goto error;
	}

	/* XXX: this should be the only place where STDIN_FILENO and STDOUT_FILENO are set */
	/* XXX: setting all of these is not neccessary. some should be -1 */
	ps.in  = ps.usein  = STDIN_FILENO;
	ps.out = ps.useout = STDOUT_FILENO;

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

	t = tasks;

	for (;;) {
		r = eval_task(t, &ps);

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
			n = task_wait(&tasks, -1, WNOHANG);
			if (n == -1) {
				goto error;
			}

			/*
			 * task_wait() returns the number of newly prioritised tasks.
			 * If this is zero, then there are no more children to #run;
			 * We can close our copy of the write end of the #tick pipe,
			 * since we don't need to hold open the pipe any more.
			 * Subsequent read() in #tick will now give EOF.
			 */
			if (n == 0) {
				close(ps.out);
			}
		}

		if (t->code == NULL && t->pid == -1) {
			task_remove(&tasks, t);
		}

		if (tasks == NULL) {
			break;
		}

		t = task_next(tasks);

		/*
		 * When task_next() returns NULL, all tasks are sleeping on children.
		 * They are waited for using a blocking wait here, because there's
		 * nothing else to do.
		 */
		if (t == NULL) {
			if (-1 == task_wait(&tasks, -1, 0)) {
				goto error;
			}

			t = tasks;
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
	assert(tasks == NULL);

	return 0;

error:

	/* TODO: free tasks */
	/* TODO: kill children? probably not. wait(2) for all? WNOHANG */

	return -1;
}

int
eval(struct frame *top, struct code *code)
{
	struct sigaction sa, sa_old;
	sigset_t ss_old;
	int r;

	assert(top != NULL);

	if (-1 == pipe(self)) {
		perror("pipe");
		return -1;
	}

	if (-1 == sigemptyset(&ss_chld)) {
		perror("sigemptyset");
		return -1;
	}

	if (-1 == sigaddset(&ss_chld, SIGCHLD)) {
		perror("sigaddset");
		return -1;
	}

	if (-1 == sigprocmask(SIG_BLOCK, &ss_chld, &ss_old)) {
		perror("sigprocmask SIG_UNBLOCK");
		goto fail;
	}

	sa.sa_handler = sigchld;
	sa.sa_flags   = SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &sa, &sa_old)) {
		perror("sigaction");
		goto fail;
	}

	r = eval_main(top, code);

	if (sigaction(SIGCHLD, &sa_old, NULL)) {
		perror("sigaction");
		goto fail;
	}

	if (-1 == sigprocmask(SIG_SETMASK, &ss_old, NULL)) {
		perror("sigprocmask SIG_SETMASK");
		goto fail;
	}

	close(self[0]);
	close(self[1]);

	return r;

fail:

	/*
	 * Failures which would leave the process in an altered state;
	 * I don't see how we could expect to recover here.
	 */

	abort();
}

