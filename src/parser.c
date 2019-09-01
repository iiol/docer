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


position
offsttopos(FILE *fp, long offst)
{
	long i, oldoffst;
	int c, l;
	wchar_t wc;
	position pos = {.ch = 0, .line = 0};


	oldoffst = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	for (i = 0, c = l = 1; i != offst; ++i, ++c) {
		wc = fgetwc(fp);

		if (wc == '\n') {
			c = 0;
			++l;
		}
		else if (wc == WEOF)
			return pos;
	}

	fseek(fp, oldoffst, SEEK_SET);

	pos.ch = c;
	pos.line = l;

	return pos;
}

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

static wchar_t*
getvarname(FILE *fp)
{
	int i, wordlen;
	wchar_t wc, *word;


	// TODO: set seek an reset it in break
	wordlen = 10;
	word = xmalloc(wordlen * sizeof (wchar_t));

	for (i = 0; (wc = fgetwc(fp)) != WEOF; ++i) {
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
	long offset;
	wchar_t wc, **cap;
	wchar_t *types[] = {
		tok_idtowcs(SETTINGS),
		tok_idtowcs(INCLUDE),
		tok_idtowcs(BODY),
		L"",
	};


	cap = types;
	offset = ftell(fp);

	for (i = 0; (wc = fgetwc(fp)) != WEOF; ++i) {
		for (capavailable = j = 0;; ++j) {
			if (cap[j] == NULL)
				continue;
			else if (cap[j][0] == '\0') {
				if (capavailable == 0) {
					fseek(fp, offset, SEEK_SET);
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
	token *tok;


	while ((wc = fgetwc(fp)) != WEOF) {
		if (wc == '\n') {
			wc = fgetwc(fp);

			if (iswalpha(wc)) {
				ungetwc(wc, fp);
				offset = ftell(fp);
				wcs = getvarname(fp);

				tok = tok_alloc();
				tok->type = VARNAME;
				tok->value = wcs;
				tok->offset = offset;
				skipspaces(fp);

				offset = ftell(fp);

				if ((wc = fgetwc(fp)) != '=') {
					printf("error: expected '='\n");
					skipline(fp);
					ungetwc('\n', fp);
					continue;
				}

				tok = tok_alloc();
				tok->type = '=';
				tok->offset = offset;
				skipspaces(fp);
				offset = ftell(fp);

				if ((wc = fgetwc(fp)) != '"') {
					printf("error: expected '\"'");
					skipline(fp);
					ungetwc('\n', fp);
					continue;
				}

				wcs = getvarval(fp);
				tok = tok_alloc();
				tok->type = VARVALUE;
				tok->value = wcs;
				tok->offset = offset;
			}
			else if (wc == '}') {
				offset = ftell(fp);
				wc = fgetwc(fp);

				if (wc == '\n') {
					tok = tok_alloc();
					tok->type = '}';
					tok->offset = offset;
					return;
				}
				else
					fseek(fp, -2*sizeof (wchar_t), SEEK_CUR);
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

static void
parse_body(FILE *fp)
{
	wchar_t wc;


	// TODO: finish writing function
	while (1) {
		wc = fgetwc(fp);

		if (wc == '}') {

			break;
		}
		else if (wc == WEOF)
			break;
	}
}

token*
parse_init(FILE *fp)
{
	wchar_t wc;
	long offset;
	token *tok;
	enum tok_types toktype;


	while (1) {
		skipspaces(fp);
		offset = ftell(fp);
		toktype = getboxtype(fp);

		if (toktype == END) {
			offset = ftell(fp);
			wc = fgetwc(fp);

			if (wc == '{') {
				tok = tok_alloc();
				tok->type = '{';
				tok->offset = offset;

				switch (tok->prev->type) {
				case SETTINGS:
				case INCLUDE:
					parse_vars(fp);
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
				ungetwc(wc, fp);
		}
		else if (toktype == SETTINGS ||
			 toktype == INCLUDE  ||
			 toktype == BODY) {
			tok = tok_alloc();
			tok->type = toktype;
			tok->offset = offset;
		}
		else {
			printf("error: unknown token\n");
			tok_free(tok);
		}
	}

	return tokhead;
}
