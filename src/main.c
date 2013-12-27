#include <stdio.h>

#include "ast.h"
#include "parse.h"

int
main(void)
{
	struct ast_node *node;

	/* TODO: feed from -c string or from stdin, or from filename */
	/* TODO: alternative idea: provide a function pointer to fgets, and pass stdin as void * */

	node = parse();
	if (node == NULL) {
		perror("parse");
		/* TODO: free ast */
		return 1;
	}

	if (-1 == ast_dump(node)) {
		perror("ast_dump");
		return 1;
	}

	/* TODO: free ast */

	return 0;
}

