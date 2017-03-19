/*
 * Copyright 2013-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef OUT_H
#define OUT_H

int out_qs   (FILE *f, struct ast *a);
int out_ast  (FILE *f, struct ast *a);
int out_frame(FILE *f, struct ast *a);
int out_eval (FILE *f, struct ast *a);

#endif

