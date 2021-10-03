#include <iostream>
#include <future>

#include "GetAudioDevices.h"
#include "MemoryCheckLinux.h"
#include "ScreenRecorder.h"

int main(int argc, char const* argv[]) {
    cout << "-> Avvio main" << endl;

    if (argc < 9) {
        cout << "Errore: parametri mancanti" << endl
             << "Utilizzo: ./main width height offset_x offset_y screen_num fps capturetime_seconds quality" << endl;
    }
    VideoSettings vs;
    AudioSettings as;
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
    
    ScreenRecorder sr{rrs, vs, as};
    cout << "-> Costruito oggetto Screen Recorder" << endl;
    cout << "-> RECORDING..." << endl;
        
    std::future<int> handle = std::async(std::launch::async, [&sr]()->int{return sr.record();});
    if(handle.get() == 0){
        cout << "-> Fine Programma"<<endl;
    }else{
        cerr << "-> Errore nella registrazione"<<endl;
    }

    /*auto dev = getAudioDevices();
        for (auto const& d : dev) {
            cout << d << endl;
        }   
    */
    return 0;
}