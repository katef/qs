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
task_remove(struct task **head)
{
	struct task *tmp;

	assert(head != NULL);

	/* TODO: free rtrn stack */

	tmp = *head;

	*head = tmp->next;

	free(tmp);
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

int
task_wait(struct task **head, pid_t pid)
{
	struct task *t;

	assert(head != NULL);

	/*
	 * When we're called here, at least one SIGCHLD was delivered, so there
	 * must be at least one child waiting. There may also be more, and so we
	 * wait() repeatedly until they're all reaped.
	 *
	 * When we're called to wait because all tasks are blocked, this ought to
	 * block until a child is reaped. So our first call here is a blocking
	 * wait(), and further calls are passed WNOHANG.
	 */

	/* TODO: this code is awkward, and could do with some refactoring */

	pid = proc_wait(pid, 0);
	if (pid == -1) {
		return -1;
	}

	do {
		t = task_find(*head, pid);
		if (t == NULL) {
			fprintf(stderr, "stray child, PID=%lu\n", (unsigned long) pid);
			goto wait;
		}

		/*
		 * TODO: maybe set some variables about the child here; stuff from waitpid()
		 *
		 * If this is called from inside the SIGCHLD handler
		 * (rather than using a self pipe), I'd need to pre-populate these variables,
		 * because I won't malloc(3) inside a signal handler.
		 */

		/* This task is no longer waiting for a child */
		t->pid = -1;

		task_promote(head, t);

wait:

		pid = proc_wait(pid, WNOHANG);
		if (pid == -1) {
			return -1;
		}

	} while(pid != 0);

	return 0;
}

