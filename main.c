#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
	LAT_CNT = 321,
	LON_CNT = 361,
	TOTAL_CNT = LAT_CNT * LON_CNT,
	LAT_RES = 161,
	LON_RES = 181,
	TOTAL_RES = LAT_RES * LON_RES,
};

static inline int
read4(const uint8_t *buf, int pos)
{
	uint8_t b3 = buf[pos];
	uint8_t b2 = buf[pos + 1];
	uint8_t b1 = buf[pos + 2];
	uint8_t b0 = buf[pos + 3];
	return (((((b3 << 8) + b2) << 8) + b1) << 8) + b0;
}

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

static inline int
read1(const uint8_t *buf, int pos)
{
	return buf[pos];
}

int
read_e(const uint8_t *buf, int pos)
{
	uint8_t b0 = buf[pos + 1];
	uint8_t b1 = buf[pos];
	int e = ((b1 & 0x7f) << 8) + b0;
	if ((b1 & 0x80) != 0)
		e = -e;
	return e;
}

double
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

int
read_n(const uint8_t *buf, int pos)
{
	uint8_t b0 = buf[pos];
	return b0 / 8;
}

float
read_y(const uint8_t *buf, int pos, double r, int e)
{
	float x = read2(buf, pos);
	float y = r + (e < 0 ? x / (1 << -e) : x * (1 << e));
	return y;
}

int
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

int
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
main(int argc, const char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Please provide input and output files.\n");
		return -1;
	}
	FILE *fin = fopen(argv[1], "rb");
	if (fin == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}

	FILE *fout = fopen(argv[2], "wb");
	if (fout == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}

	int size = get_size(fin);

	uint8_t *buf = malloc(size);
	if (buf == NULL) {
		fprintf(stderr, "Malloc failed: %s\n", "buf");
		return -1;
	}
	int res_size = TOTAL_RES * sizeof(float);
	float *res = malloc(res_size);
	if (res == NULL) {
		fprintf(stderr, "Malloc failed: %s\n", "res");
		return -1;
	}
	fseek(fin, 0, SEEK_SET);
	while (fread(buf, 1, size, fin) == size) {
		read_data(buf, res);
		fwrite(res, 1, res_size, fout);
	}

	free(buf);
	free(res);
	fclose(fin);
	fclose(fout);
	return 0;
}
