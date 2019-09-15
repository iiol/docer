#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "parser.h"
#include "stream.h"
#include "macro.h"


token *tokhead;

wchar_t *tok_types_wcs[] = {
	L"settings",
	L"include",
	L"body",
	L"variable",
	NULL,
};

char *tok_types_str[] = {
	"settings",
	"include",
	"body",
	"variable",
	NULL,
};


token*
tok_add(enum tok_types type, void *val, long offset)
{
	token *tok;


	if (tokhead == NULL) {
		list_init(tok);
		tokhead = tok;
	}
	else
		tok = list_alloc_at_end(tokhead);

	tok->type = type;
	tok->value = val;
	tok->offset = offset;

	return tok;
}

void
tok_free(token *head)
{
	for (; head != NULL;) {
		if (head->type == TEXT ||
		    head->type == VARNAME)
			free(head->value);
		head = list_delete(head);
	}
}

static wchar_t*
getvarname(FILE *fp)
{
	int i, wordlen;
	wchar_t wc, *word;


	wordlen = 10;
	word = xmalloc(wordlen * sizeof (wchar_t));

	for (i = 0; (wc = stream_getwc()) != WEOF; ++i) {
		if (!iswalpha(wc) && wc != '_') {
			stream_wcback(1);
			break;
		}

		if (wordlen <= i + 1) {
			wordlen *= 2;
			word = xrealloc(word, wordlen * sizeof (wchar_t));
		}

		word[i] = wc;
	}

	word[i++] = '\0';
	word = xrealloc(word, i * sizeof (wchar_t));

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
		tok_idtowcs(SETTINGS_BOX),
		tok_idtowcs(INCLUDE_BOX),
		tok_idtowcs(BODY_BOX),
		tok_idtowcs(VARIABLE_BOX),
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

static void
parse_vars(FILE *fp)
{
	wchar_t wc, *wcs;
	long offset;


	while (1) {
		offset = stream_getofst();
		wc = stream_getwc();

		if (iswalpha(wc) && stream_getprevwc() == '\n') {
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

			tok_add(VARVALUE, wcs, offset);
		}
		else if (wc == '}' && stream_getprevwc() == '\n') {
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
			continue;
		else if (wc == WEOF)
			break;
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
	wchar_t *text, *varname;
	unsigned int textlen;


	i = 0;
	textlen = 1024;
	text = xmalloc(textlen * sizeof (wchar_t));

	while (1) {
		offset = stream_getofst();
		wc = stream_getwc();

		if (wc == '\\') {
			wc = stream_getwc();

			if (wc == WEOF)
				// TODO: error
				;

			if (textlen <= i + 1) {
				textlen *= 2;
				text = xrealloc(text, textlen * sizeof (wchar_t));
			}

			text[i++] = wc;
		}
		else if (wc == '}' && stream_getprevwc() == '\n') {
			offset = stream_getofst() - 1;
			wc = stream_getwc();

			if (wc == '\n' || wc == WEOF) {
				if (i != 0) {
					i -= 1;
					text[i++] = '\0';
					text = xrealloc(text, i * sizeof (wchar_t));
					tok_add(TEXT, text, offset - wcslen(text));
				}
				else
					free(text);

				tok_add('}', NULL, offset);

				break;
			}
			else
				stream_wcback(2);
		}
		else if (wc == '$') {
			if (i != 0) {
				text[i++] = '\0';
				text = xrealloc(text, i * sizeof (wchar_t));
				tok_add(TEXT, text, offset - wcslen(text));

				i = 0;
				textlen = 1024;
				text = xmalloc(textlen * sizeof (wchar_t));
			}

			varname = getvarname(fp);
			tok_add(VARIABLE, varname, offset);
		}
		else if (wc == WEOF) {
			if (i != 0) {
				text[i++] = '\0';
				text = xrealloc(text, i * sizeof (wchar_t));
				tok_add(TEXT, text, offset - wcslen(text));
			}
			else
				free(text);

			return;
		}
		else { // if text
			if (textlen <= i + 1) {
				textlen *= 2;
				text = xrealloc(text, textlen * sizeof (wchar_t));
			}

			if (iswspace(wc))
				wc = ' ';

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

	while (1) {
		stream_skipsp();
		offset = stream_getofst();
		toktype = getboxtype(fp);

		if (toktype == SETTINGS_BOX ||
			 toktype == INCLUDE_BOX  ||
			 toktype == BODY_BOX ||
			 toktype == VARIABLE_BOX)
			tok_add(toktype, NULL, offset);
		else {	// if toktype == UNKNOWN
			offset = stream_getofst();
			wc = stream_getwc();

			if (wc == '{') {
#if 1	// TODO:
				wc = stream_getwc();
				if (wc != '\n') {
					// error
					// skip
					continue;
				}
#endif

				tok = tok_add('{', NULL, offset);

				switch (list_get_prev(tok)->type) {
				case SETTINGS_BOX:
				case INCLUDE_BOX:
				case VARIABLE_BOX:
					parse_vars(fp);
					break;

				case BODY_BOX:
					parse_body(fp);
					break;

				default:
					// error
					// skip to "\n}\n"
					list_delete(tok);

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
