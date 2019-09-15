#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <mxml.h>
#include <zip.h>

#include "odt.h"
#include "macro.h"


#define MIMETYPE "application/vnd.oasis.opendocument.text"


enum content_type {
	TEXT,
	IMAGE,
	TABLE,
};

struct odt_version {
	int version_maj;
	int version_min;
};

struct content_list {
	enum content_type type;
	union {
		wchar_t *text;
		// image object;
		// table object;
	};

	struct list_node _list;
};

struct odt_manifest {
	int fd;

	const struct odt_version *version;
};

struct odt_meta {
	int fd;

	const struct odt_version *version;
	/*
	time_t ctime;
	time_t mtime;
	*/
	int edit_cycles;
	char *generator;
};

struct odt_styles {
	int fd;

	int integer;
};

struct odt_content {
	int fd;

	struct content_list *list;
};

struct odt_doc {
	enum odt_errors errnum;

	struct odt_meta *meta;
	struct odt_styles *styles;
	struct odt_content *content;
	char *mimetype;
};


static const struct odt_version version = {
	.version_maj = 1,
	.version_min = 2,
};


void
odt_set_text(odt_doc *doc, wchar_t *text)
{
	struct content_list *entry;


	if (doc->content->list == NULL)
		entry = list_init(doc->content->list);
	else
		entry = list_alloc_at_end(doc->content->list);

	entry->type = TEXT;
	entry->text = text;
}

static const char*
whitespace_cb(mxml_node_t *node, int where)
{
	const char *element;


	element = mxmlGetElement(node);

	switch (where) {
	case MXML_WS_BEFORE_OPEN:
	case MXML_WS_BEFORE_CLOSE:
		if (!strncmp(element, "?xml", strlen("?xml")))
			return "";
		else
			return "\n";

		break;

	case MXML_WS_AFTER_OPEN:
	case MXML_WS_AFTER_CLOSE:
		break;
	}

	return "";
}

struct odt_doc*
odt_new(void)
{
	int size;
	struct odt_doc *doc;


	doc = xmalloc(sizeof (struct odt_doc));

	size = sizeof (struct odt_meta);
	doc->meta = xmalloc(size);
	memset(doc->meta, 0, size);

	size = sizeof (struct odt_styles);
	doc->styles = xmalloc(size);
	memset(doc->styles, 0, size);

	size = sizeof (struct odt_content);
	doc->content = xmalloc(size);
	memset(doc->content, 0, size);

	doc->mimetype = MIMETYPE;

	return doc;
}

static int
create_mimetype(void)
{
	int fd;


	fd = open("/tmp", O_RDWR | O_TMPFILE, 0644);
	write(fd, MIMETYPE, strlen(MIMETYPE));

	return fd;
}

static int
create_styles(struct odt_doc *doc)
{
	int fd;
	mxml_node_t *xml;


	// TODO: change xml initialization method
	fd = open("./templates/styles.xml", O_RDONLY);
	xml = mxmlLoadFd(NULL, fd, MXML_NO_CALLBACK);
	close(fd);

	fd = open("/tmp", O_RDWR | O_TMPFILE, 0644);
	mxmlSaveFd(xml, fd, whitespace_cb);
	mxmlDelete(xml);

	return fd;
}

static int
create_content(struct odt_doc *doc)
{
	int fd;
	size_t size;
	char *str;
	mxml_node_t *xml, *node;
	struct content_list *list;


	// TODO: change xml initialization method
	fd = open("./templates/content.xml", O_RDONLY);
	xml = mxmlLoadFd(NULL, fd, MXML_NO_CALLBACK);
	node = mxmlFindPath(xml, "office:document-content/office:body/office:text/text:p");
	close(fd);

	list_foreach (doc->content->list, list) {
		if (list->type != TEXT)
			continue;

		size = (wcslen(list->text) + 1) * MB_CUR_MAX;
		str = xmalloc(size);
		size = wcstombs(str, list->text, size);
		mxmlNewText(node, 0, str);

		free(str);
	}

	fd = open("/tmp", O_RDWR | O_TMPFILE, 0644);
	mxmlSaveFd(xml, fd, whitespace_cb);
	mxmlDelete(xml);

	return fd;
}

int
odt_write(struct odt_doc *doc, const char *path)
{
	int fd;
	FILE *fp;
	int err;
	zip_uint64_t idx;
	zip_t *zip;
	zip_source_t *zip_src;


	assert(path);

	if (access(path, F_OK) == 0) {
		doc->errnum = ODT_EEXIST;
		return -1;
	}

	if ((zip = zip_open(path, ZIP_CREATE, &err)) == NULL) {
		printf("error open archive: %s\n", strerror(err));
		doc->errnum = ODT_EOPEN;
		return -1;
	}

	// mimetype
	fd = create_mimetype();
	lseek(fd, 0, SEEK_SET);
	fp = fdopen(fd, "rw");
	zip_src = zip_source_filep(zip, fp, 0, 0);
	idx = zip_file_add(zip, "mimetype", zip_src, ZIP_FL_ENC_UTF_8);
	zip_set_file_compression(zip, idx, ZIP_CM_DEFAULT, 0);

	// styles
	fd = create_styles(doc);
	lseek(fd, 0, SEEK_SET);
	fp = fdopen(fd, "rw");
	zip_src = zip_source_filep(zip, fp, 0, 0);
	idx = zip_file_add(zip, "styles.xml", zip_src, ZIP_FL_ENC_UTF_8);
	zip_set_file_compression(zip, idx, ZIP_CM_DEFAULT, 0);

	// content
	fd = create_content(doc);
	lseek(fd, 0, SEEK_SET);
	fp = fdopen(fd, "rw");
	zip_src = zip_source_filep(zip, fp, 0, 0);
	idx = zip_file_add(zip, "content.xml", zip_src, ZIP_FL_ENC_UTF_8);
	zip_set_file_compression(zip, idx, ZIP_CM_DEFAULT, 0);

	if (zip_close(zip) == -1)
		printf("err: %s\n", zip_strerror(zip));

	return 0;
}

#define MSGLEN 1024
const char*
odt_strerror(struct odt_doc *doc)
{
	static char errmsg[MSGLEN];


	switch (doc->errnum) {
	case ODT_SUCCESS:
		strncpy(errmsg, "Success", MSGLEN);
		break;

	case ODT_EEXIST:
		strncpy(errmsg, "File exist", MSGLEN);
		break;

	case ODT_EOPEN:
		strncpy(errmsg, "Open error", MSGLEN);
		break;

	case ODT_EZIP:
		strncpy(errmsg, "Zip file error", MSGLEN);
		break;
	}

	return errmsg;
}
#undef MSGLEN

void
odt_free(struct odt_doc *doc)
{
	struct content_list *entry;


	for (entry = doc->content->list; entry != NULL;)
		entry = list_delete(entry);

	free(doc->meta);
	free(doc->styles);
	free(doc->content);
	free(doc);
}
