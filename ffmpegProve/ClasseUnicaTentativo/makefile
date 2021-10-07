main: *.o
	g++ -Wall -std=c++2a -o main main.o GetAudioDevices.o ScreenRecorder.o MemoryCheckLinux.o -lavformat -lavcodec -lavutil -lavdevice -lm -lswscale -lX11 -lpthread -lswresample -lasound

*.o :*.c
	g++ -Wall -std=c++2a -c main.cpp GetAudioDevices.cpp ScreenRecorder.cpp MemoryCheckLinux.cpp
*.c:
	echo "Starting compiling"
clean:
	rm main *.o