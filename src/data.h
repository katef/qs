#ifndef DATA_H
#define DATA_H

struct data {
	struct lex_pos pos;
	char *s;
	struct data *next;
};

struct data *
data_push(struct data **head, const struct lex_pos *pos,
	size_t n, const char *s);

struct data *
data_int(struct data **head, const struct lex_pos *pos,
	int n);

struct data *
data_pop(struct data **head);

void
data_free(struct data *data);

void
data_dump(FILE *f, const struct data *data);

#endif

