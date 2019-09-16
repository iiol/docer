#include <stdio.h>
#include <locale.h>

#include "macro.h"
#include "odt.h"
#include "parser.h"
#include "stream.h"
#include "dhat.h"


enum cont_type {
	TOK,
	BOX,
};

struct box_args {
	wchar_t *arg;

	struct list_node _list;
};

struct box {
	wchar_t *name;
	struct box_args *args;
	struct box_content *cont;
};

struct box_content {
	enum cont_type type;

	union {
		token *tok;
		struct box *box;
	};

	struct list_node _list;
};


void
print_indent(int n)
{
	for (; n != 0; --n)
		printf("  ");
}

void
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
			printf(".%ls\n", (wchar_t*)toks->value);
		else if (toks->type == STRING)
			printf("\"%ls\"\n", (wchar_t*)toks->value);
		else if (toks->type == TEXT)
			printf("TEXT: '%ls'\n", (wchar_t*)toks->value);
		else if (toks->type == VARIABLE)
			printf("VAR : '%ls'\n", (wchar_t*)toks->value);
		else
			printf("%c\n", toks->type);
	}
}

void
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
	}

	if (level != 0)
		--level;
}

struct box_args*
get_arguments(token **head)
{
	token *toks;
	size_t size;
	struct box_args *args= NULL;
	wchar_t *wcs = NULL;


	if ((*head)->type != '(')
		return NULL;

	args = list_init(args);
	args->arg = NULL;

	list_foreach (list_get_next(*head), toks) {
		if (toks->type == ')')
			break;
		else if (toks->type == STRING) {
			if (wcs) {
				size = (wcslen(wcs) + wcslen(toks->value) + 1) *
				    sizeof (wchar_t);
				wcs = xrealloc(wcs, size);
				wcscat(wcs, toks->value);
			}
			else {
				size = (wcslen(toks->value) + 1) * sizeof (wchar_t);
				wcs = xmalloc(size);
				wcscpy(wcs, toks->value);
			}
		}
		else if (toks->type == ',') {
			args->arg = wcs;
			args = list_alloc_at_end(args);
			args->arg = NULL;
		}
		else
			// TODO:
			printf("ERROR\n");
	}

	*head = toks;

	return args;
}

struct box_content*
parse_boxes(token **head_tok)
{
	token *toks;
	struct box_content *head_cont, *cont;
	wchar_t *wcs;


	head_cont = list_init(cont);
	cont->type = 0;
	cont->box = NULL;
	cont->tok = NULL;

	list_foreach (*head_tok, toks) {
		switch ((int)toks->type) {
		case BOXNAME:
			cont->type = BOX;
			cont->box = xmalloc(sizeof (struct box));
			cont->box->args = NULL;
			cont->box->cont = NULL;

			cont->box->name = toks->value;
			toks = list_get_next(toks);

			if (toks->type == '(')
				cont->box->args = get_arguments(&toks);

			if (toks->type == '{') {
				toks = list_get_next(toks);
				cont->box->cont = parse_boxes(&toks);
			}

			cont = list_alloc_at_end(cont);
			cont->type = 0;
			cont->box = NULL;
			cont->tok = NULL;

			break;

		case TEXT:
		case VARIABLE: {
			token *tok;

			if (cont->tok)
				tok = list_alloc_at_end(cont->tok);
			else
				tok = list_init(cont->tok);

			cont->type = TOK;
			tok->type = toks->type;
			tok->value = toks->value;
			tok->offset = toks->offset;

			cont = list_alloc_at_end(cont);
			cont->type = 0;
			cont->box = NULL;
			cont->tok = NULL;

			break;
		}

		case '}':
			goto exit;

		default:
			printf("ERROR\n");

			break;
		}

		if (toks == NULL)
			break;
	}

exit:
	list_delete(cont);
	*head_tok = toks;

	return head_cont;
}

int
main(void)
{
	FILE *fp;
	odt_doc *doc;
	token *head_toks, *toks;
	struct box_content *cont;


	setlocale(LC_ALL, "");
	fp = fopen("test.txt", "r");
	head_toks = toks = parse_init(fp);
	doc = odt_new();

#if 1	// DEBUG
	print_tok_debug(toks);
	cont = parse_boxes(&toks);
	putchar('\n');
	print_box_debug(cont);
#endif

	if (odt_write(doc, "file.odt") == -1)
		printf("error: %s\n", odt_strerror(doc));

	odt_free(doc);
	tok_free(head_toks);
	stream_free();

	return 0;
}
