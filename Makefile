main: main.c
	gcc main.c converter.c -g -o prepare_data

clean:
	rm prepare_data

