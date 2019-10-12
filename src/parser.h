#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"
#include "odt.h"

enum cont_type {
	TOK,
	BOX,
};

struct box_args {
	wchar_t *arg;

	struct list_node _list;
};

struct box {
	wchar_t *name;
	struct box_args *args;
	struct box_content *cont;
};

struct box_content {
	enum cont_type type;

	union {
		token *tok;
		struct box *box;
	};

	struct list_node _list;
};

struct box_content* parse_init(token *head);
void parse_generate(odt_doc *doc, struct box_content *cont);

#endif // _PARSER_H
