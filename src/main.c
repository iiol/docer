#include <stdio.h>

#include "macro.h"
#include "odt.h"
#include "parser.h"


int
main(void)
{
	int i;
	FILE *fp;
	odt_doc *doc;
	token *head, *toks;
	position pos;


	fp = fopen("test.txt", "r");
	head = toks = parse_init(fp);

	for (i = 1; toks != NULL; ++i, toks = toks->next) {
		if (toks->type == SETTINGS ||
		    toks->type == INCLUDE  ||
		    toks->type == BODY) {
			pos = offsttopos(fp, toks->offset);
			printf("%d) %s %d:%d\n", i, tok_idtostr(toks->type), pos.line, pos.ch);
		}
		else if (toks->type == VARNAME ||
			 toks->type == VARVALUE) {
			pos = offsttopos(fp, toks->offset);
			printf("%d) %ls %d:%d\n", i, toks->value, pos.line, pos.ch);
		}
		else {
			pos = offsttopos(fp, toks->offset);
			printf("%d) %c : %d:%d\n", i, toks->type, pos.line, pos.ch);
		}
	}

	tok_free(head);

#if 0
	doc = odt_new();
	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));
#endif

	return 0;
}
