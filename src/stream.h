#ifndef _STREAM_H
#define _STREAM_H

typedef struct position {
	int ch;
	int line;
} position;


void stream_init(FILE *fp);
void stream_free();

void stream_wcback(int n);
wchar_t stream_getprevwc();
wchar_t stream_getwc();
void stream_skipsp();
void stream_skipline();

long stream_getofst();
void stream_setofst(long ofst);
position stream_ofsttopos(long ofst);

#endif // _STREAM_H
