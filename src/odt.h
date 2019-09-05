#ifndef _ODT_H
#define _ODT_H

#include <wchar.h>

enum odt_errors {
	ODT_SUCCESS,
	ODT_EEXIST,
	ODT_EOPEN,
	ODT_EZIP,
};

typedef struct odt_doc odt_doc;

void odt_set_text(odt_doc *doc, wchar_t *text);
struct odt_doc *odt_new(void);
int odt_write(struct odt_doc *doc, const char *path);
const char *odt_strerror(struct odt_doc *doc);

#endif // _ODT_H
