#include "MemoryCheckLinux.h"

#include <string.h>

#include <iostream>

using namespace std;

//KB
static int memory_limit;

//mem measures
int parseLine(char *line) {
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9') p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

int getCurrentVMemUsedByProc() {  //Note: this value is in KB!
    FILE *file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != nullptr) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    //cout << "CurrMemUsedByProc: " << (double)result / 1024 / 1024 << "GB" << endl;
    return result;
}

/*Setup the memory limit starting from the current memory used by the process. 
  memory_limit=currentUsedMemory+mem_limit, where mem_limit is the argument passed to the function.
  Mem_limit is in MB*/
void memoryCheck_init(int mem_limit) {
    memory_limit = getCurrentVMemUsedByProc() + (mem_limit * 1024);
}

void memoryCheck_limitSurpassed() {
    if (getCurrentVMemUsedByProc() > memory_limit) {
        throw runtime_error{"Reached memory limit of " + to_string((float)memory_limit / 1024) + " MB"};
    }
}
