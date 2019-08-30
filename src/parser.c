#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "parser.h"
#include "macro.h"


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


static void
skipspaces(FILE *fp)
{
	wchar_t wc;


	while ((wc = fgetwc(fp)) != WEOF && iswspace(wc))
		;

	ungetwc(wc, fp);
}

static void
skipline(FILE *fp)
{
	wchar_t wc;

	while ((wc = fgetwc(fp)) != WEOF && wc != '\n')
		;
}

wchar_t*
getvarname(FILE *fp)
{
	int i, wordlen;
	wchar_t wc, *word;


	// TODO: set seek an reset it in break
	skipspaces(fp);

	wordlen = 10;
	word = xmalloc(wordlen * sizeof (wchar_t));

	for (i = 0; (wc = fgetwc(fp)) != WEOF; ++i) {
		if (!iswalpha(wc) && wc != '-' && wc != '_')
			break;

		if (wordlen <= i - 1) {
			wordlen *= 2;
			word = xrealloc(word, wordlen * sizeof (wchar_t));
		}

		word[i] = wc;
	}

	word[i] = '\0';
	word = xrealloc(word, ++i * sizeof (wchar_t));

	return word;
}

wchar_t*
getstring(FILE *fp)
{
	int i, strlen;
	wchar_t wc, *str;


	// TODO: set seek an reset it in break
	strlen = 10;
	str = xmalloc(strlen * sizeof (wchar_t));

	for (i = 0; (wc = fgetwc(fp)) != WEOF; ++i) {
		if (wc == '\\') {
			wc = fgetwc(fp);

			if (wc == WEOF) {
				printf("error: expected '\"' at end of input\n");
				free(str);
				return NULL;
			}
		}
		else if (wc == '"')
			break;

		if (strlen <= i - 1) {
			strlen *= 2;
			str = xrealloc(str, strlen * sizeof (wchar_t));
		}

		str[i] = wc;
	}

	str[i] = '\0';
	str = xrealloc(str, ++i * sizeof (wchar_t));

	return str;
}

enum tok_types
getboxtype(FILE *fp)
{
	int i, j, capavailable;
	long pos;
	wchar_t wc, **cap;
	wchar_t *types[] = {
		tok_idtowcs(SETTINGS),
		tok_idtowcs(INCLUDE),
		tok_idtowcs(BODY),
		L"",
	};


	pos = ftell(fp);
	skipspaces(fp);
	cap = types;

	// add capavailable
	for (i = 0; (wc = fgetwc(fp)) != WEOF; ++i) {
		for (capavailable = j = 0;; ++j) {
			if (cap[j] == NULL)
				continue;
			else if (cap[j][0] == '\0') {
				if (capavailable == 0) {
					fseek(fp, pos, SEEK_SET);
					return END;
				}
				break;
			}

			++capavailable;

			if (cap[j][i] == '\0') {
				if (iswalpha(wc))
					cap[j] = NULL;
				else {
					ungetwc(wc, fp);
					return tok_wcstoid(cap[j]);
				}
			}
			else if (cap[j][i] != wc)
				cap[j] = NULL;
		}
	}

	return END;
}

token*
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

void
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
	if (tok == tokhead)
		tokhead = toktail = NULL;
	else {
		toktail = tok->prev;
		tok->prev->next = NULL;
	}

	tok_free_rec(tok);
}

void
parse_settings(FILE *fp)
{
	wchar_t wc, *var, *val;
	token *tok;


	while ((wc = fgetwc(fp)) != WEOF) {
		tok = tok_alloc();

		if (wc == '\n') {
			wc = fgetwc(fp);

			if (wc == '}') {
				wc = fgetwc(fp);

				if (wc == '\n') {
					tok->type = '\n';
					return;
				}
				else
					fseek(fp, -2*sizeof (wchar_t), SEEK_CUR);
			}
			else if (iswalpha(wc)) {
				ungetwc(wc, fp);
				var = getvarname(fp);
				skipspaces(fp);

				if ((wc = fgetwc(fp)) != '=') {
					printf("error: expected '='\n");
					skipline(fp);
					ungetwc('\n', fp);
					continue;
				}

				skipspaces(fp);

				if ((wc = fgetwc(fp)) != '"') {
					printf("error: expected '\"'");
					skipline(fp);
					ungetwc('\n', fp);
					continue;
				}

				val = getstring(fp);
			}
			else if (iswspace(wc))
				ungetwc(wc, fp);
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

void
parse_include(FILE *fp)
{
}

void
parse_body(FILE *fp)
{
}

token*
parse_init(FILE *fp)
{
	wchar_t wc;
	token *tok;
	enum tok_types toktype;


	while (1) {
		tok = tok_alloc();
		toktype = getboxtype(fp);

		if (toktype == END) {
			wc = fgetwc(fp);

			if (wc == '{') {
				tok->type = '{';

				switch (tok->prev->type) {
				case SETTINGS:
					parse_settings(fp);
					break;

				case INCLUDE:
					parse_include(fp);
					break;

				case BODY:
					parse_body(fp);
					break;

				default:
					//error
					break;
				}
			}
			else if (wc == WEOF)
				break;
			else
				printf("error: unknown token\n");
		}
		else if (toktype == SETTINGS ||
			 toktype == INCLUDE  ||
			 toktype == BODY)
			tok->type = toktype;
		else {
			printf("error: unknown token\n");
			tok_free(tok);
		}
	}

	return tokhead;
}
