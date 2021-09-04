main: *.o
	g++ -o main main.o -I/usr/include/x86_64-linux-gnu -lavformat -lavcodec -lavutil -lavdevice -lm -lswscale -lX11

*.o :*.c
	g++ -c  main.cpp
*.c:
	echo "Starting compiling"

clean:
	rm main *.o