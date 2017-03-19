/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef VAR_H
#define VAR_H

struct ast;

struct var {
	char *name;
	struct code *code;
	struct var *next;
};

void
var_replace(struct var *v, struct code *code);

struct var *
var_set(struct var **v, size_t n, const char *name,
	struct code *code);

struct var *
var_get(struct var *v, size_t n, const char *name);

void
var_free(struct var *v);

#endif

