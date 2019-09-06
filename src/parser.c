#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "parser.h"
#include "stream.h"
#include "macro.h"


#define tok_add(TYPE, VAL, OFST)	\
({					\
	token *tok = tok_alloc();	\
					\
	tok->type = TYPE;		\
	tok->value = VAL;		\
	tok->offset = OFST;		\
					\
	tok;				\
})


wchar_t *tok_types_wcs[] = {
	L"settings",
	L"include",
	L"body",
	NULL,
};

char *tok_types_str[] = {
	"settings",
	"include",
	"body",
	NULL,
};

token *tokhead;
token *toktail;


static wchar_t*
getvarname(FILE *fp)
{
	int i, wordlen;
	wchar_t wc, *word;


	wordlen = 10;
	word = xmalloc(wordlen * sizeof (wchar_t));

	for (i = 0; (wc = stream_getwc()) != WEOF; ++i) {
		if (!iswalpha(wc) && wc != '-' && wc != '_')
			break;

		if (wordlen <= i + 1) {
			wordlen *= 2;
			word = xrealloc(word, wordlen * sizeof (wchar_t));
		}

		word[i] = wc;
	}

	word[i] = '\0';
	word = xrealloc(word, ++i * sizeof (wchar_t));

	return word;
}

static wchar_t*
getvarval(FILE *fp)
{
	int i, strlen;
	long oldoffset;
	wchar_t wc, *str;


	if ((wc = stream_getwc()) != '"') {
		stream_wcback(1);

		return NULL;
	}

	strlen = 10;
	str = xmalloc(strlen * sizeof (wchar_t));
	oldoffset = stream_getofst();

	for (i = 0; (wc = stream_getwc()) != WEOF; ++i) {
		if (wc == '\\') {
			wc = stream_getwc();

			if (wc == WEOF) {
				stream_setofst(oldoffset);
				printf("error: expected '\"' at end of input\n");
				free(str);

				return NULL;
			}
		}
		else if (wc == '"')
			break;

		if (strlen <= i + 1) {
			strlen *= 2;
			str = xrealloc(str, strlen * sizeof (wchar_t));
		}

		str[i] = wc;
	}

	str[i] = '\0';
	str = xrealloc(str, ++i * sizeof (wchar_t));

	return str;
}

static enum tok_types
getboxtype(FILE *fp)
{
	int i, j, capavailable;
	long oldoffset;
	wchar_t wc, **cap;
	wchar_t *types[] = {
		tok_idtowcs(SETTINGS),
		tok_idtowcs(INCLUDE),
		tok_idtowcs(BODY),
		L"",
	};


	cap = types;
	oldoffset = stream_getofst();

	for (i = 0; (wc = stream_getwc()) != WEOF; ++i) {
		for (capavailable = j = 0;; ++j) {
			if (cap[j] == NULL)
				continue;
			else if (cap[j][0] == '\0') {
				if (capavailable == 0) {
					stream_setofst(oldoffset);
					return UNKNOWN;
				}
				break;
			}

			++capavailable;

			if (cap[j][i] == '\0') {
				if (iswalpha(wc))
					cap[j] = NULL;
				else {
					stream_wcback(1);
					return tok_wcstoid(cap[j]);
				}
			}
			else if (cap[j][i] != wc)
				cap[j] = NULL;
		}
	}

	return UNKNOWN;
}

static token*
tok_alloc()
{
	token *tok;


	tok = xmalloc(sizeof (token));
	memset(tok, 0, sizeof (token));

	if (toktail != NULL)
		toktail->next = tok;

	if (tokhead == NULL)
		tokhead = tok;

	tok->prev = toktail;
	toktail = tok;

	return tok;
}

static void
tok_free_rec(token *tok)
{
	if (tok == NULL)
		return;

	tok_free_rec(tok->next);
	free(tok);
}

void
tok_free(token *tok)
{
	if (tok == NULL)
		return;
	else if (tok == tokhead)
		tokhead = toktail = NULL;
	else {
		toktail = tok->prev;
		tok->prev->next = NULL;
	}

	tok_free_rec(tok);
}

static void
parse_vars(FILE *fp)
{
	wchar_t wc, *wcs;
	long offset;


	while ((wc = stream_getwc()) != WEOF) {
		if (wc == '\n') {
			offset = stream_getofst();
			wc = stream_getwc();

			if (iswalpha(wc)) {
				stream_wcback(1);
				wcs = getvarname(fp);

				tok_add(VARNAME, wcs, offset);
				stream_skipsp();

				offset = stream_getofst();
				wc = stream_getwc();

				if (wc != '=') {
					printf("error: expected '='\n");
					stream_skipline();
					stream_wcback(1);
					continue;
				}

				tok_add('=', NULL, offset);
				stream_skipsp();

				offset = stream_getofst();
				wc = stream_getwc();
				stream_wcback(1);

				if (wc != '"') {
					printf("error: expected '\"'");
					stream_skipline();
					stream_wcback(1);

					continue;
				}

				wcs = getvarval(fp);
				if (wcs == NULL)
					// TODO:
					// error
					// skipline ?
					;

				tok_add(VARNAME, wcs, offset);
			}
			else if (wc == '}') {
				wc = stream_getwc();

				if (wc == '\n') {
					tok_add('}', NULL, offset);

					return;
				}
				else {
					stream_wcback(2);
				}
			}
			else if (iswspace(wc))
				stream_wcback(1);
			else
				// error
				;
		}
		else if (wc == ' ' || wc == '\t')
			continue;
		else
			// error
			// skip line
			;
	}
}

static void
parse_body(FILE *fp)
{
	unsigned int i;
	wchar_t wc;
	long offset;
	wchar_t *text;
	unsigned int textlen;


	textlen = 1024;
	text = xmalloc(textlen * sizeof (wchar_t));

	for (i = 0;;) {
		offset = stream_getofst();
		wc = stream_getwc();

		if (wc == '\n') {
			offset = stream_getofst();
			wc = stream_getwc();

			if (wc == '}') {
				wc = stream_getwc();

				if (wc == '\n' || wc == WEOF) {
					if (i != 0) {
						text[i++] = '\0';
						text = xrealloc(text, i * sizeof (wchar_t));
						tok_add(TEXT, text, offset);

						i = 0;
						textlen = 1024;
						text = xmalloc(textlen * sizeof (wchar_t));
					}

					tok_add('}', NULL, offset);
				}
				else
					stream_wcback(2);
			}
			else
				stream_wcback(1);
		}
		else if (wc == WEOF) {
			if (i != 0) {
				text[i++] = '\0';
				text = xrealloc(text, i * sizeof (wchar_t));
				tok_add(TEXT, text, offset);
			}
			else
				free(text);

			return;
		}
		else {
			if (textlen <= i + 1) {
				textlen *= 2;
				text = xrealloc(text, textlen * sizeof (wchar_t));
			}

			text[i++] = wc;
		}
	}
}

token*
parse_init(FILE *fp)
{
	wchar_t wc;
	long offset;
	token *tok;
	enum tok_types toktype;


	stream_init(fp);
	tokhead = toktail = NULL;

	while (1) {
		stream_skipsp();
		offset = stream_getofst();
		toktype = getboxtype(fp);

		if (toktype == SETTINGS ||
			 toktype == INCLUDE  ||
			 toktype == BODY)
			tok_add(toktype, NULL, offset);
		else {	// if toktype == UNKNOWN
			offset = stream_getofst();
			wc = stream_getwc();

			if (wc == '{') {
				tok = tok_add('{', NULL, offset);

				switch (tok->prev->type) {
				case SETTINGS:
				case INCLUDE:
					parse_vars(fp);
					break;

				case BODY:
					parse_body(fp);
					break;

				default:
					// error
					// skip to "\n}\n"
					tok_free(tok);

					break;
				}
			}
			else if (wc == WEOF)
				break;
			else {
				// error
				// just skip without stream_wcback()
				// read token
				// read word
			}
		}
	}

	return tokhead;
}
