#include <stdbool.h>

enum {
	LAT_RES = 161,
	LON_RES = 181,
	TOTAL_RES = LAT_RES * LON_RES,
};

bool mask[TOTAL_RES];

static inline bool
is_equal(float x, float y)
{
	return (int)(x * 100) == (int)(y * 100);
}

int
converter(const char *in);

int
normalizer(const char *in, const char *out, bool is_minmax);

int
coefficients(void);
