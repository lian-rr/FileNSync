all:
	gcc main.c -o fileNsync -Iinclude -lzmq -Wall

clean:
	rm main
