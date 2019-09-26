#ifndef _ODT_H
#define _ODT_H

#include <stdint.h>
#include <wchar.h>

enum odt_errors {
	ODT_SUCCESS,
	ODT_EEXIST,
	ODT_EOPEN,
	ODT_EZIP,
};

enum text_type {
	PARAGRAPH,
	SIMPLE_TEXT,
};

typedef struct odt_text_style {
	enum text_type type;
	uint32_t color;
	uint32_t bgcolor;
} odt_tstyle;

typedef struct odt_doc odt_doc;


void odt_set_text(odt_doc *doc, wchar_t *text, unsigned int snum);
unsigned int odt_init_text_style(odt_doc *doc, odt_tstyle *style);

struct odt_doc *odt_new(void);
int odt_write(struct odt_doc *doc, const char *path);
const char *odt_strerror(struct odt_doc *doc);
void odt_free(struct odt_doc *doc);

#endif // _ODT_H
