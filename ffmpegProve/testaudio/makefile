main: *.o
	g++ -Wall -o main main.o -lavformat -lavcodec -lavutil -lavdevice -lm -lswscale -lX11 -lswresample -lasound

*.o :*.c
	g++ -Wall -c main.cpp
*.c:
	echo "Starting compiling"

clean:
	rm main *.o