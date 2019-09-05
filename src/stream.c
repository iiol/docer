#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "macro.h"
#include "stream.h"


wchar_t *wcstring;
unsigned long wcstringlen, offset;


void
stream_wcback(int n)
{
	if (offset > n)
		offset -= n;
	else
		offset = 0;
}

wchar_t
stream_getwc()
{
	if (offset < wcstringlen)
		return wcstring[offset++];
	else
		return WEOF;
}

void
stream_skipsp()
{
	wchar_t wc;


	while ((wc = stream_getwc()) != WEOF && iswspace(wc))
		;

	stream_wcback(1);
}

void
stream_skipline()
{
	wchar_t wc;


	while ((wc = stream_getwc()) != WEOF && wc != '\n')
		;
}

long
stream_getofst()
{
	return offset;
}

void
stream_setofst(long ofst)
{
	offset = ofst;
}

position
stream_ofsttopos(long ofst)
{
	long i;
	int cn, ln;
	char c[MB_CUR_MAX];
	wchar_t wc;
	position pos = {.ch = 0, .line = 0};


	for (i = 0, cn = ln = 1; i < ofst; ++cn) {
		wc = wcstring[i];

		memset(c, 0, MB_CUR_MAX);
		wctomb(c, wc);
		i += strlen(c);

		if (wc == '\n') {
			cn = 0;
			++ln;
		}
		else if (wc == WEOF)
			return pos;
	}

	pos.ch = cn;
	pos.line = ln;

	return pos;
}

// TODO: rewrite
void
stream_init(FILE *fp)
{
	int i;
	unsigned long wcslen;
	wchar_t wc;


	offset = 0;
	wcslen = 1024;
	wcstring = xmalloc(wcslen * sizeof (wchar_t));

	for (i = 0;; ++i) {
		wc = fgetwc(fp);

		if (wc == WEOF)
			break;
		else if (wcslen <= i + 1) {
			wcslen *= 2;
			wcstring = xrealloc(wcstring, wcslen * sizeof (wchar_t));
		}

		wcstring[i] = wc;
	}

	wcstringlen = i;
	wcstring = xrealloc(wcstring, i * sizeof (wchar_t));
}
