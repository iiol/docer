#ifndef _LEXER_H
#define _LEXER_H

#include "macro.h"

enum tok_types {
	UNKNOWN	= 0x0,
	END	= 0x0,
	//
	// 0x01 - 0xFF: one char tokens
	//
	BOXNAME	= 0x100,
	//
	TEXT,
	STRING,
	VARIABLE,
};

typedef struct token {
	enum tok_types type;
	wchar_t *wcs;
	long offset;

	struct list_node _list;
} token;

token* lex_init(FILE *fp);
void tok_free(token *head);

#endif
