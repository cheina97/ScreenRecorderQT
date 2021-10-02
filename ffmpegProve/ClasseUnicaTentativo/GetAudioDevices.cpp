#include "GetAudioDevices.h"

#include <filesystem>
#include <fstream>
#include <regex>

using namespace filesystem;
using namespace std;

getAudioDevices::getAudioDevices() {
    cout << "Starting" << endl;
    directory_iterator alsaDir{this->alsaPath};
    regex findCard{".*card(0|[1-9]?[0-9]*)"};
    regex findPcm{".*pcm.*"};
    string value, field, card, device, streamType;

    for (auto i : alsaDir) {
        if (regex_match(i.path().c_str(), findCard)) {
            directory_iterator cardDir{i.path()};
            for (auto const& j : cardDir) {
                if (regex_match(j.path().c_str(), findPcm)) {
                    ifstream info{(string)j.path() + "/info"};
                    if (info.is_open()) {
                        while (!info.eof()) {
                            info >> field;
                            info >> value;
                            if (field == "card:") {
                                card = value;
                            } else if (field == "device:") {
                                device = value;
                            } else if (field == "stream:") {
                                streamType = value;
                            }
                        }
                        if (streamType == "CAPTURE") {
                            devices.emplace_back("hw:" + card + "," + device);
                        }
                    } else {
                        cout << "Errore in apertura" << endl;
                    }
                }
            }
        }
    }
}

vector<string> getAudioDevices::getDevices() {
    return this->devices;
}

getAudioDevices::~getAudioDevices() {
    
}