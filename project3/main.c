/*
This 11 stages pipeline CPU emulator is built on
my version 2.0 of CPU emulator.
This emulator satisfies all demands in Project3.pdf.


note:
This CPU emulator outputs an "mmap_<program name>.txt" if it receives <program name> instruction file.
For example, if run this command "./sim program4.txt", the program will output "mmap_program4.txt".
Run program1,2,3 is the same. My running results are the same with provided logs including results and
memory_map checked by diff command.


How to run:
make sure program4.txt and memory_map.txt are under the directory of three c files
Input command line commands below
make
./sim program4.txt

Author: Yao Zhang
Version: 3.0
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
    output_memory_map(cpu->data_memory, filename);
    CPU_stop(cpu);
}

int main(int argc, const char * argv[]) 
{
    if (argc<=1) {
        fprintf(stderr, "Error : missing required args\n");
        return -1;
    }
    char* filename = (char*)argv[1];
    
    run_cpu_fun(filename);

    return 0;
}


void output_memory_map(int *memory, char* filename)
{   
    if (strcmp(filename, "program1.txt") == 0)
    {
        FILE *file = fopen("mmap_program1.txt", "w");
        if (file == NULL)
        {
            printf("Error opening file.\n");
            return ;
        }
        for (int i = 0; i < 16*1024; ++i)
        {
            fprintf(file, "%d", memory[i]);
            if (i < 16*1024-1) 
            {
                fprintf(file, " ");
            }
        }
        fclose(file);
    }
    if (strcmp(filename, "program2.txt") == 0)
    {
        FILE *file = fopen("mmap_program2.txt", "w");
        if (file == NULL)
        {
            printf("Error opening file.\n");
            return ;
        }
        for (int i = 0; i < 16*1024; ++i)
        {
            fprintf(file, "%d", memory[i]);
            if (i < 16*1024-1) 
            {
                fprintf(file, " ");
            }
        }
        fclose(file);
    }
    if (strcmp(filename, "program3.txt") == 0)
    {
        FILE *file = fopen("mmap_program3.txt", "w");
        if (file == NULL)
        {
            printf("Error opening file.\n");
            return ;
        }
        for (int i = 0; i < 16*1024; ++i)
        {
            fprintf(file, "%d", memory[i]);
            if (i < 16*1024-1) 
            {
                fprintf(file, " ");
            }
        }
        fclose(file);
    }
    if (strcmp(filename, "program4.txt") == 0)
    {
        FILE *file = fopen("mmap_program4.txt", "w");
        if (file == NULL)
        {
            printf("Error opening file.\n");
            return ;
        }
        for (int i = 0; i < 16*1024; ++i)
        {
            fprintf(file, "%d", memory[i]);
            if (i < 16*1024-1) 
            {
                fprintf(file, " ");
            }
        }
        fclose(file);
    }
}