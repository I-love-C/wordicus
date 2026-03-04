CFLAGS = -O3 -march=native

main:
	gcc $(CFLAGS) wordicus.c -o wordicus
