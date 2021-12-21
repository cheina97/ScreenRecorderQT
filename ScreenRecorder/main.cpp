#include <future>
#include <iostream>
#include <vector>

#include "GetAudioDevices.h"
#include "MemoryCheckLinux.h"
#include "ScreenRecorder.h"

int main(int argc, char const* argv[]) {
    try {
        cout << "-> Avvio main" << endl;

        if (argc < 11) {
            throw logic_error{ "Errore: parametri mancanti. Utilizzo: ./main width height offset_x offset_y screen_num fps capturetime_seconds quality audioOn outFilePath" };
        }
        VideoSettings vs;
        RecordingRegionSettings rrs;
        rrs.width = atoi(argv[1]);
        rrs.height = atoi(argv[2]);
        rrs.offset_x = atoi(argv[3]);
        rrs.offset_y = atoi(argv[4]);
        rrs.screen_number = atoi(argv[5]);
        rrs.fullscreen = false;
        vs.fps = atoi(argv[6]);
        vs.capturetime_seconds = atoi(argv[7]);
        vs.quality = atof(argv[8]);

        bool audioOn = atoi(argv[9]) == 1 ? true : false;
        string outFilePath = argv[10];

        ScreenRecorder sr{rrs, vs, audioOn, outFilePath, getAudioDevices()[2].c_str()};
        cout << "-> Costruito oggetto Screen Recorder" << endl;
        cout << "-> RECORDING..." << endl;

        //std::future<int> handle = std::async(std::launch::async, [&sr]()->int{return sr.record();});
        sr.record();
    }
    catch (const std::exception &e)
    {
        cout << "Exception handled in main" << endl;
        cerr << e.what() << endl;
    }
    return 0;
}
