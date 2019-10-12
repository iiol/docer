#include <wchar.h>

#include "parser.h"
#include "macro.h"


static void
parse_body(struct box_content *cont)
{
	list_foreach (cont, cont) {
		if (cont->type == BOX) {
			if (wcscmp(cont->box->name, L"pp") == 0) {
			}
		}
		else if (cont->type == TOK) {
		}
	}
}

static void
parse_style(struct box_content *cont)
{
	list_foreach (cont, cont) {
		if (cont->type == BOX && !wcscmp(cont->box->name, L"var"))
			;
	}
}

void
parse_generate(odt_doc *doc, struct box_content *cont)
{
	odt_tstyle style;
	unsigned int def_style;


	style.type = PARAGRAPH;
	style.bgcolor = 0xFFFF00;
	style.color = 0xFF0000;
	def_style = odt_init_text_style(doc, &style);
	odt_set_text(doc, L"Hello World!", def_style);

	style.type = PARAGRAPH;
	style.bgcolor = 0xFF0000;
	style.color = 0xFFFF00;
	def_style = odt_init_text_style(doc, &style);
	odt_set_text(doc, L"How are you?", def_style);

	list_foreach (cont, cont) {
		if (cont->type == BOX) {
			if (wcscmp(cont->box->name, L"body") == 0)
				parse_body(cont->box->cont);
			else if (wcscmp(cont->box->name, L"var") == 0)
				;
			else if (wcscmp(cont->box->name, L"style") == 0)
				parse_style(cont->box->cont);
		}
		else if (cont->type == TOK) {
		}
	}
}

static struct box_args*
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
				size = (wcslen(wcs) + wcslen(toks->wcs) + 1) *
				    sizeof (wchar_t);
				wcs = xrealloc(wcs, size);
				wcscat(wcs, toks->wcs);
			}
			else {
				size = (wcslen(toks->wcs) + 1) * sizeof (wchar_t);
				wcs = xmalloc(size);
				wcscpy(wcs, toks->wcs);
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

static struct box_content*
parse_box(token **head_tok)
{
	token *toks;
	struct box_content *head_cont, *cont;


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

			cont->box->name = toks->wcs;
			toks = list_get_next(toks);

			if (toks->type == '(') {
				cont->box->args = get_arguments(&toks);
				toks = list_get_next(toks);
			}

			if (toks->type == '{') {
				toks = list_get_next(toks);
				cont->box->cont = parse_box(&toks);
			}
			else
				toks = list_get_prev(toks);

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
			tok->wcs = toks->wcs;
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

struct box_content*
parse_init(token *head)
{
	return parse_box(&head);
}
