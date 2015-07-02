#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>

#include "task.h"

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

