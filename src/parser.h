#ifndef _PARSER_H
#define _PARSER_H

#include "macro.h"

#define tok_idtowcs(id) tok_types_wcs[id - 0x100]
#define tok_idtostr(id) tok_types_str[id - 0x100]

#define tok_wcstoid(wcs) ({					\
	int i, ret;						\
								\
	for (ret = i = 0; tok_types_wcs[i] != NULL; ++i)	\
		if (wcscmp(wcs, tok_types_wcs[i]) == 0)		\
			ret = 0x100 + i;			\
								\
	ret;							\
})

#define tok_strtoid(str) ({					\
	int i, ret;						\
								\
	for (ret = i = 0; tok_types_str[i] != NULL; ++i)	\
		if (strcmp(str, tok_types_str[i]) == 0)		\
			ret = 0x100 + i;			\
								\
	ret;							\
})

extern wchar_t *tok_types_wcs[];
extern char *tok_types_str[];

enum tok_types {
	UNKNOWN  = 0x0,
	END      = 0x0,
	//
	// 0x01 - 0xFF: one char tokens
	//
	// box types:
	SETTINGS_BOX = 0x100,
	INCLUDE_BOX,
	BODY_BOX,
	VARIABLE_BOX,
	//
	// other tokens (tokens in boxes):
	TEXT,
	VARIABLE,
	VARNAME,
	VARVALUE,
};

typedef struct variable {
	wchar_t *name;
	wchar_t *val;
} variable;

typedef struct token {
	enum tok_types type;
	void *value;
	long offset;

	struct list_node _list;
} token;

token* parse_init(FILE *fp);
token* tok_add(enum tok_types type, void *val, long offset);
void tok_free(token *head);

#endif
