all:
	gcc main.c -o main -Iinclude -lzmq

clean:
	rm main
