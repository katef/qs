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
		goto error;
	}

	if (-1 == ast_dump(node)) {
		perror("ast_dump");
		goto error;
	}

	ast_free_node(node);

	return 0;

error:

	ast_free_node(node);

	return 1;
}

