#ifndef TASK_H
#define TASK_H

struct task {
	/*
	 * The next instruction to execute, set to code->next when complete.
	 *
	 * Some instructions are considered re-entrant, for which this remains
	 * pointing to the same node so that it may be evaluated again.
	 */
	const struct code *code;

	/*
	 * PID of child when waiting on a process; -1 otherwise.
	 */
	pid_t pid;

	struct task *next;
};

struct task *
task_add(struct task **t, struct code *code);

void
task_remove(struct task **t);

struct task *
task_next(const struct task *tasks);

struct task *
task_find(const struct task *head, pid_t pid);

struct task *
task_wait(const struct task *tasks, pid_t pid);

#endif

