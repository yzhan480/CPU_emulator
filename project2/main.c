/*
This 11 stages pipeline CPU emulator is built on
my version 1.0 of CPU emulator.
This emulator satisfies all demands in Project2.pdf.

log:
This version of CPU emulator repairs version 1.0 can not properly handle
at the end of some instructions in the .txt file followed by one space.
So my project1 has some bugs and it does not as I said that perform same with
given results.


note:
This CPU emulator outputs an "output_memory.txt" if it receives right instruction file.
But this output filename does not vary as the name of input file varies.
My stall cycles due to data hazard is 65 but yours is 62 when executing program4 and
the rest of results I think are the same.


How to run:
make sure program4.txt and memory_map.txt are under the directory of three c files
Input command line commands below
make
./sim program4.txt

Author: Yao Zhang
Version: 2.0
All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

int binary_flag;

void run_cpu_fun(char* filename){

    CPU *cpu = CPU_init(filename);
    if (!cpu)
    {
        fprintf(stderr, "Error : Unable to initialize CPU\n");
        exit(1);
    }
    CPU_run(cpu);
    CPU_stop(cpu);
}

int main(int argc, const char * argv[]) {
    if (argc<=1) {
        fprintf(stderr, "Error : missing required args\n");
        return -1;
    }
    char* filename = (char*)argv[1];
    
    run_cpu_fun(filename);
    
    return 0;
}
