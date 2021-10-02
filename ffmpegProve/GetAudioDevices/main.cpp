#include <iostream>

#include "GetAudioDevices.h"

int main(int argc, char const *argv[]) {
    getAudioDevices gad;
    auto devicesList = gad.getDevices();
    for_each(devicesList.begin(), devicesList.end(),
             [](string s) { cout << s << endl; });
    return 0;
}