#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "task.h"
#include "proc.h"

struct task *
task_add(struct task **head, struct code *code)
{
	struct task *new;

	assert(head != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->code = code;
	new->pid  = -1;

	new->next = *head;
	*head = new;

	return new;
}

void
task_remove(struct task **head, struct task *task)
{
	struct task **next, **t;

	assert(head != NULL);

	for (t = head; *t != NULL; t = next) {
		next = &(*t)->next;

		if (*t == task) {
			free(*t);
			*t = *next;
			return;
		}
	}
}

struct task *
task_next(const struct task *tasks)
{
	const struct task *t;

	assert(tasks != NULL);

	for (t = tasks; t != NULL; t = t->next) {
		if (t->pid != -1) {
			/* blocked on waiting for child */
			continue;
		}

		return (struct task *) t;
	}

	/*
	 * All tasks are blocked on waiting for their respective child.
	 * Now the caller should wait(2) for whichever raises SIGCHLD first,
	 * and re-enter for that child's task.
	 */

	return NULL;
}

struct task *
task_find(const struct task *head, pid_t pid)
{
	const struct task *p;

	assert(pid != -1);

	for (p = head; p != NULL; p = p->next) {
		if (p->pid == pid) {
			return (struct task *) p;
		}
	}

	return NULL;
}

void
task_promote(struct task **head, struct task *task)
{
	struct task **t;

	assert(head != NULL);
	assert(task != NULL);

	for (t = head; *t != task; t = &(*t)->next)
		;

	*t = (*t)->next;

	task->next = *head;
	*head = task;
}

/* returns number of promoted tasks which were waiting for the given PID,
 * or -1 on error */
int
task_wait(struct task **head, pid_t pid, int options)
{
	int n;

	assert(head != NULL);

	/*
	 * When we're called here, at least one SIGCHLD was delivered, so there
	 * must be at least one child waiting. There may also be more, and so we
	 * wait() repeatedly until they're all reaped.
	 *
	 * A caller may also want to block, depending on the options flag passed.
	 * Subsequent calls should never block, because there may be no further
	 * children waiting. So subsequent calls are passed WNOHANG.
	 */

	n = 0;

	for (;;) {
		struct task *t;
		pid_t r;

		r = proc_wait(pid, options);

		if (r == -1 && errno == ECHILD) {
			/* TODO: DEBUG_WAIT "no child" */
			return 0;
		}

		if (r == -1) {
			return -1;
		}

		/*
		 * TODO: maybe set some variables about the child here; stuff from waitpid()
		 *
		 * If this is called from inside the SIGCHLD handler
		 * (rather than using a self pipe), I'd need to pre-populate these variables,
		 * because I won't malloc(3) inside a signal handler.
		 */

		t = task_find(*head, r);
		if (t == NULL) {
			fprintf(stderr, "stray child, PID=%lu\n", (unsigned long) r);
			continue;
		}

		/* This task is no longer waiting for a child */
		t->pid = -1;

		task_promote(head, t);

		n++;

		if (pid != -1) {
			break;
		}

		options |= WNOHANG;
	}

	return n;
}

