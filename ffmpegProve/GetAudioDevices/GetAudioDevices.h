#include <iostream>
#include <vector>
#include <string>

using namespace std;

class getAudioDevices {
   private:
    vector<string> devices;
    string const alsaPath = "/proc/asound";

   public:
    getAudioDevices();
    ~getAudioDevices();
    vector<string> getDevices();
};
