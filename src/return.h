#ifndef RETURN_H
#define RETURN_H

struct rtrn {
	const struct code *code;
	struct rtrn *next;
};

struct rtrn *
rtrn_push(struct rtrn **head, const struct code *code);

struct rtrn *
rtrn_pop(struct rtrn **head);

void
rtrn_free(struct rtrn *rtrn);

#endif

