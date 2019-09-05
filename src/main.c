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
	printf("Tokens:\n");
	for (i = 1; toks != NULL; ++i, toks = toks->next) {
		if (toks->type == SETTINGS ||
		    toks->type == INCLUDE  ||
		    toks->type == BODY) {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  %s \n", i, pos.line, pos.ch, tok_idtostr(toks->type));
		}
		else if (toks->type == VARNAME ||
			 toks->type == VARVALUE) {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  %ls\n", i, pos.line, pos.ch, toks->value);
		}
		else if (toks->type == TEXT) {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  TEXT: %ls\n", i, pos.line, pos.ch, toks->value);
		}
		else {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  %c\n", i, pos.line, pos.ch, toks->type);
		}
	}
	toks = head;
#endif

	doc = odt_new();

	for (; toks != NULL; toks = toks->next) {
		if (toks->type == BODY) {
			for (; toks != NULL; toks = toks->next) {
				if (toks->type == '}')
					break;
				else if (toks->type == TEXT)
					odt_set_text(doc, toks->value);
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

	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));

	tok_free(head);

	return 0;
}
