#include <string.h>
#include <stdio.h>

#include "main.h"

enum mode {
	NOTHING,
	CONVERTER,
};

int
main(int argc, const char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Too little arguments.\n");
		return -1;
	}
	enum mode mode = NOTHING;
	const char *fin = NULL;
	const char *fout = NULL;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			fprintf(stderr, "Wrong argument: %s.\n", argv[i]);
			return -1;
		}
		switch (argv[i][1]) {
		case 'c':
			if (i + 1 >= argc) {
				fprintf(stderr, "Input file name missing.\n");
				return -1;
			}
			++i;
			fin = argv[i];
			if (mode != NOTHING) {
				fprintf(stderr, "Too many modes.\n");
				return -1;
			}
			mode = CONVERTER;
			break;
		case 'o':
			if (i + 1 >= argc) {
				fprintf(stderr, "Output file name missing.\n");
				return -1;
			}
			++i;
			fout = argv[i];
			break;
		default:
			fprintf(stderr, "Wrong option.\n");
			return -1;
		}
	}

	switch (mode) {
	case CONVERTER:
		if (fin == NULL) {
			fprintf(stderr, "Input file name missing.\n");
			return -1;
		}
		if (fout == NULL)
			fout = "converter_output";
		return converter(fin, fout);
	default:
		fprintf(stderr, "Mode does not defined.\n");
		return -1;
	}
	return 0;
}
