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
	int k = (len / n) * i;
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
new_data(const float *data, int len, const float *qs, int n)
{
	uint8_t *res = malloc(len);
	for (int i = 0; i < len; ++i) {
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

	int len = size / 4;
	float *quantiles = new_quantiles(data, len, 10);
	uint8_t *result = new_data(data, len, quantiles, 10);

	FILE *fout = fopen(out, "wb");
	fwrite(result, 1, len, fout);
	fclose(fout);

	free(quantiles);
	free(result);
	free(data);
	return 0;
}
