all:
	gcc main.c -o main -Iinclude -lzmq -Wall

clean:
	rm main
