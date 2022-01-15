#include <future>
#include <iostream>
#include <vector>

#include "ScreenRecorder.h"

void handler(ScreenRecorder& screenRecorder) {
    bool handlerEnd = false;
    int command = 0;
    std::cout << "\nScreerRecorder Handler" << endl;
    std::cout << "Commands:\n[1] Pause\n[2] Resume\n[3] Stop" << endl;
    while (!handlerEnd && screenRecorder.getStatus() != RecordingStatus::stopped) {
        cin >> command;
        switch (command) {
            case 1:
                std::cout << "Pause Recording..." << endl;
                screenRecorder.pauseRecording();
                break;
            case 2:
                std::cout << "Resuming Recording..." << endl;
                screenRecorder.resumeRecording();
                break;
            case 3:
                std::cout << "End Recording!" << endl;
                screenRecorder.stopRecording();
                handlerEnd = true;
                break;
            default:
                std::cout << "Invalid Command:\n[1] Pause\n[2] Resume\n[3] Stop" << endl;
                break;
        }
    }
}

int main(int argc, char const* argv[]) {
    if (argc < 12) {
        cerr << "Errors: missing parameters. Usage: ./main width height offset_x offset_y screen_num fps quality compression audioOn audioDevice outFilePath" << endl;
        exit(1);
    }
    VideoSettings vs;
    RecordingRegionSettings rrs;
    rrs.width = atoi(argv[1]);
    rrs.height = atoi(argv[2]);
    rrs.offset_x = atoi(argv[3]);
    rrs.offset_y = atoi(argv[4]);
    rrs.screen_number = atoi(argv[5]);

    vs.fps = atoi(argv[6]);
    vs.quality = atof(argv[7]);
    vs.compression = atoi(argv[8]);
    vs.audioOn = atoi(argv[9]);

    string audioDevice = argv[10];
    string outFilePath = argv[11];

    try {
        ScreenRecorder screenRecorder{rrs, vs, outFilePath, audioDevice};

        auto screenRecorder_thread = thread{
            [&]() {
                try {
                    screenRecorder.record();
                } catch (const std::exception& e) {
                    std::cerr << e.what() << endl;
                    cout << "There was an error in the library, please write 3 and press enter to save what has been recorded" << endl;
                    screenRecorder.stopRecording();
                }
            }};

        handler(screenRecorder);

        screenRecorder_thread.join();
    } catch (const std::exception& e) {
        std::cerr << e.what() << endl;
        cout << "Maybe there was an error opening a device, try to change it" << endl;
    }

    return 0;
}
