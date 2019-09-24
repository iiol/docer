#include <wchar.h>

#include "parser.h"
#include "macro.h"


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
parse_init(token **head_tok)
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
				cont->box->cont = parse_init(&toks);
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
