#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

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
