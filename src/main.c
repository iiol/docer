#include <stdio.h>
#include <locale.h>

#include "macro.h"
#include "odt.h"
#include "parser.h"
#include "stream.h"


int
main(void)
{
	int i;
	FILE *fp;
	odt_doc *doc;
	token *head, *toks;
	position pos;


	setlocale(LC_ALL, "");
	fp = fopen("test.txt", "r");
	head = toks = parse_init(fp);

#if 1	// DEBUG: print tokens
	for (i = 1; toks != NULL; ++i, toks = toks->next) {
		if (toks->type == SETTINGS ||
		    toks->type == INCLUDE  ||
		    toks->type == BODY) {
			pos = stream_ofsttopos(toks->offset);
			printf("%d) %s %d:%d\n", i, tok_idtostr(toks->type), pos.line, pos.ch);
		}
		else if (toks->type == VARNAME ||
			 toks->type == VARVALUE) {
			pos = stream_ofsttopos(toks->offset);
			printf("%d) %ls %d:%d\n", i, toks->value, pos.line, pos.ch);
		}
		else {
			pos = stream_ofsttopos(toks->offset);
			printf("%d) %c : %d:%d\n", i, toks->type, pos.line, pos.ch);
		}
	}
	toks = head;
#endif

	for (; toks != NULL; toks = toks->next) {
		if (toks->type == BODY) {
			for (; toks != NULL; toks = toks->next) {
				if (toks->type == '}')
					break;
				else if (toks->type == TEXT)
					// TODO: print text to odt file
					;
			}

			if (toks == NULL)
				break;
		}
		else if (toks->type == INCLUDE)
			// TODO: write
			;
		else if (toks->type == SETTINGS)
			// TODO: write
			;
	}

	tok_free(head);

#if 1
	doc = odt_new();

	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));
#endif

	return 0;
}
