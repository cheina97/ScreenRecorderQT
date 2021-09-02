main: *.o
	gcc -o main main.o -I/usr/include/x86_64-linux-gnu -lavformat -lavcodec -lavutil -lavdevice -lm

*.o :*.c
	gcc -c  main.c
*.c:
	echo "Starting compiling"

clean:
	rm main *.o