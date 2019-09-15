#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

#include "dhat.h"
#include "macro.h"

static struct entry*
ealloc(void)
{
	struct entry *tmp;

	tmp = xmalloc(sizeof (struct entry));
	memset(tmp, 0, sizeof (struct entry));

	return tmp;
}

//djb2 hash
static unsigned int
gethash_djb2(const wchar_t *s, unsigned int size)
{
	unsigned int i;
	unsigned int hash = 5381;

	for (i = 0; s[i] != '\0'; ++i)
		hash += (hash << 5) + s[i];

	return hash % size;
}

static unsigned int
gethash(dhat *ht, const wchar_t *s, unsigned int size)
{
	if (ht->gethash_external != NULL)
		return ht->gethash_external(s, size);
	else
		return gethash_djb2(s, size);
}

static dhat*
dhat_resize(dhat *ht)
{
	int i;
	struct entry *head;
	dhat *newht;

	newht = dhat_new(ht->factor * ht->size, ht->maxdepth, ht->factor);

	for (i = 0; i < ht->size; ++i) {
		head = ht->entry[i];

		for (; head != NULL; head = head->next)
			newht = dhat_put(newht, head->key, head->value);
	}

	dhat_free(ht);

	return newht;
}

dhat*
dhat_new(unsigned int size, int depth, int factor)
{
	dhat *ht;

	ht = xmalloc(sizeof(dhat));
	memset(ht, 0, sizeof(dhat));
	ht->size = size;
	ht->maxdepth = depth;
	ht->factor = factor;

	size *= sizeof (struct entry*);
	ht->entry = xmalloc(size);
	memset(ht->entry, 0, size);

	return ht;
}

void
dhat_hashfunc(dhat *ht, unsigned int (*gethash)(const wchar_t *s, unsigned int size))
{
	ht->gethash_external = gethash;
}

dhat*
dhat_put(dhat *ht, const wchar_t *key, const void *value)
{
	int i;
	struct entry *head;
	unsigned int hash;

	hash = gethash(ht, key, ht->size);
	head = ht->entry[hash];

	for (i = 0;; head = head->next, ++i) {
		if (head == NULL) {
			head = ht->entry[hash] = ealloc();
			++i;
			break;
		}
		else if (wcscmp(head->key, key) == 0)
			break;
		else if (head->next == NULL) {
			head->next = ealloc();
			head = head->next;
			++i;
			break;
		}
	}

	head->key = xrealloc(head->key, (wcslen(key) + 1) * sizeof (wchar_t));
	wcscpy(head->key, key);
	head->value = value;

	if (ht->maxdepth < i)
		ht = dhat_resize(ht);

	return ht;
}

int
dhat_get(dhat *ht, const wchar_t *key, const void **value)
{
	struct entry *head;

	head = *(ht->entry + gethash(ht, key, ht->size));

	for (; head != NULL; head = head->next) {
		if (wcscmp(head->key, key) == 0) {
			*value = head->value;

			return 1;
		}
	}

	return 0;
}

void
dhat_remove(dhat *ht, const wchar_t *key)
{
	unsigned int hash;
	struct entry *head, **phead;

	hash = gethash(ht, key, ht->size);
	head = ht->entry[hash];
	phead = ht->entry + hash;

	if (head == NULL)
		return;

	while (1) {
		if (wcscmp(key, head->key) == 0) {
			*phead = head->next;
			free(head->key);
			free(head);
			break;
		}

		if (head->next == NULL)
			break;

		phead = &head->next;
		head = head->next;
	}
}

void
dhat_free(dhat *ht)
{
	int i;
	struct entry *head, *next;

	for (i = 0; i < ht->size; ++i) {
		head = ht->entry[i];

		for (; head != NULL; head = next) {
			next = head->next;
			dhat_remove(ht, head->key);
		}
	}

	free(ht->entry);
	free(ht);
}
