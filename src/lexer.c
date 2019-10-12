#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "lexer.h"
#include "stream.h"
#include "macro.h"


token *tokhead;


static token*
tok_add(enum tok_types type, wchar_t *wcs, long offset)
{
	token *tok;


	if (tokhead == NULL) {
		list_init(tok);
		tokhead = tok;
	}
	else
		tok = list_alloc_at_end(tokhead);

	tok->type = type;
	tok->wcs = wcs;
	tok->offset = offset;

	return tok;
}

void
tok_free(token *head)
{
	for (; head != NULL;) {
		if (head->type == TEXT)
			free(head->wcs);
		head = list_delete(head);
	}
}

static wchar_t*
getvarname()
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
getstring()
{
	int i, strlen;
	long oldoffset;
	wchar_t wc, *str;


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

static void
lex_arguments()
{
	wchar_t wc, *wcs;
	unsigned long offset;


	while (1) {
		stream_skipsp();
		offset = stream_getofst();
		wc = stream_getwc();

		if (wc == '"') {
			wcs = getstring();
			tok_add(STRING, wcs, offset);
		}
		else if (wc == ',')
			tok_add(',', NULL, offset);
		else if (wc == ')') {
			tok_add(')', NULL, offset);
			return;
		}
		else if (wc == WEOF)
			return;
	}
}

token*
lex_init(FILE *fp)
{
	unsigned long i, textlen, offset;
	wchar_t wc, *wcs, *text;
	int isfirstch;


	stream_init(fp);

	i = 0;
	isfirstch = 1;
	textlen = 1024;
	text = xmalloc(textlen * sizeof (wchar_t));

	while (1) {
		offset = stream_getofst();
		wc = stream_getwc();


		if (wc == '.' && (iswspace(stream_getprevwc()) ||
				  stream_getprevwc() == WEOF)) {
			isfirstch = 0;
			wcs = getvarname(fp);
			tok_add(BOXNAME, wcs, offset);
			stream_skipsp();
		}
		else if (wc == '\n') {
			isfirstch = 1;

			if (i == 0)
				continue;

			if (textlen <= i + 1) {
				textlen *= 2;
				text = xrealloc(text, textlen * sizeof (wchar_t));
			}

			text[i++] = ' ';
		}
		else if (iswspace(wc) && isfirstch)
			;
		else if (wc == '(' || wc == '{' || wc == '}' || wc == '$') {
			isfirstch = 0;

			if (i != 0) {
				text[i++] = '\0';
				text = xrealloc(text, i * sizeof (wchar_t));
				tok_add(TEXT, text, offset - wcslen(text));

				i = 0;
				textlen = 1024;
				text = xmalloc(textlen * sizeof (wchar_t));
			}

			if (wc == '(') {
				tok_add('(', NULL, offset);
				lex_arguments(fp);
				offset = stream_getofst();
				stream_skipsp();

				if (stream_getwc() == '{')
					stream_wcback(1);
				else
					stream_setofst(offset);
			}
			else if (wc == '{')
				tok_add('{', NULL, offset);
			else if (wc == '}')
				tok_add('}', NULL, offset);
			else if (wc == '$') {
				wcs = getvarname(fp);
				tok_add(VARIABLE, wcs, offset);
			}
		}
		else if (wc == '\\') {
			isfirstch = 0;
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
		else if (wc == WEOF) {
			if (i != 0) {
				text[i++] = '\0';
				text = xrealloc(text, i * sizeof (wchar_t));
				tok_add(TEXT, text, offset - wcslen(text));
			}
			else
				free(text);

			return tokhead;
		}
		else {
			isfirstch = 0;

			if (textlen <= i + 1) {
				textlen *= 2;
				text = xrealloc(text, textlen * sizeof (wchar_t));
			}

			text[i++] = wc;
		}
	}

	return tokhead;
}
