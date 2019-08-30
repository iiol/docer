#ifndef _MACRO_H
#define _MACRO_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


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

#endif // _MACRO_H
