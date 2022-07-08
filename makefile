CFLAGS = -Wall -pedantic -Werror -Wextra -Wconversion -std=gnu11 -O2
CC = gcc
#-pg : Generate extra code to write profile information suitable for the analysis program gprof. 
#You must use this option when compiling the source files you want data about, 
#and you must also use it when linking.

all: folders main

reqs:
#sudo apt install netpbm -y

folders:
	mkdir -p ./bin

main: ./src/main.c
	$(CC) $(CFLAGS) ./src/main.c -o ./bin/main -fopenmp

clean:
	rm -rf ./bin *.o pgmb main