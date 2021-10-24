#include <string.h>
#include <stdio.h>

#include "main.h"

enum mode {
	CONVERTER	= 0,
	NORMALIZER	= 1,
	MINMAX		= 1 << 1,
	COEFFICIENTS	= 1 << 2,
};

int
main(int argc, const char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Too little arguments.\n");
		return -1;
	}
	enum mode mode = CONVERTER;
	const char *fin = NULL;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			if (fin == NULL) {
				fin = argv[i];
				continue;
			}
			fprintf(stderr, "Wrong argument: %s.\n", argv[i]);
			return -1;
		}

		switch (argv[i][1]) {
		case 'n':
			mode |= NORMALIZER;
			break;
		case 'm':
			if ((mode & NORMALIZER) != 0) {
				fprintf(stderr, "Cannot execute -n and -m at "
						"the same time.\n");
				return -1;
			}
			mode |= MINMAX;
			break;
		case 'c':
			mode |= COEFFICIENTS;
			break;
		default:
			fprintf(stderr, "Wrong option.\n");
			return -1;
		}
	}
	if (fin == NULL) {
		fprintf(stderr, "Input file name is required.\n");
		return -1;
	}
	if (converter(fin) != 0)
		return -1;
	if (mode == 0)
		return 0;

	FILE *fmask = fopen("mask", "rb");
	if (fmask == NULL ||
	    fread(mask, sizeof(bool), TOTAL_RES, fmask) != TOTAL_RES) {
		fprintf(stderr, "Wrong mask file.\n");
		return -1;
	}
	fclose(fmask);

	if ((mode & NORMALIZER) != 0 &&
	    normalizer("data", "data_norm", false) != 0)
		return -1;
	else if ((mode & MINMAX) != 0 &&
	    normalizer("data", "data_norm", true) != 0)
		return -1;
	if ((mode & COEFFICIENTS) != 0) {
		if (coefficients() != 0)
			return -1;
		if ((mode & NORMALIZER) != 0) {
			if (normalizer("data_a", "data_a_norm", false) != 0)
				return -1;
			if (normalizer("data_b", "data_b_norm", false) != 0)
				return -1;
		}
		if ((mode & MINMAX) != 0) {
			if (normalizer("data_a", "data_a_norm", true) != 0)
				return -1;
			if (normalizer("data_b", "data_b_norm", true) != 0)
				return -1;
		}
	}
	return 0;
}
