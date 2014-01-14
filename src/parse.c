#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "debug.h"
#include "lex.h"
#include "bet.h"
#include "parse.h"

enum parse {
	p_accept,
	p_reject,
	p_error,
	p_panic
};

#define EXPECT t->type
#define ACCEPT lex_next(l, t);             \
               switch (t->type) {          \
               case tok_error: goto error; \
               case tok_panic: goto panic; \
               default:        break;      \
               }

static enum parse p_exprs(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_expr(struct lex_state *l, struct lex_tok *t, struct bet **bet);

static enum parse p_list_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_assign_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_pipe_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_and_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_or_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_exprs_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);

static enum parse
p_list(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_str:

		a = bet_new_leaf(BET_STR, t->e - t->s, t->s);
		if (a == NULL) {
			return p_error;
		}

		ACCEPT;

		switch (p_list_prime(l, t, &b)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> 'str' %s_prime ;\n",
					__func__ + 2, __func__ + 2); /* TODO: show actual str */
			}

			goto accept;

/* TODO: free a, b */
		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

	default:

		break;
	}

	return p_reject;

accept:

	if (a == NULL) {
		*out = b;
	} else if (b == NULL) {
		*out = a;
	} else {
		*out = bet_new_branch(BET_CONS, a, b);
		if (*out == NULL) {
/* TODO: free a, b */
			return p_error;
		}
	}

	return p_accept;

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_list_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_list(l, t, out)) {
	case p_accept:

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> list ;\n", __func__ + 2);
		}

		return p_accept;

	case p_reject:

		break;

	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}
}

static enum parse
p_cmd(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_list(l, t, out)) {
	case p_accept:
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> list;\n", __func__ + 2);
		}

		return p_accept;

	case p_reject:

		break;

	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

	/* alt2 */
/* TODO: need a node type for {} */
	switch (EXPECT) {
	case tok_obrace:

		ACCEPT;

		switch (p_exprs(l, t, out)) {
		case p_accept: break;
		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		switch (EXPECT) {
		case tok_cbrace:

			ACCEPT;

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"{\" expr \"}\" ;\n", __func__ + 2);
			}

			return p_accept;

		default:

			return p_reject;
		}


		return p_accept;

	default:

		break;
	}

	/* alt3 */
/* TODO: need a node type for () */
	switch (EXPECT) {
	case tok_oparen:

		ACCEPT;

		switch (p_exprs(l, t, out)) {
		case p_accept: break;
		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		switch (EXPECT) {
		case tok_cparen:

			ACCEPT;

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"(\" expr \")\" ;\n", __func__ + 2);
			}

			return p_accept;

		default:

			return p_reject;
		}

		return p_accept;

	default:

		break;
	}

	return p_reject;

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_assign_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_cmd(l, t, &a)) {
	case p_accept:
		switch (p_assign_expr_prime(l, t, &b)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> cmd %s_prime ;\n",
					__func__ + 2, __func__ + 2);
			}

			goto accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

	case p_reject: return p_reject;
	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

accept:

	if (a == NULL) {
		*out = b;
	} else if (b == NULL) {
		*out = a;
	} else {
		*out = bet_new_branch(BET_ASSIGN, a, b);
		if (*out == NULL) {
/* TODO: free a, b */
			return p_error;
		}
	}

	return p_accept;
}

static enum parse
p_assign_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_equ:

		ACCEPT;

		switch (p_assign_expr(l, t, out)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"=\" assign_expr ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		return p_accept;

	default:

		break;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_pipe_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_assign_expr(l, t, &a)) {
	case p_accept:
		switch (p_pipe_expr_prime(l, t, &b)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> assign_expr %s_prime ;\n",
					__func__ + 2, __func__ + 2);
			}

			goto accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

	case p_reject: return p_reject;
	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

accept:

	if (a == NULL) {
		*out = b;
	} else if (b == NULL) {
		*out = a;
	} else {
		*out = bet_new_branch(BET_PIPE, a, b);
		if (*out == NULL) {
/* TODO: free a, b */
			return p_error;
		}
	}

	return p_accept;
}

static enum parse
p_pipe_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_pipe:

		ACCEPT;

		switch (p_pipe_expr(l, t, out)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"|\" pipe_expr ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		return p_accept;

	default:

		break;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_and_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_pipe_expr(l, t, &a)) {
	case p_accept:
		switch (p_and_expr_prime(l, t, &b)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> pipe_expr %s_prime ;\n",
					__func__ + 2, __func__ + 2);
			}

			goto accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

	case p_reject: return p_reject;
	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

accept:

	if (a == NULL) {
		*out = b;
	} else if (b == NULL) {
		*out = a;
	} else {
		*out = bet_new_branch(BET_AND, a, b);
		if (*out == NULL) {
		/* TODO: free a, b */
			return p_error;
		}
	}

	return p_accept;
}

static enum parse
p_and_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_and:

		ACCEPT;

		switch (p_and_expr(l, t, out)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"&&\" and_expr ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		return p_accept;

	default:

		break;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_or_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_and_expr(l, t, &a)) {
	case p_accept:
		switch (p_or_expr_prime(l, t, &b)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> and_expr %s_prime ;\n",
					__func__ + 2, __func__ + 2);
			}

			goto accept;

/* TODO: free bet */
		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;
		}

	case p_reject: return p_reject;
	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

accept:

	if (a == NULL) {
		*out = b;
	} else if (b == NULL) {
		*out = a;
	} else {
		*out = bet_new_branch(BET_OR, a, b);
		if (*out == NULL) {
/* TODO: free a, b */
			return p_error;
		}
	}

	return p_accept;
}

static enum parse
p_or_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_or:

		ACCEPT;

		switch (p_or_expr(l, t, out)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> \"||\" or_expr ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

		return p_accept;

	default:

		break;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_postfixop(struct lex_state *l, struct lex_tok *t,
	enum bet_type *out)
{
	const char *s;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	switch (EXPECT) {
	case tok_nl:   *out = BET_EXEC; s = "\\n"; break;
	case tok_semi: *out = BET_EXEC; s = ";";   break;
	case tok_bg:   *out = BET_BG;   s = "&";   break;

	default:

		return p_reject;
	}

	if (debug & DEBUG_PARSE) {
		fprintf(stderr, "%s -> \"%s\" ;\n",
			__func__ + 2, s);
	}

	ACCEPT;

	return p_accept;

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_exprs(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	struct bet *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_expr(l, t, out)) {
	case p_accept:
		switch (p_exprs_prime(l, t, &b)) {
		case p_accept:

			/* XXX: uses knowledge internal to p_expr() */
			assert((*out)->u.op.b == NULL);
			(*out)->u.op.b = b;

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> and_expr %s_prime ;\n",
					__func__ + 2, __func__ + 2);
			}

			return p_accept;

/* TODO: free bet */
		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;
		}

	case p_reject:

			break;

	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

	return p_reject;
}

static enum parse
p_exprs_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_expr(l, t, out)) {
	case p_accept:

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> expr ;\n", __func__ + 2);
		}

		return p_accept;

	case p_reject:

		break;

	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}

	/* alt2 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}
}

static enum parse
p_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **out)
{
	enum bet_type op;
	struct bet *a, *b;

	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(out != NULL);

	/* alt1 */
	switch (p_or_expr(l, t, &a)) {
	case p_accept:
		switch (p_postfixop(l, t, &op)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> or_expr postfixop ;\n", __func__ + 2);
			}

			b = NULL;

			goto accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;

		default:
			errno = EINVAL;
			return p_error;
		}

	case p_reject:
		break;

	case p_error: return p_error;
	case p_panic: return p_panic;
	}

	/* alt2, alt3 */
	{
		switch (EXPECT) {
		case tok_nl:
		case tok_semi:

			ACCEPT;

			if (debug & DEBUG_PARSE) {
/* XXX: quote newline */
				fprintf(stderr, "%s -> \"%.*s\" ;\n",
					__func__ + 2, (int) (t->e - t->s), t->s);
			}

*out = NULL;

			return p_accept;

		default:

			break;
		}
	}

	/* alt4 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

*out = NULL;

		return p_accept;
	}

accept:

	/*
	 * Note here we cannot fold for when only one of a or b is NULL,
	 * because BET_BG has different semantics than BET_EXEC.
	 */

	if (a == NULL && b == NULL) {
		*out = NULL;
	} else {
		*out = bet_new_branch(op, a, b);
		if (*out == NULL) {
			return p_error;
		}
	}

	return p_accept;

error:

	return p_error;

panic:

	return p_panic;
}

int
parse(struct lex_state *l,
	int (*dispatch)(struct bet *))
{
	struct lex_tok t;

	assert(l != NULL);
	assert(l->f != NULL);
	assert(dispatch != NULL);

	l->p  = l->buf;

	lex_next(l, &t);

	if (t.type == tok_error) {
		return -1;
	}

	while (t.type != tok_eof) {
		struct bet *bet;

		switch (p_expr(l, &t, &bet)) {
		case p_accept: break;
		case p_reject: goto eof;
		case p_error:  return -1;
		case p_panic:  continue;
		}

		if (-1 == dispatch(bet)) {
			return -1;
		}

		bet_free(bet);
	}

eof:

	if (debug & DEBUG_PARSE) {
		fprintf(stderr, "%s -> { expr } <eof> ;\n", "script");
	}

	return 0;
}

