#ifndef _MACRO_H
#define _MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>


#define SYSCALL(estat, func, args...)					\
({									\
	int ret;							\
									\
									\
	while ((ret = ((func)(args))) == -1 && errno == EINTR)		\
		;							\
									\
	if (ret == -1) {						\
		perror(#func "(" #args ")");				\
		exit(estat);						\
	}								\
									\
	ret;								\
})

static inline void*
xmalloc(size_t size)
{
	void *ret;

	errno = 0;
	ret = malloc(size);

	if (size != 0 && ret == NULL) {
		perror("malloc()");
		exit(EXIT_FAILURE);
	}

	return ret;
}

static inline void*
xrealloc(void *ptr, size_t size)
{
	void *ret;

	ret = realloc(ptr, size);

	if (size != 0 && ret == NULL) {
		perror("realloc()");
		exit(EXIT_FAILURE);
	}

	return ret;
}

#define xassert(var)							\
({									\
	typeof (var) _var = var;					\
									\
									\
	if (!_var) {							\
		fprintf(stderr, "%s:%d: %s: Assertion `%s' failed.\n",	\
			__FILE__, __LINE__, __func__, #var);		\
		fflush(stderr);						\
		exit(1);						\
	}								\
									\
	_var;								\
})

#define list_init(p)							\
({									\
	typeof (p) _p;							\
									\
									\
	_p = malloc(sizeof (typeof (*_p)));				\
	memset(&_p->_list, 0, sizeof (struct list_node));		\
									\
	_p->_list.meta = malloc(sizeof (struct list_meta));		\
	_p->_list.meta->offset = offsetof(typeof (*_p), _list);		\
	_p->_list.meta->head = &_p->_list;				\
	_p->_list.meta->tail = &_p->_list;				\
	_p->_list.prev = NULL;						\
	_p->_list.next = NULL;						\
									\
	p = _p;								\
})

#define list_alloc_at_end(p)						\
({									\
	typeof (p) newentry, _p = xassert(p);				\
									\
									\
	newentry = malloc(sizeof (typeof (*_p)));			\
	newentry->_list.meta = _p->_list.meta;				\
	newentry->_list.prev = _p->_list.meta->tail;			\
	newentry->_list.next = NULL;					\
	newentry->_list.meta->tail = &(newentry->_list);		\
	if (newentry->_list.prev != NULL)				\
		newentry->_list.prev->next = &(newentry->_list);	\
									\
	newentry;							\
})

#define list_delete(p)							\
({									\
	typeof (p) head = NULL, _p = xassert(p);			\
									\
									\
	if (_p->_list.prev == NULL)					\
		_p->_list.meta->head = _p->_list.next;			\
	else								\
		_p->_list.prev->next = _p->_list.next;			\
									\
	if (_p->_list.next == NULL)					\
		_p->_list.meta->tail = _p->_list.prev;			\
	else								\
		_p->_list.next->prev = _p->_list.prev;			\
									\
	if (_p->_list.meta->head != NULL)				\
		head = (typeof (_p)) ((int8_t*)_p->_list.meta->head - _p->_list.meta->offset);	\
									\
	if (_p->_list.prev == NULL && _p->_list.next == NULL)		\
		free(_p->_list.meta);					\
									\
	free(_p);							\
									\
	head;								\
})

#define list_get_prev(p)						\
({									\
	typeof (p) ret = NULL, _p = xassert(p);				\
									\
									\
	if (_p->_list.prev != NULL)					\
		ret = (void*)((int8_t*)_p->_list.prev - _p->_list.meta->offset); \
									\
	ret;								\
})

#define list_get_next(p)						\
({									\
	typeof (p) ret = NULL, _p = xassert(p);				\
									\
									\
	if (_p->_list.next != NULL)					\
		ret = (void*)((int8_t*)_p->_list.next - _p->_list.meta->offset); \
									\
	ret;								\
})

#define list_get_head(p)						\
({									\
	typeof (p) ret = NULL, _p = xassert(p);				\
									\
									\
	if (_p->_list.meta->head != NULL)				\
		ret = (void*)((int8_t*)_p->_list.meta->head - _p->_list.meta->offset);	\
									\
	ret;								\
})

#define list_get_tail(p)						\
({									\
	typeof (p) ret = NULL, _p = xassert(p);				\
									\
									\
	if (_p->_list.meta->tail != NULL)				\
		ret = (void*)((int8_t*)_p->_list.meta->tail - _p->_list.meta->offset);	\
									\
	ret;								\
})

#define list_foreach(head, entry) for (entry = head; entry != NULL; entry = list_get_next(entry))

struct list_meta {
	struct list_node *head, *tail;
	int offset;
};

struct list_node {
	struct list_meta *meta;
	struct list_node *prev, *next;
};

#endif // _MACRO_H
