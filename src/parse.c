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

static enum parse p_expr(struct lex_state *l, struct lex_tok *t, struct bet **bet);

static enum parse p_list_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_assign_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_pipe_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_and_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);
static enum parse p_or_expr_prime(struct lex_state *l, struct lex_tok *t, struct bet **bet);

static enum parse
p_list(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_str:

		ACCEPT;

		switch (p_list_prime(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> 'str' list_prime ;\n", __func__ + 2); /* TODO: show actual str */
			}

			return p_accept;

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

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_list_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_list(l, t, bet)) {
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

		return p_accept;
	}
}

static enum parse
p_cmd(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_list(l, t, bet)) {
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
	switch (EXPECT) {
	case tok_obrace:

		ACCEPT;

		switch (p_expr(l, t, bet)) {
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
	switch (EXPECT) {
	case tok_oparen:

		ACCEPT;

		switch (p_expr(l, t, bet)) {
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
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_cmd(l, t, bet)) {
	case p_accept:
		switch (p_assign_expr_prime(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> cmd assign_expr_prime ;\n", __func__ + 2);
			}

			return p_accept;

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
}

static enum parse
p_assign_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_pipe:

		ACCEPT;

		switch (p_assign_expr(l, t, bet)) {
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

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_pipe_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_assign_expr(l, t, bet)) {
	case p_accept:
		switch (p_pipe_expr_prime(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> assign_expr pipe_expr_prime ;\n", __func__ + 2);
			}

			return p_accept;

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
}

static enum parse
p_pipe_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_pipe:

		ACCEPT;

		switch (p_pipe_expr(l, t, bet)) {
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

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_and_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_pipe_expr(l, t, bet)) {
	case p_accept:
		switch (p_and_expr_prime(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> pipe_expr and_expr_prime ;\n", __func__ + 2);
			}

			return p_accept;

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
}

static enum parse
p_and_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_and:

		ACCEPT;

		switch (p_and_expr(l, t, bet)) {
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

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_or_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_and_expr(l, t, bet)) {
	case p_accept:
		switch (p_or_expr_prime(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> and_expr or_expr_prime ;\n", __func__ + 2);
			}

			return p_accept;

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
}

static enum parse
p_or_expr_prime(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (EXPECT) {
	case tok_or:

		ACCEPT;

		switch (p_or_expr(l, t, bet)) {
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

		return p_accept;
	}

error:

	return p_error;

panic:

	return p_panic;
}

static enum parse
p_expr(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_or_expr(l, t, bet)) {
	case p_accept:

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> or_expr ;\n", __func__ + 2);
		}

		return p_accept;

	case p_reject: return p_reject;
	case p_error:  return p_error;
	case p_panic:  return p_panic;

	default:
		errno = EINVAL;
		return p_error;
	}
}

static enum parse
p_postfixop(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	switch (EXPECT) {
	case tok_nl:
	case tok_semi:
	case tok_bg:

		ACCEPT;

		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> \"\\n\" | \";\" | \"&\" ;\n", __func__ + 2);
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
p_exprs(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	/* alt1 */
	switch (p_postfixop(l, t, bet)) {
	case p_accept:
		switch (p_exprs(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> postfixop exprs ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;
		}

	case p_reject:
		break;

	case p_error: return p_error;
	case p_panic: return p_panic;
	}

	/* alt2 */
	switch (p_expr(l, t, bet)) {
	case p_accept:
		switch (p_postfixop(l, t, bet)) {
		case p_accept:

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> expr postfixop ;\n", __func__ + 2);
			}

			return p_accept;

		case p_reject: return p_reject;
		case p_error:  return p_error;
		case p_panic:  return p_panic;
		}

	case p_reject:
		break;

	case p_error:  return p_error;
	case p_panic:  return p_panic;
	}

	/* alt3 (epsilon) */
	{
		if (debug & DEBUG_PARSE) {
			fprintf(stderr, "%s -> ;\n", __func__ + 2);
		}

		return p_accept;
	}
}

static enum parse
p_script(struct lex_state *l, struct lex_tok *t,
	struct bet **bet)
{
	assert(l != NULL);
	assert(t != NULL && t->type != tok_error && t->type != tok_panic);
	assert(bet != NULL);

	switch (p_exprs(l, t, bet)) {
	case p_accept:

		switch (EXPECT) {
		case tok_eof:

			/* no ACCEPT, since we're at EOF */

			if (debug & DEBUG_PARSE) {
				fprintf(stderr, "%s -> exprs ;\n", __func__ + 2);
			}

			return p_accept;

		default:

			return p_reject;
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

int
parse(struct lex_state *l, struct bet **bet_out)
{
	struct lex_tok t;

	assert(l != NULL);
	assert(l->f != NULL);
	assert(bet_out != NULL);

	l->p  = l->buf;

	lex_next(l, &t);

	if (t.type == tok_error) {
		return -1;
	}

	do {
		if (p_error == p_script(l, &t, bet_out)) {
			return -1;
		}
	} while (t.type == tok_panic);

	if (t.type == tok_error) {
		return -1;
	}

	return 0;
}

