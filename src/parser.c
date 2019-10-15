#include <wchar.h>

#include "parser.h"
#include "macro.h"


struct styles {
	wchar_t *name;
	int id;

	struct list_node _list;
} *styles;

struct variables {
	wchar_t *var;
	wchar_t *val;

	struct list_node _list;
} *vars;

static struct styles*
style_search(wchar_t *name)
{
	struct styles *style;


	list_foreach(styles, style)
		if (!wcscmp(style->name, name))
			break;

	return style;
}

static void
set_var(wchar_t *var, wchar_t *val)
{
	struct variables *variable;


	if (vars)
		variable = list_alloc_at_end(vars);
	else
		variable = list_init(vars);

	variable->var = var;
	variable->val = val;
}

static wchar_t*
find_var(wchar_t *var)
{
	struct variables *variable;


	list_foreach (vars, variable)
		if (!wcscmp(variable->var, var))
			return variable->val;

	return NULL;
}

static void
parse_pp(odt_doc *doc, wchar_t *stylename, struct box_content *cont)
{
	struct styles *style;
	wchar_t *text = NULL;
	wchar_t *wcs;
	size_t size;


	list_foreach (cont, cont) {
		if (cont->type != TOK)
			continue;

		if (cont->tok->type == TEXT)
			wcs = cont->tok->wcs;
		else if (cont->tok->type == VARIABLE)
			wcs = find_var(cont->tok->wcs);
		else
			continue;

		if (text) {
			size = (wcslen(text) + wcslen(wcs) + 1) * sizeof (wchar_t);
			text = xrealloc(text, size);
		}
		else {
			size = (wcslen(wcs) + 1) * sizeof (wchar_t);
			text = xmalloc(size);
			text[0] = '\0';
		}

		wcscat(text, wcs);
	}

	if ((style = style_search(stylename)) == NULL)
		// TODO: set default style
		;

	odt_set_text(doc, text, style->id);
}

static void
parse_body(odt_doc *doc, struct box_content *cont)
{
	list_foreach (cont, cont) {
		if (cont->type == BOX) {
			if (!wcscmp(cont->box->name, L"pp"))
				parse_pp(doc, cont->box->args->arg, cont->box->cont);
		}
		else if (cont->type == TOK) {
			if (cont->tok->type != TEXT)
				continue;
		}
	}
}

static void
parse_style(odt_doc *doc, wchar_t *stylename, struct box_content *cont)
{
	int i;
	wchar_t *var, *val;
	struct box_args *arg;
	struct styles *style;
	odt_tstyle tstyle;


	list_foreach (cont, cont) {
		if (cont->type != BOX || wcscmp(cont->box->name, L"var") != 0)
			// ERROR
			continue;

		i = 0;
		list_foreach(cont->box->args, arg) {
			if (++i > 2)
				// ERROR: too many arguments
				break;

			if (i == 1)
				var = arg->arg;
			else if (i == 2)
				val = arg->arg;
		}

		if (i < 2) {
			// ERROR: too few arguments
			continue;
		}

		if (!wcscmp(var, L"color"))
			tstyle.color = wcstol(val, NULL, 16);
		else if (!wcscmp(var, L"bgcolor"))
			tstyle.bgcolor = wcstol(val, NULL, 16);
	}

	if (styles)
		style = list_alloc_at_end(styles);
	else
		style = list_init(styles);

	tstyle.type = PARAGRAPH;
	style->name = stylename;
	style->id = odt_init_text_style(doc, &tstyle);
}

void
parse_generate(odt_doc *doc, struct box_content *cont)
{
	list_foreach (cont, cont) {
		if (cont->type == BOX) {
			if (wcscmp(cont->box->name, L"body") == 0) {
				// TODO: check args->arg
				parse_body(doc, cont->box->cont);
			}
			else if (wcscmp(cont->box->name, L"var") == 0) {
				// TODO: check args->arg
				set_var(cont->box->args->arg, list_get_next(cont->box->args)->arg);
			}
			else if (wcscmp(cont->box->name, L"style") == 0)
				// TODO: check args->arg
				parse_style(doc, cont->box->args->arg, cont->box->cont);
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
	struct box_args *head_args = NULL, *args;
	wchar_t *wcs = NULL;


	if ((*head)->type != '(')
		return NULL;

	list_foreach (list_get_next(*head), toks) {
		if (toks->type == STRING) {
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
		else if (toks->type == ',' || toks->type == ')') {
			if (head_args)
				args = list_alloc_at_end(args);
			else
				head_args = list_init(args);

			args->arg = wcs;
			wcs = NULL;

			if (toks->type == ')')
				break;
		}
		else
			// TODO:
			printf("ERROR\n");
	}

	*head = toks;

	return head_args;
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
