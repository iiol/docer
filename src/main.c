#include <stdio.h>

#include "macro.h"
#include "odt.h"
#include "parser.h"


int
main(void)
{
	FILE *fp;
	odt_doc *doc;
	token *toks;


	fp = fopen("test.txt", "r");
	toks = parse_init(fp);

#if 0
	doc = odt_new();
	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));
#endif

	return 0;
}
