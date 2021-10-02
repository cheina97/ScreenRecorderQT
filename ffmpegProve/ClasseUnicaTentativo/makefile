main: *.o
	g++ -Wall -std=c++20 -o main main.o GetAudioDevices.o ScreenRecorder.o

*.o :*.c
	g++ -Wall -std=c++20 -c main.cpp GetAudioDevices.cpp ScreenRecorder.c
*.c:
	echo "Starting compiling"
clean:
	rm main *.o