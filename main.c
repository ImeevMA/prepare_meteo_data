#include "main.h"
#include <stdio.h>

int
main(int argc, const char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Please provide input and output files.\n");
		return -1;
	}
	return converter(argv[1], argv[2]);
}
