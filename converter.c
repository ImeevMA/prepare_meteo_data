#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

enum {
	LAT_CNT = 321,
	LON_CNT = 361,
	TOTAL_CNT = LAT_CNT * LON_CNT,
};

static inline int
read3(const uint8_t *buf, int pos)
{
	uint8_t b2 = buf[pos];
	uint8_t b1 = buf[pos + 1];
	uint8_t b0 = buf[pos + 2];
	return (((b2 << 8) + b1) << 8) + b0;
}

static inline int
read2(const uint8_t *buf, int pos)
{
	uint8_t b1 = buf[pos];
	uint8_t b0 = buf[pos + 1];
	return (b1 << 8) + b0;
}

static int
read_e(const uint8_t *buf, int pos)
{
	uint8_t b0 = buf[pos + 1];
	uint8_t b1 = buf[pos];
	int e = ((b1 & 0x7f) << 8) + b0;
	if ((b1 & 0x80) != 0)
		e = -e;
	return e;
}

static double
read_r(const uint8_t *buf, int pos)
{
	uint8_t b0 = buf[pos + 3];
	uint8_t b1 = buf[pos + 2];
	uint8_t b2 = buf[pos + 1];
	uint8_t b3 = buf[pos];
	int exp = (b3 & 0x7f) * 4 - 256 - 24;
	double r = ((((b2 << 8) + b1) << 8) + b0);
	if (exp < 0)
		r /= 1 << -exp;
	else
		r *= 1 << exp;
	if ((b3 & 0x80) != 0)
		r = -r;
	return r;
}

static int
read_n(const uint8_t *buf, int pos)
{
	uint8_t b0 = buf[pos];
	return b0 / 8;
}

static inline float
read_y(const uint8_t *buf, int pos, double r, int e)
{
	float x = read2(buf, pos);
	float y = r + (e < 0 ? x / (1 << -e) : x * (1 << e));
	return y;
}

static int
get_size(FILE *file)
{
	fseek(file, 4, SEEK_SET);
	uint8_t b0;
	uint8_t b1;
	uint8_t b2;
	fread(&b2, 1, 1, file);
	fread(&b1, 1, 1, file);
	fread(&b0, 1, 1, file);
	int size = ((((b2 << 8) + b1) << 8) + b0 + 0x60) & 0xfffffff8;
	return size;
}

static int
read_data(const uint8_t *buf, float *res)
{
	int size0 = 8;
	int size1 = read3(buf, size0) + size0;
	int size2 = read3(buf, size1) + size1;
	int size3 = 0 + size2;
	int size4 = read3(buf, size3) + size3;

	int e = read_e(buf, size3 + 4);
	float r = read_r(buf, size3 + 6);
	int n = read_n(buf, size3 + 10);

	int k = 0;
	int m = size3 + 11;
	for (int i = 0; i < LAT_RES; ++i) {
		for (int j = 0; j < LON_RES; ++j) {
			res[k++] = read_y(buf, m, r, e);
			m += 2 * n;
		}
		m += n * (LON_CNT - 1);
	}
}

int
converter(const char *in)
{
	FILE *fin = fopen(in, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}

	int size = get_size(fin);

	uint8_t *buf = malloc(size);
	if (buf == NULL) {
		fprintf(stderr, "Malloc failed: %s\n", "buf");
		return -1;
	}
	float *res = malloc(TOTAL_RES * sizeof(float));
	if (res == NULL) {
		fprintf(stderr, "Malloc failed: %s\n", "res");
		return -1;
	}

	FILE *fout = fopen("data", "wb");
	if (fout == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}

	float metadata[8];
	metadata[0] = LAT_RES;
	metadata[1] = LON_RES;
	metadata[2] = 80.0;
	metadata[3] = 0.0;
	metadata[4] = -0.5;
	metadata[5] = -90.0;
	metadata[6] = 0.0;
	metadata[7] = 0.5;
	fwrite(metadata, sizeof(float), 8, fout);

	fseek(fin, 0, SEEK_SET);
	while (fread(buf, 1, size, fin) == size) {
		read_data(buf, res);
		fwrite(res, sizeof(float), TOTAL_RES, fout);
	}

	free(buf);
	free(res);
	fclose(fin);
	fclose(fout);
	return 0;
}
