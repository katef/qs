#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>
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

struct task *
task_wait(const struct task *tasks, pid_t pid)
{
	const struct task *t;

	assert(tasks != NULL);

	pid = proc_wait(pid);
	if (pid == -1) {
		return NULL;
	}

	t = task_find(tasks, pid);
	if (t == NULL) {
		errno = ECHILD; /* stray child */
		return NULL;
	}

	return (struct task *) t;
}

