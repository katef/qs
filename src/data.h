#ifndef DATA_H
#define DATA_H

struct data {
	char *s;
	struct data *next;
};

struct data *
data_push(struct data **head, size_t n, const char *s);

struct data *
data_pop(struct data **head);

void
data_free(struct data *data);

struct data **
data_clone(struct data **dst, const struct data *src);

int
data_dump(FILE *f, const struct data *data);

#endif

