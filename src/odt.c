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


struct odt_version {
	int version_maj;
	int version_min;
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


static const char*
whitespace_cb(mxml_node_t *node, int where)
{
	const char *element;


	element = mxmlGetElement(node);

	switch (where) {
	case MXML_WS_BEFORE_OPEN:
		if (!strncmp(element, "?xml", strlen("?xml")))
			return "";
		else if (!strncmp(element, "manifest:file-entry", strlen("manifest:file-entry")))
			return "\n\t";
		else
			return "\n";

		break;

	case MXML_WS_AFTER_OPEN:
		break;

	case MXML_WS_BEFORE_CLOSE:
		return "\n";
		break;

	case MXML_WS_AFTER_CLOSE:
		break;
	}

	return "";
}

struct odt_doc*
odt_new(void)
{
	struct odt_doc *doc;


	doc = xmalloc(sizeof (struct odt_doc));
	doc->meta = xmalloc(sizeof (struct odt_meta));
	doc->styles = xmalloc(sizeof (struct odt_styles));

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

	fd = open("./src/templates/styles.xml", O_RDONLY);
	xml = mxmlLoadFd(NULL, fd, MXML_NO_CALLBACK);
	close(fd);

	fd = open("/tmp", O_RDWR | O_TMPFILE, 0644);
	mxmlSaveFd(xml, fd, whitespace_cb);

	return fd;
}

static int
create_content(struct odt_doc *doc)
{
	FILE *fp;
	int fd;
	mxml_node_t *xml;


	fp = fopen("./src/templates/content.xml", "r");
	xml = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);

	fd = open("/tmp", O_RDWR | O_TMPFILE, 0644);
	mxmlSaveFd(xml, fd, whitespace_cb);

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
