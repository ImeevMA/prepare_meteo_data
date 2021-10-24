#include <string.h>
#include <stdio.h>

#include "main.h"

enum mode {
	CONVERTER	= 0,
	NORMALIZER	= 1,
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
	return 0;
}
