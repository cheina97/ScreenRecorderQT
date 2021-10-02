main: *.o
	g++ -Wall -std=c++20 -o main main.o GetAudioDevices.o

*.o :*.c
	g++ -Wall -std=c++20 -c main.cpp GetAudioDevices.cpp
*.c:
	echo "Starting compiling"

clean:
	rm main *.o