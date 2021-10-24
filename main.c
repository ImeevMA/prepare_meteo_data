#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum {
	LAT_RES = 161,
	LON_RES = 181,
	TOTAL_RES = LAT_RES * LON_RES,
	LAT_CNT = 321,
	LON_CNT = 361,
	TOTAL_CNT = LAT_CNT * LON_CNT,
};

static int stack[1024];
static bool mask[TOTAL_RES];

static inline bool
is_equal(float x, float y)
{
	return (int)(x * 100) == (int)(y * 100);
}

enum mode {
	CONVERTER	= 0,
	NORMALIZER	= 1,
	MINMAX		= 1 << 1,
	COEFFICIENTS	= 1 << 2,
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

int
get_coefs(const float *row1, const float *row2, float *a, float *b)
{
	bool local_mask[TOTAL_RES];
	memcpy(local_mask, mask, sizeof(bool) * TOTAL_RES);
	for (int i = 0; i < TOTAL_RES; ++i) {
		if (!local_mask[i])
			continue;
		stack[0] = i;
		float x = row1[i];
		float delta = row2[i] - x;
		int k = 1;
		float a0 = delta;
		float b0 = delta * delta;
		for (int j = i + 1; j < TOTAL_RES; ++j) {
			if (!local_mask[j] || !is_equal(x, row1[j]))
				continue;
			local_mask[i] = false;
			if (k >= 1024) {
				fprintf(stderr, "Temporary stack overflow.\n");
				return -1;
			}
			stack[k++] = j;
			delta = row2[j] - x;
			a0 += delta;
			b0 += delta * delta;
		}
		a0 /= k;
		b0 = sqrt(b0/k - a0 * a0);
		for (int j = 0; j < k; ++j) {
			a[stack[j]] = a0;
			b[stack[j]] = b0;
		}
	}
	return 0;
}

int
coefficients(void)
{
	FILE *fin = fopen("data", "rb");
	if (fin == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}
	int size = TOTAL_RES * sizeof(float);
	float metadata[8];
	float *row1 = malloc(size);
	float *row2 = malloc(size);
	float *a = calloc(1, size);
	float *b = calloc(1, size);
	if (fread(metadata, sizeof(float), 8, fin) != 8 ||
	    fread(row1, sizeof(float), TOTAL_RES, fin) != TOTAL_RES) {
		fprintf(stderr, "Wrong size of data.\n");
		return -1;
	};

	FILE *fa = fopen("data_a", "wb");
	if (fa == NULL) {
		fprintf(stderr, "Wrong output file for coefficient A.\n");
		return -1;
	}
	FILE *fb = fopen("data_b", "wb");
	if (fb == NULL) {
		fprintf(stderr, "Wrong output file for coefficient B.\n");
		return -1;
	}
	fwrite(metadata, sizeof(float), 8, fa);
	fwrite(metadata, sizeof(float), 8, fb);
	while (fread(row2, sizeof(float), TOTAL_RES, fin) == TOTAL_RES) {
		if (get_coefs(row1, row2, a, b) != 0)
			return -1;
		fwrite(a, 1, size, fa);
		fwrite(b, 1, size, fb);
		float *tmp = row1;
		row1 = row2;
		row2 = tmp;
	}
	free(b);
	free(a);
	free(row2);
	free(row1);
	fclose(fin);
	return 0;
}

static float
get_quantile(const float *data, int len, float min, float max, int k)
{
	while (max - min > 0.01) {
		int m = 0;
		float d = (max + min) / 2;
		for (int i = 0; i < len; ++i) {
			if (d > data[i])
				++m;
		}
		if (m < k)
			min = d;
		else
			max = d;
	}
	return (max + min) / 2;
}

static float
get_min_max_quantile(const float *data, int len, float min, float max,
		     bool is_min)
{
	int k = (len / 1000.0) * is_min ? 1 : 999;
	while (max - min > 0.01) {
		int m = 0;
		float d = (max + min) / 2;
		for (int i = 0; i < len; ++i) {
			if (d > data[i])
				++m;
		}
		if (m < k)
			min = d;
		else
			max = d;
	}
	return (max + min) / 2;
}

static float *
new_quantiles(const float *data, int len, int n)
{
	float *quantiles = malloc((n + 1) * sizeof(float));
	float min = data[0];
	float max = data[0];
	for (int i = 0; i < len; ++i) {
		if (min > data[i])
			min = data[i];
		if (max < data[i])
			max = data[i];
	}
	quantiles[0] = min;
	quantiles[n] = max;
	for (int i = 1; i < n; ++i) {
		float tmp_min = quantiles[i - 1];
		int k = (double)len / n * i;
		quantiles[i] = get_quantile(data, len, tmp_min, max, k);
	}
	return quantiles;
}

static float *
new_minmax_quantiles(const float *data, int len)
{
	float *quantiles = malloc(4 * sizeof(float));
	float min = data[0];
	float max = data[0];
	for (int i = 0; i < len; ++i) {
		if (min > data[i])
			min = data[i];
		if (max < data[i])
			max = data[i];
	}
	int min_limit = (double)len / 100;
	int max_limit = (double)len / 100 * 99;
	quantiles[0] = min;
	quantiles[1] = get_quantile(data, len, min, max, min_limit);
	quantiles[2] = get_quantile(data, len, min, max, max_limit);
	quantiles[3] = max;
	return quantiles;
}

static uint8_t *
new_data(const float *data, const float *qs, int n)
{
	uint8_t *res = malloc(TOTAL_RES);
	for (int i = 0; i < TOTAL_RES; ++i) {
		if (!mask[i]) {
			res[i] = 0;
			continue;
		}
		float d = data[i];
		res[i] = n;
		for (int j = 1; j < n; ++j) {
			if (d < qs[j]) {
				res[i] = j;
				break;
			}
		}
	}
	return res;
}

int
normalizer(const char *in, const char *out, bool is_minmax) {
	int masked_count = 0;
	for (int i = 0; i < TOTAL_RES; ++i)
		masked_count += mask[i];
	int masked_size = masked_count * sizeof(float);
	float *masked_data = malloc(masked_size);

	FILE *fin = fopen(in, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}
	int size = TOTAL_RES * sizeof(float);
	float *data = malloc(size);
	FILE *fout = fopen(out, "wb");
	fseek(fin, 8 * sizeof(float), SEEK_SET);
	while (fread(data, sizeof(float), TOTAL_RES, fin) == TOTAL_RES) {
		int j = 0;
		for (int i = 0; i < TOTAL_RES; ++i) {
			if (mask[i] != 0)
				masked_data[j++] = data[i];
		}

		float *quantiles;
		uint8_t *result;
		if (is_minmax) {
			quantiles = new_minmax_quantiles(masked_data,
							 masked_count);
			result = new_data(data, quantiles, 3);
		} else {
			quantiles = new_quantiles(masked_data, masked_count,
						  10);
			result = new_data(data, quantiles, 10);
		}

		fwrite(result, 1, TOTAL_RES, fout);
		free(quantiles);
		free(result);
	}
	free(data);
	fclose(fout);
	fclose(fin);
	return 0;
}

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
