#ifndef SCOPE_H
#define SCOPE_H

struct ast;
struct scope;

struct var **
scope_push(struct scope **sc);

struct var *
scope_pop(struct scope **sc);

struct ast *
scope_get(const struct scope *sc, const char *name);

#endif

