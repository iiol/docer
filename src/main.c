#include <stdio.h>
#include <locale.h>

#include "macro.h"
#include "odt.h"
#include "lexer.h"
#include "parser.h"
#include "stream.h"
#include "dhat.h"


static void
print_indent(int n)
{
	for (; n != 0; --n)
		printf("  ");
}

static void
print_tok_debug(token *toks)
{
	int i, j;
	position pos;

	i = j = 0;
	printf("Tokens:\n");
	list_foreach (toks, toks) {
		++i;
		pos = stream_ofsttopos(toks->offset);

		printf("%3d) at: %2d:%-2d  ", i, pos.line, pos.ch);

		if (toks->type == '{')
			print_indent(j++);
		else if (toks->type == '}')
			print_indent(--j);
		else
			print_indent(j);

		if (toks->type == BOXNAME)
			printf(".%ls\n", toks->wcs);
		else if (toks->type == STRING)
			printf("\"%ls\"\n", toks->wcs);
		else if (toks->type == TEXT)
			printf("TEXT: '%ls'\n", toks->wcs);
		else if (toks->type == VARIABLE)
			printf("VAR : '%ls'\n", toks->wcs);
		else
			printf("%c\n", toks->type);
	}
}

static void
print_box_debug(struct box_content *cont)
{
	static int level = 0;


	list_foreach (cont, cont) {
		if (cont->type == BOX) {
			print_indent(level);
			printf("%ls\n", cont->box->name);

			if (cont->box->cont) {
				++level;
				print_box_debug(cont->box->cont);
			}
		}
		else if (cont->type == TOK) {
			print_indent(level);
			printf("TOKEN\n");
		}
		else
			printf("ERROR\n");
	}

	if (level != 0)
		--level;
}

int
main(void)
{
	FILE *fp;
	odt_doc *doc;
	token *toks;
	struct box_content *cont;


	setlocale(LC_ALL, "");
	fp = fopen("test.txt", "r");
	toks = lex_init(fp);
	cont = parse_init(toks);
	doc = odt_new();

#if 1	// DEBUG
	print_tok_debug(toks);
	putchar('\n');
	print_box_debug(cont);
#endif

	parse_generate(doc, cont);

	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));

	odt_free(doc);
	tok_free(toks);
	stream_free();

	return 0;
}
