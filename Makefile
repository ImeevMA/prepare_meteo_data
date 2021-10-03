main: main.c
	gcc main.c converter.c -o prepare_data

clean:
	rm prepare_data

