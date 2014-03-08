#ifndef VAR_H
#define VAR_H

struct ast;

struct var {
	char *name;
	struct code *code;
	struct data *data;
	struct var *next;
};

struct var *
var_set(struct var **v, size_t n, const char *name,
	struct code *code, struct data *data);

struct var *
var_get(struct var *v, size_t n, const char *name);

void
var_free(struct var *v);

#endif

