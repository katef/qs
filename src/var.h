#ifndef VAR_H
#define VAR_H

struct ast;
struct var;

static struct var *
var_find(struct var *v, const char *name);

static struct var *
var_new(struct var **v, const char *name, struct ast *val);

struct var *
var_set(struct var **v, const char *name, struct ast *val);

struct ast *
var_get(struct var *v, const char *name);

void
var_free(struct var *v);

#endif

