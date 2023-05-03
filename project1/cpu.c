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
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 128



CPU*
CPU_init(char* filename) //initialization
{
    if (!filename)
    {
        return NULL;
    }

    CPU* cpu = malloc(sizeof(*cpu));
    if (!cpu) 
    {
        return NULL;
    }





    cpu->pc = 0;
    cpu->cycles = 0;
    cpu->executed_instr = 0;



    cpu->is_accessing_memory = false;
    cpu->structural_hazard_count = 0;




    memset(cpu->stage, 0, sizeof(PipelineStage) * STAGE_NUM);
    memset(cpu->data_memory, 0, sizeof(int) * MEM_SIZE);
    
    load_memory_map(cpu->data_memory, "memory_map.txt");
    

    //printf("%d", cpu->data_memory[258]);
    /*
    printf("zaijian\n");
    for (int i = 0; i < MEM_SIZE; ++i)
    {
        printf("%d ", cpu->data_memory[i]);
    }
    */

    /* Create register files */
    cpu->regs = create_registers(REG_COUNT);
    /* Create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory) 
    {
        printf("No codes in the memory!\n");
        free(cpu);
        return NULL;
    }

    //printf("OK\n");

    return cpu;
}

/*
 * This function de-allocates CPU cpu.
 */
void
CPU_stop(CPU* cpu)
{
    free(cpu);
}

/*
 * This function prints the content of the registers.
 */
void
print_registers(CPU *cpu){
    
    
    printf("================================\n");
    printf("--------------------------------\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

/*
 *  CPU CPU simulation loop
 */
int
CPU_run(CPU* cpu) //main entrance
{
    
    while(true)
    {
        if (cpu->ins_completed == cpu->code_memory_size)
        {
            break;
        }
        //printf("here");
        
        writeback(cpu);
        memory(cpu,2);
        branch(cpu);
        divide(cpu);
        mul(cpu);
        add(cpu);
        reg_read(cpu);
        analyze(cpu);
        decode(cpu);
        fetch(cpu);

        if (cpu->is_accessing_memory == true) 
        {
            cpu->is_accessing_memory = false;
        }

        cpu->cycles++; //excute all the stage then plus one cycle
                       //
        
        /*
        printf("%d ", cpu->cycles);
        printf("%d ", cpu->is_accessing_memory);
        PipelineStage* stage = &cpu->stage[IF];
        PipelineStage* stage2 = &cpu->stage[WB];
        printf("%s ", stage->opcode);
        printf("%s\n", stage2->opcode);
        */
    }

    print_registers(cpu);
    printf("Stalled cycles due to structural hazard: %d\n", cpu->structural_hazard_count);
    printf("Total execution cycles: %d\n", cpu->cycles);
    printf("Total instruction simulated: %d\n", cpu->executed_instr);
    printf("IPC: %f\n", ((double)cpu->executed_instr/cpu->cycles));

   
    return 0;
}

Register*
create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        regs[i].value = 0;
        regs[i].is_writing = false;
    }
    return regs;
}

void load_memory_map(int *memory, char *path){
    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("cannot open file: %s", path);
        exit(1);
    }
    char c;
    c = fgetc(fp);
    char num_str[10]={0};
    int str_idx = 0;
    int memory_idx = 0;
    while (c != EOF)
    {
        if (c == ' ') {
            num_str[str_idx] = '\0';
            memory[memory_idx++] = atoi(num_str);
            str_idx = 0;
        }else{
            num_str[str_idx++] = c;
        }
        c = fgetc(fp);
    }
    fclose(fp);
    if (memory_idx != 16*1024) {
        printf("memory size(%d) failed!\n",memory_idx);
        exit(1);
    }  
} 

int get_num_from_string(char* buffer){
    char str[16];
    int j = 0;
    for (int i = 1; buffer[i] != '\0'; ++i) {
        str[j] = buffer[i];
        j++;
    }
    str[j] = '\0';
    return atoi(str);
}

void create_Instruction(Instruction* ins, char* buffer)
{
    
    char* token = strtok(buffer, " ");
    int token_num = 0;
    char tokens[6][128];
    while (token != NULL) {
        if (token_num>0) {
            strcpy(tokens[token_num-1], token);
        }
        token_num++;
        token = strtok(NULL, " ");
    }
    token_num--;
    
    
    ins->rd = -1;
    ins->rs1 = -1;
    ins->rs2 = -1;
    ins->imm_1 = -1;
    ins->imm_2 = -1;
    ins->is_imm_1 = false;
    ins->is_imm_2 = false;

    char *token0 = tokens[0];
    
    if (token0[strlen(token0)-1] == '\n') {
        token0[strlen(token0)-1] = '\0';
    }
    if (token0[strlen(token0)-1] == '\r') {
        token0[strlen(token0)-1] = '\0';
    }

    strcpy(ins->opcode, token0);
    
    char *token_last = tokens[token_num-1];
    

    if (token_last[0] != 'r') //not ret instruction
    {
        if (token_last[0] == '#') 
        {   
            if (tokens[token_num-2][0] == '#')
            {
                printf("%s", tokens[token_num-2]);
                ins->is_imm_1 = true;
                ins->is_imm_2 = true;        
            }
            else
            {
                ins->is_imm_1 = true;
                ins->is_imm_2 = false;
            }            
        }
    }

    if (strcmp(ins->opcode, "set") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->imm_1 = get_num_from_string(tokens[2]);
    }
    
    if (strcmp(ins->opcode, "ld") == 0)
    {
        ins->rd = get_num_from_string(tokens[1]);
        if (ins->is_imm_1) 
        {
            ins->imm_1 = get_num_from_string(tokens[2]);
        }
        else
        {
            ins->rs1 = get_num_from_string(tokens[2]);
        }
    }
        
    if (strcmp(ins->opcode, "add") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);

        if (ins->is_imm_2) 
        {   
            ins->imm_1 = get_num_from_string(tokens[2]);
            ins->imm_2 = get_num_from_string(tokens[3]);
        }
        else
        {   
            ins->rs1 = get_num_from_string(tokens[2]);
            ins->imm_1 = get_num_from_string(tokens[3]);
        }
    }
    
    if (strcmp(ins->opcode, "sub") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);

        if (ins->is_imm_2) 
        {   
            ins->imm_1 = get_num_from_string(tokens[2]);
            ins->imm_2 = get_num_from_string(tokens[3]);
        }
        else
        {   
            ins->rs1 = get_num_from_string(tokens[2]);
            ins->imm_1 = get_num_from_string(tokens[3]);
        }
    }
    
    if (strcmp(ins->opcode, "mul") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);

        if (ins->is_imm_2) 
        {   
            ins->imm_1 = get_num_from_string(tokens[2]);
            ins->imm_2 = get_num_from_string(tokens[3]);
        }
        else
        {   
            ins->rs1 = get_num_from_string(tokens[2]);
            ins->imm_1 = get_num_from_string(tokens[3]);
        }
    }
    if (strcmp(ins->opcode, "div") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);

        if (ins->is_imm_2) 
        {   
            ins->imm_1 = get_num_from_string(tokens[2]);
            ins->imm_2 = get_num_from_string(tokens[3]);
        }
        else
        {   
            ins->rs1 = get_num_from_string(tokens[2]);
            ins->imm_1 = get_num_from_string(tokens[3]);
        }
    }
}






Instruction* create_code_memory(const char* filename, int* size)
{
    if (!filename) {
        return NULL;
    }
   
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }
    
    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    int code_memory_size = 0;
    
    while ((nread = getline(&line, &len, fp)) != -1) {
        code_memory_size++;
    }
    *size = code_memory_size;
    if (!code_memory_size) {
        fclose(fp);
        return NULL;
    }
    
    Instruction* code_memory = malloc(sizeof(*code_memory) * code_memory_size);
    if (!code_memory) {
        fclose(fp);
        return NULL;
    }
    
    rewind(fp);
    int current_instruction = 0;
    while ((nread = getline(&line, &len, fp)) != -1) {
        create_Instruction(&code_memory[current_instruction], line);
        current_instruction++;
    }
    free(line);
    fclose(fp);
    return code_memory;
}

Instruction* get_cur_instruction(CPU* cpu)
{
    int code_index = (cpu->pc)/4;
    if (code_index>=cpu->code_memory_size)
    {
        return NULL; //mark
    }

    return &cpu->code_memory[code_index];
}



/*
Instruction *parse_instr(char *instr_str)
{
    Instruction *instr = (Instruction *)malloc(sizeof(Instruction));
    char *token = strtok(instr_str, " ");
    if(strcmp(token, "sub") == 0)
    {
        instr->type = sub;
        instr->opcode = 0;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token+1);
        token = strtok(NULL, " ");
        instr->op3 = atoi(&token+1);
    }
    else if(strcmp(token, "add") == 0)
    {
        instr->type = add;
        instr->opcode = 1;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token+1);
        token = strtok(NULL, " ");
        instr->op3 = atoi(&token+1);
    }
    else if(strcmp(token, "mul") == 0)
    {
        instr->type = mul;
        instr->opcode = 2;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token+1);
        token = strtok(NULL, " ");
        instr->op3 = atoi(&token+1);
    }
    else if(strcmp(token, "div") == 0)
    {
        instr->type = div;
        instr->opcode = 3;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token+1);
        token = strtok(NULL, " ");
        instr->op3 = atoi(&token+1);
    }
    else if(strcmp(token, "set") == 0)
    {
        instr->type = set;
        instr->opcode = 0;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token+1);
    }
    else if(strcmp(token, "ld") == 0)
    {
        instr->type = ld;
        instr->opcode = 1;
        token = strtok(NULL, " ");
        instr->op1 = atoi(&token[1]);
        token = strtok(NULL, " ");
        instr->op2 = atoi(token);
    }
    else if(strcmp(token, "st") == 0)
    {
        instr->type = st;
        instr->opcode = 2;
        token = strtok(NULL, " ");
        instr->op1 = atoi(token);
        token = strtok(NULL, " ");
        instr->op2 = atoi(&token[1]);
    }
    else if(strcmp(token, "ret") == 0)
    {
        instr->type = ret;
        instr->opcode = 0;
    }
    else
    {
        print("parse_instr wrong!")
    }
    
    return instr;

}*/

void fetch(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[IF];
    if (cpu->is_accessing_memory == true)
    {
        stage->stall = 1;
        cpu->structural_hazard_count++;
    }
    else
    {
        stage->stall = 0;
    }
    if (!stage->stall) //maybe have bugs here
    {
        Instruction* current_instruction = get_cur_instruction(cpu);
        if (current_instruction == NULL)        
        {
            //printf("fetch_null\n");
            return ;
        }
        
        stage->pc = cpu->pc;
        strcpy(stage->opcode, current_instruction->opcode);

        stage->rd = current_instruction->rd;
        stage->rs1 = current_instruction->rs1;
        stage->rs2 = current_instruction->rs2;
        
        stage->imm_1 = current_instruction->imm_1;
        stage->imm_2 = current_instruction->imm_2;
        stage->is_imm_1 = current_instruction->is_imm_1;
        stage->is_imm_2 = current_instruction->is_imm_2;
        
        stage->has_instr = 1;

        cpu->pc = cpu->pc + 4;

        cpu->stage[ID] = cpu->stage[IF]; //mark

    }

    stage->has_instr = 0;

}

void decode(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[ID];
    if ((stage->has_instr) && (!stage->stall))
    {   
        if (strcmp(stage->opcode, "set") == 0)
        {

        }
        if (strcmp(stage->opcode, "add") == 0)
        {
            
        }
        if (strcmp(stage->opcode, "sub") == 0)
        {
            
        }
        if (strcmp(stage->opcode, "mul") == 0)
        {

        }
        if (strcmp(stage->opcode, "div") == 0)
        {

        }
        if (strcmp(stage->opcode, "ld") == 0) 
        {

        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[IA] = cpu->stage[ID];
        stage->has_instr = 0;
    }

}

void analyze(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[IA];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[RR] = cpu->stage[IA];
        stage->has_instr = 0;
    }
}

void reg_read(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[RR];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        if (strcmp(stage->opcode, "add") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->rs1_value = cpu->regs[stage->rs1].value;
            }
        }
        if (strcmp(stage->opcode, "sub") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->rs1_value = cpu->regs[stage->rs1].value;
            }
        }
        if (strcmp(stage->opcode, "mul") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->rs1_value = cpu->regs[stage->rs1].value;
            }
        }
        if (strcmp(stage->opcode, "div") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->rs1_value = cpu->regs[stage->rs1].value;
            }
        }
        if (strcmp(stage->opcode, "ld") == 0)
        {
            if (stage->is_imm_1 == false)
            {
                stage->rs1_value = cpu->regs[stage->rs1].value;
            }
        }

        cpu->stage[ADD] = cpu->stage[RR];
        stage->has_instr = 0;
    }
}

void add(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[ADD];
    if ((stage->has_instr) && (!stage->stall))
    {
        
        if (strcmp(stage->opcode, "add") == 0)
        {   
            if (stage->is_imm_2 == false)
            {
                stage->buffer = stage->rs1_value + stage->imm_1;
            }
            else
            {
                stage->buffer = stage->imm_1 + stage->imm_2;
            }
        }

        if (strcmp(stage->opcode, "sub") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->buffer = stage->rs1_value - stage->imm_1;
            }
            else
            {
                stage->buffer = stage->imm_1 - stage->imm_2;
            }
        }
        if (strcmp(stage->opcode, "set") == 0)
        {
            stage->buffer = stage->imm_1;
        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[MUL] = cpu->stage[ADD];
        stage->has_instr = 0;
    }
}

void mul(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[MUL];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "mul") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->buffer = stage->rs1_value * stage->imm_1;
            }
            else
            {
                stage->buffer = stage->imm_1 * stage->imm_2;
            }
        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[DIV] = cpu->stage[MUL];
        stage->has_instr = 0;
    }   
}

void divide(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[DIV];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "div") == 0)
        {
            if (stage->is_imm_2 == false)
            {
                stage->buffer = stage->rs1_value / stage->imm_1;
            }
            else
            {
                stage->buffer = stage->imm_1 / stage->imm_2;
            }
        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[BR] = cpu->stage[DIV];
        stage->has_instr = 0;
    }
}

void branch(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[BR];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[MEM] = cpu->stage[BR];
        stage->has_instr = 0;
    }
}

void memory(CPU* cpu, int mem_order)
{
    PipelineStage* stage = &cpu->stage[MEM-1+mem_order];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (mem_order == 2)
        {
            if (strcmp(stage->opcode, "ld") == 0)
            {
                cpu->is_accessing_memory = true;
                printf("%d\n", stage->buffer);
                printf("%d\n", stage->is_imm_1);
                stage->buffer = cpu->data_memory[stage->buffer/4];
                printf("%d\n", stage->buffer);
            }
        }
        else
        {
            if (strcmp(stage->opcode, "ld") == 0)
            {
                if (stage->is_imm_1)
                {
                    stage->buffer = stage->imm_1;
                }
                else
                {
                    stage->buffer = stage->rs1_value;
                }
            }
        }
        cpu->stage[MEM+mem_order] = *stage;
        stage->has_instr = 0;   
    }
    if (mem_order>1)
    {
        memory(cpu, mem_order-1);
    }
}

void writeback(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[WB];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (stage->rd >= 0 && stage->rd < 128) 
        {            
            cpu->regs[stage->rd].value = stage->buffer;
        }
    
        cpu->ins_completed = (stage->pc)/4; 
        stage->has_instr = 0;
        cpu->executed_instr++; 

        if (strcmp(stage->opcode, "ret") == 0) 
        {
            cpu->ins_completed = cpu->code_memory_size;
        }
    
    }
}
/*

int get_code_memory_index(int pc)
{
    return (pc-0) / 4;
}
*/

