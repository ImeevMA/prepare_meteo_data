#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"


static float
get_quantile(const float *data, int len, float min, float max, int i, int k,
	     const uint8_t *mask, int mask_len)
{
	while (max - min > 0.01) {
		int m = 0;
		float d = (max + min) / 2;
		for (int i = 0; i < len; ++i) {
			if (mask[i % mask_len] && d > data[i])
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
new_quantiles(const float *data, int len, const uint8_t *mask, int mask_len,
	      int n)
{
	float *quantiles = malloc((n + 1) * sizeof(float));
	float min = 0;
	float max = 0;
	for (int i = 0; i < len; ++i) {
		if (mask[i % mask_len] == 0)
			continue;
		if (min > data[i])
			min = data[i];
		if (max < data[i])
			max = data[i];
	}
	quantiles[0] = min;
	quantiles[n] = max;
	int m = 0;
	for (int i = 0; i < mask_len; ++i)
		m += mask[i];
	int k = (len / mask_len) * m / n;
	for (int i = 1; i < n; ++i) {
		float tmp_min = quantiles[i - 1];
		quantiles[i] = get_quantile(data, len, tmp_min, max, i, k * i, mask, mask_len);
	}
	return quantiles;
}

static uint8_t *
new_data(const float *data, int len, const uint8_t *mask, int mask_len,
	 const float *qs, int n)
{
	uint8_t *res = malloc(len);
	for (int i = 0; i < len; ++i) {
		if (mask[i % mask_len] == 0) {
			res[i] = 0;
			continue;
		}
		float d = data[i];
		if (d < qs[1])
			res[i] = 1;
		else if (d > qs[n - 1])
			res[i] = 3;
		else
			res[i] = 2;
		// for (int j = 1; j < n; ++j) {
		// 	if (d < qs[j]) {
		// 		res[i] = j;
		// 		break;
		// 	}
		// }
	}
	return res;
}

int
normalizer(const char *in, const char *out) {
	FILE *fin = fopen(in, "rb");
	if (fin == NULL) {
		fprintf(stderr, "Wrong input file.\n");
		return -1;
	}
	fseek(fin, 0, SEEK_END);
	int size = ftell(fin) - 8 * sizeof(float);

	fseek(fin, 8 * sizeof(float), SEEK_SET);
	if (size > (1 << 30)) {
		fprintf(stderr, "Too large malloc.\n");
		return -1;
	}
	float *data = malloc(size);
	fread(data, 1, size, fin);
	fclose(fin);

	int mask_len = 181 * 161;
	FILE *fmask = fopen("mask", "rb");
	uint8_t *mask = malloc(mask_len);
	fread(mask, 1, mask_len, fmask);
	fclose(fmask);

	int len = size / 4;
	float *quantiles = new_quantiles(data, len, mask, mask_len, 100);
	uint8_t *result = new_data(data, len, mask, mask_len, quantiles, 100);

	FILE *fout = fopen(out, "wb");
	fwrite(result, 1, len, fout);
	fclose(fout);

	free(quantiles);
	free(result);
	free(mask);
	free(data);
	return 0;
}
