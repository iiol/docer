#include <stdio.h>
#include <locale.h>

#include "macro.h"
#include "odt.h"
#include "parser.h"
#include "stream.h"
#include "dhat.h"


int
main(void)
{
	int i;
	FILE *fp;
	odt_doc *doc;
	token *head, *toks;
	position pos;
	dhat *vars;
	wchar_t *varname, *varval, *text;


	setlocale(LC_ALL, "");
	fp = fopen("test.txt", "r");
	head = parse_init(fp);
	vars = dhat_new(20, 100, 2);

#if 1	// DEBUG: print tokens
	i = 0;
	printf("Tokens:\n");
	list_foreach (head, toks) {
		++i;

		if (toks->type == SETTINGS_BOX ||
		    toks->type == INCLUDE_BOX  ||
		    toks->type == BODY_BOX ||
		    toks->type == VARIABLE_BOX) {
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
			printf("%3d) at: %2d:%-2d  TEXT: '%ls'\n", i, pos.line, pos.ch, toks->value);
		}
		else if (toks->type == VARIABLE) {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  VARIABLE: '%ls'\n", i, pos.line, pos.ch, toks->value);
		}
		else {
			pos = stream_ofsttopos(toks->offset);
			printf("%3d) at: %2d:%-2d  %c\n", i, pos.line, pos.ch, toks->type);
		}
	}
#endif

	doc = odt_new();

	list_foreach (head, toks) {
		if (toks->type == BODY_BOX) {
			if ((toks = list_get_next(toks))->type != '{')
				// TODO:
				// error
				printf("error\n");

			list_foreach (toks, toks) {
				if (toks->type == '}')
					break;
				else if (toks->type == TEXT)
					odt_set_text(doc, toks->value);
				else if (toks->type == VARIABLE) {
					if (!dhat_get(vars, toks->value, (const void**)&text))
						// TODO: error var not found
						text = L"";

					odt_set_text(doc, text);
				}
				else
					// TODO: error: unknown token
					;
			}
		}
		else if (toks->type == INCLUDE_BOX)
			// TODO: write
			;
		else if (toks->type == SETTINGS_BOX)
			// TODO: write
			;
		else if (toks->type == VARIABLE_BOX) {
			if ((toks = list_get_next(toks))->type != '{')
				// TODO:
				// error
				printf("error\n");

			// TODO: rewrite
			list_foreach (list_get_next(toks), toks) {
				if (toks->type != VARNAME)
					break;

				varname = toks->value;

				if ((toks = list_get_next(toks))->type != '=')
					break;

				if ((toks = list_get_next(toks))->type != VARVALUE)
					break;

				varval = toks->value;
				dhat_put(vars, varname, varval);
			}
		}
		else
			// TODO: unknown token
			;
	}

	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));

	// list_free(head);

	return 0;
}
