/*
This 11 stages pipeline CPU emulator is built on
Xiang's version of CPU emulator.
This emulator satisfies all demands in Project1.pdf
and successfully handles that the memory unit only has one port.
Excution outputs are same with the required program outputs provided.

Running environment: Apple clang version 14.0.0 (clang-1400.0.29.202)
How to run:
make sure program_1.txt and memory_map.txt are under the directory of three c files
Input command line commands below
make
./sim program_1.txt

Author: Yao Zhang
Version: 1.0
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

int main(int argc, const char * argv[]) 
{
    if (argc<=1) 
    {
        fprintf(stderr, "Error : missing required args\n");
        return -1;
    }
    char* filename = (char*)argv[1]; //filename should be like program_1.txt
    
    /*
    FILE *fp = fopen(filename, "r");
    if(fp == NULL)
    {
        printf("Error: could not open input file\n");
        return -1;
    }
    
    char line[MAX_LINE_LEN];
    int i = 0;
    while(fgets(line, MAX_LINE_LEN, fp) != NULL)
    {
        // Parse the instruction and add it to instruction memory
        Instruction *instr = parse_instr(line);
        instr_mem[i++] = instr;
    }
    fclose(fp);*/

    

    run_cpu_fun(filename);
    
    return 0;
}
