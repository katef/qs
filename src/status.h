#ifndef STATUS_H
#define STATUS_H

struct status {
	int r;
	int s;
};

extern struct status status;

void
status_exit(int r);

void
status_sig(int s);

int
status_print(FILE *f);

#endif

