#ifndef _DHAT_H
#define _DHAT_H

#include <wchar.h>

struct entry {
	wchar_t *key;
	const void *value;
	struct entry *next;
};

typedef struct dhat {
	unsigned int size;
	int maxdepth;
	int factor;
	struct entry **entry;

	unsigned int (*gethash_external)(const wchar_t *s, unsigned int size);
} dhat;

dhat *dhat_new(unsigned int size, int depth, int factor);
void dhat_hashfunc(dhat *ht, unsigned int (*gethash)(const wchar_t *s, unsigned int size));
dhat *dhat_put(dhat *ht, const wchar_t *key, const void *value);
int dhat_get(dhat *ht, const wchar_t *key, const void **value);
void dhat_remove(dhat *ht, const wchar_t *key);
void dhat_free(dhat *ht);

#endif //_DHAT_N
