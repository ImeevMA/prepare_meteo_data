#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"


static float
get_quantile(const float *data, int len, float min, float max, int i, int n)
{
	if (i == 0)
		return min;
	if (i == n)
		return max;
	int m = i == 1 ? 1 : 999;
	int k = (len / 1000.0) * m;
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
		quantiles[i] = get_quantile(data, len, tmp_min, max, i, n);
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

	int mask_len = 161 * 181;
	FILE *fmask = fopen("mask", "rb");
	if (fmask == NULL) {
		fprintf(stderr, "Wrong mask file.\n");
		return -1;
	}
	uint8_t *mask = malloc(mask_len * sizeof(*mask));
	fread(mask, 1, mask_len, fmask);
	int masked = 0;
	for (int i = 0; i < mask_len; ++i)
		masked += mask[i];
	int masked_size = masked * sizeof(float);
	float *masked_data = malloc(masked_size);

	FILE *fout = fopen(out, "wb");
	for (int k = 0; k < 1464; ++k) {
		int j = 0;
		int d = k * mask_len;
		for (int i = 0; i < mask_len; ++i) {
			if (mask[i % mask_len] != 0)
				masked_data[j++] = data[i + d];
		}

		float *quantiles = new_quantiles(masked_data, masked, 3);
		uint8_t *result = new_data(&data[d], mask_len, mask, mask_len,
					   quantiles, 3);

		fwrite(result, 1, mask_len, fout);
		free(quantiles);
		free(result);
	}
	fclose(fout);

	free(data);
	return 0;
}
