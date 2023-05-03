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


#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>


#define MEM_SIZE 16*1024
/*
typedef enum
{
    SET,
    ADD,
    SUB,
    MUL,
    DIV,
    LD,
    RET
} InstrType;
*/
enum
{
	IF,           
	ID,         
	IA,         
    RR,         
	ADD,        
	MUL,         
	DIV,        
	BR,
	MEM,
	MEM2,
	WB,
	STAGE_NUM         
};

typedef struct
{
    //InstrType type;
    char opcode[128];
    int rd;
    int rs1;
    int rs2;
    int imm_1;
    int imm_2;
    bool is_imm_1;
    bool is_imm_2;


} Instruction;


typedef struct {
    int pc;
    char opcode[128];
    int stall; //indicate if the stage is stalled
    int has_instr; //indicate if the stage has valid instruction
    int buffer;
    
    int imm_1;
    int imm_2;
    bool is_imm_1;
    bool is_imm_2;

    int rd;
    int rs1;
    int rs2;
    int rd_value;
    int rs1_value;
    int rs2_value;


} PipelineStage;


typedef struct Register
{
    int value;          // contains register value
    bool is_writing;    // indicate that the register is current being written
	                    // True: register is not ready
						// False: register is ready
} Register;



/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;
	
	int pc;
	int cycles;
	
	int executed_instr; //used to calculate IPC
	
	PipelineStage stage[STAGE_NUM];
	
	Instruction* code_memory;	
	int code_memory_size;

	int data_memory[MEM_SIZE];
	
	bool is_accessing_memory; // indicate that the memory is currently being written
							// True: memory port is not ready
							// False: memory port is ready
	
	int structural_hazard_count;

	int ins_completed;



} CPU;

CPU*
CPU_init(char* filename);

Register*
create_registers(int size);

int
CPU_run(CPU* cpu);

void
CPU_stop(CPU* cpu);

Instruction* create_code_memory(const char* filename, int* size);

Instruction* get_cur_instr(CPU* cpu);

void load_memory_map(int *memory, char *path);

void fetch(CPU* cpu);

void decode(CPU* cpu);

void analyze(CPU* cpu);

void reg_read(CPU* cpu);

void add(CPU* cpu);

void mul(CPU* cpu);

void divide(CPU* cpu);

void branch(CPU* cpu);

void memory(CPU* cpu, int mem_order);

void writeback(CPU* cpu);



#endif
