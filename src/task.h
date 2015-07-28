#ifndef TASK_H
#define TASK_H

struct code;
struct data;
struct task;
struct frame;

struct tick_state {
	char *s;
	size_t l;
};

struct task {
	/*
	 * Tasks fork from other tasks (at least conceptually); the only
	 * evidence of this is that newly-created tasks share the stack of frames
	 * from their parent. So in general this is a branch through a tree
	 * to a common root. Both parent and child may push and pop frames.
	 *
	 * A task has no business popping frames above the point where its stack
	 * branched from the parent, and the parser should never generate code
	 * which does so.
	 */
	struct frame *frame;

	/*
	 * The next instruction to execute, set to code->next when complete.
	 *
	 * Some instructions are considered re-entrant, for which this remains
	 * pointing to the same node so that it may be evaluated again.
	 *
	 * Executing code maintains its own data stack, which ought to be
	 * empty when execution is complete.
	 */
	struct code *code;
	struct data *data;

	/*
	 * PID of child when waiting on a process; -1 otherwise.
	 */
	pid_t pid;

	/*
	 * Instruction-specific state. These are short-lived things used by
	 * particular instructions either for re-entrancy or to pass state
	 * from one instruction to the next.
	 */
	struct tick_state ts;

	struct task *next;
};

struct task *
task_add(struct task **t, struct frame *frame, struct code *code);

void
task_remove(struct task **head, struct task *task);

struct task *
task_next(const struct task *tasks);

struct task *
task_find(const struct task *head, pid_t pid);

void
task_prioritise(struct task **head, struct task *task);

int
task_wait(struct task **head, pid_t pid, int options);

#endif

