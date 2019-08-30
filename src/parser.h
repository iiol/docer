#ifndef _PARSER_H
#define _PARSER_H

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
	END      = 0x0,
	// 0x01 - 0xFF: one char tokens
	SETTINGS = 0x100,
	INCLUDE,
	BODY,
};

typedef struct variable {
	char *name;
	char *val;
} variable;

typedef struct token {
	enum tok_types type;
	union {
		char *val;
		variable **vars;
	};

	struct token *prev;
	struct token *next;
} token;

token* parse_init(FILE *fp);

#endif
