#include <stdbool.h>

static inline bool
is_equal(float x, float y)
{
	return (int)(x * 100) == (int)(y * 100);
}

int
converter(const char *in);

int
normalizer(const char *in, const char *out);

int
coefficients(void);
