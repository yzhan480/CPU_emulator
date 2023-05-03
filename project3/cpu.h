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

#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>

#define MEM_SIZE 16*1024

enum
{
	IF,//pc           
	ID,         
	IA,         
    RR,   //160      
	ADD, //156    
	MUL,      //152  
	DIV,  //148    
	BR,	//144
	MEM, //140
	MEM2, //136
	WB,	//132
	STAGE_NUM         
};

typedef struct
{
    char opcode[128];
    int rd;
    int rs1;
    int rs2;
    int imm;
    bool is_imm;

} Instruction;

typedef struct 
{
    int pc;
    char opcode[128];
    
    int stall; //indicate if the stage is stalled
    int has_instr; //indicate if the stage has valid instruction
    
    int buffer;
    
    int imm;
    bool is_imm;

    int rd;
    int rs1;
    int rs2;
    int rd_value;
    int rs1_value;
    int rs2_value;

    bool if_branch;
	bool if_taken;

} PipelineStage;


typedef struct Register
{
    int value;          // contains register value
    bool is_writing;	// indicate that the register is current being written
	                    // True: register is not ready
						// False: register is ready
    int will_be_written;	// number of times that this register will be written in data
    int forwarding;	
    int forwarding_value;	//if forwarding is True, then this value can be used to replace the dependent register value
    bool forwarding_control; //control forwarding

} Register;

typedef struct BTB
{
	int tag;
	int target;
} BTB;

/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;

	int PT[16];
	int pt_index;
	int updated_pt;

	BTB *btb;
	int write_btb; //target address
	int pc_tag;

	int branch;

	int pc;
	int updated_pc;
	int cycles;
	
	int executed_instr; //used to calculate IPC
	
	PipelineStage stage[STAGE_NUM];
	
	Instruction* code_memory;	
	int code_memory_size;

	int data_memory[MEM_SIZE];
		
	int data_hazard_count;

	int ins_completed;

	int updated_reg_id;


} CPU;

CPU*
CPU_init(char* filename);

Register*
create_registers(int size);

BTB*
create_BTB(int size);

int
CPU_run(CPU* cpu);

void
CPU_stop(CPU* cpu);

Instruction* create_code_memory(const char* filename, int* size);

Instruction* get_cur_instr(CPU* cpu);

void load_memory_map(int *memory, char *path);
void output_memory_map(int *memory, char* filename);

void fetch(CPU* cpu);

void decode(CPU* cpu);

void analyze(CPU* cpu);

void reg_read(CPU* cpu);

void add(CPU* cpu);

void mul(CPU* cpu);

void divide(CPU* cpu);

void branch(CPU* cpu);

void memory_1(CPU* cpu);
void memory_2(CPU* cpu);

void writeback(CPU* cpu);

bool is_branch(char *opcode);

#endif
