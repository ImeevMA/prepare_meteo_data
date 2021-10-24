main: main.c
	gcc main.c converter.c coefficients.c normalizer.c -g -lm -o prepare_data

clean:
	rm prepare_data

