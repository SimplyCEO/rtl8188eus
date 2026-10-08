#if !defined(ISTRING_H)
# define ISTRING_H

# include <linux/types.h>

static inline char* __istrncpy(char* sl, const char* s2, size_t n)
{
	size_t i = 0;

	for (; ((i < n) && (s2[i] != '\0')); ++i)
	{
		sl[i] = s2[i];
	}

	for (; i<n; ++i)
	{
		sl[i] = '\0';
	}

	return sl;
}

# if defined(strncpy)
#   undef strncpy
# endif

# define strncpy __istrncpy

#endif

