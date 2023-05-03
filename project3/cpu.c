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
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 16
 
CPU*
CPU_init(char* filename)
{
    if (!filename)
    {
        return NULL;
    }

    CPU* cpu = malloc(sizeof(*cpu));
    if (!cpu) {
        return NULL;
    }

    cpu->pc = 0;
    cpu->cycles = 0;
    cpu->executed_instr = 0;
    cpu->data_hazard_count = 0;
    cpu->updated_reg_id = -1;
    
    cpu->updated_pt = 0;
    cpu->pt_index = -1;

    cpu->write_btb = -1;
    cpu->pc_tag = -1;

    cpu->branch = 0;

    cpu->updated_pc = -1;

    memset(cpu->stage, 0, sizeof(PipelineStage) * STAGE_NUM);
    memset(cpu->data_memory, 0, sizeof(int) * MEM_SIZE);
    
    for (int i = 0; i < 16; ++i)
    {
        cpu->PT[i] = 3;
    }

    load_memory_map(cpu->data_memory, "memory_map.txt");
    
    /* Create register files */
    cpu->regs= create_registers(REG_COUNT);
    cpu->btb = create_BTB(16); //Branch target buffer
    /* Create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory) 
    {
        printf("No codes in the memory!\n");
        free(cpu);
        return NULL;
    }

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
    
    
    printf("================================\n\n");

    printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n\n");

    printf("--------------------------------\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

void print_display(CPU *cpu, int cycle){
    printf("================================\n");
    printf("Clock Cycle #: %d\n", cycle);
    printf("--------------------------------\n");

   for (int reg=0; reg<REG_COUNT; reg++) {
       
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n");
    printf("\n");

}

/*
 *  CPU CPU simulation loop
 */
int
CPU_run(CPU* cpu)
{

    while(true)
    {   
        if (cpu->ins_completed == cpu->code_memory_size)
        {
            break;
        }
        PipelineStage* initial_if_stage = &cpu->stage[IF];
        initial_if_stage->has_instr = 1;
        
        /*
        if (cpu->pc == 48 )
        {
            printf("%d ", cpu->cycles+1);
        }*/
        
        writeback(cpu);
        memory_2(cpu);
        memory_1(cpu);
        branch(cpu);
        divide(cpu);
        mul(cpu);
        add(cpu);
        reg_read(cpu);
        analyze(cpu);
        decode(cpu);
        fetch(cpu);

        if (cpu->updated_reg_id != -1) 
        {
            cpu->regs[cpu->updated_reg_id].is_writing = false;
            cpu->updated_reg_id = -1;
        }

        if (cpu->pc_tag != -1)
        {
            cpu->btb[cpu->pt_index].tag = cpu->pc_tag;
            cpu->btb[cpu->pt_index].target = cpu->write_btb;
            cpu->pc_tag = -1;
            cpu->write_btb = -1;
        }

        if (cpu->pt_index != -1)
        {
            if (cpu->updated_pt == 1)
            {
                cpu->PT[cpu->pt_index]++;
                if (cpu->PT[cpu->pt_index] > 7)
                {
                    cpu->PT[cpu->pt_index] = 7;
                }
            }
            if (cpu->updated_pt == -1)
            {
                cpu->PT[cpu->pt_index]--;
                if (cpu->PT[cpu->pt_index] < 0)
                {
                    cpu->PT[cpu->pt_index] = 0;
                }
            }
            cpu->pt_index = -1;
            cpu->updated_pt = 0;
        }

        if (cpu->updated_pc != -1)
        {
            cpu->pc = cpu->updated_pc;
            cpu->updated_pc = -1;
        }

        cpu->cycles++; //excute all the stage then plus one cycle
                       //
        
        print_display(cpu, cpu->cycles);
        /*for (int i = 0; i < 16; ++i)
        {
            printf("%d ", cpu->btb[i].tag);
        }*/
        //printf("%d %d", cpu->cycles, cpu->regs[6].will_be_written);
        //printf("\n");
    }

    //print_display(cpu,0); 
    print_registers(cpu);   
    printf("Stalled cycles due to data hazard: %d\n", cpu->data_hazard_count);
    printf("Total execution cycles: %d\n", cpu->cycles);
    printf("Total instruction simulated: %d\n", cpu->executed_instr);
    printf("IPC: %f\n", ((double)cpu->executed_instr/cpu->cycles));

   
    return 0;
}

bool is_branch(char *opcode)
{
    if (strcmp(opcode, "bez") == 0 || strcmp(opcode, "bgez") == 0 || strcmp(opcode, "blez") == 0 || strcmp(opcode, "bgtz") == 0 || strcmp(opcode, "bltz") == 0)
    {
        return true;
    }
    return false;
}

BTB*
create_BTB(int size)
{
    BTB* btb = malloc(sizeof(*btb) * size);
    if (!btb) {
        return NULL;
    }
    for (int i=0; i<size; i++)
    {
        btb[i].tag = -1;
        btb[i].target = -1;
    }
    return btb;
}

Register*
create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++)
    {
        regs[i].value = 0;
        regs[i].is_writing = false;
        regs[i].will_be_written = 0;
        regs[i].forwarding = 0;
        regs[i].forwarding_value = 0;
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
        token = strtok(NULL, ", \n");
    }
    token_num--;
    
    ins->rd = -1;
    ins->rs1 = -1;
    ins->rs2 = -1;
    ins->imm = -1;
    ins->is_imm = false;

    char *token0 = tokens[0];
    
    strcpy(ins->opcode, token0);
    
    char *token_last = tokens[token_num-1];
    
    if (token_last[0] == '#') 
    {
        ins->is_imm = true;
    }

    if (strcmp(ins->opcode, "set") == 0) 
    { // set
        ins->rd = get_num_from_string(tokens[1]);
        ins->imm = get_num_from_string(tokens[2]);
    }
    
    if (strcmp(ins->opcode, "ld") == 0) 
    { // load
        ins->rd = get_num_from_string(tokens[1]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[2]);
        }
        else
        {
            ins->rs1 = get_num_from_string(tokens[2]);
        }
    }
    
    if (strcmp(ins->opcode, "st") == 0) 
    {// store
        ins->rd = get_num_from_string(tokens[1]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[2]);
        }
        else
        {
            ins->rs1 = get_num_from_string(tokens[2]);
        }
    }
    
    if (strcmp(ins->opcode, "add") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->rs1 = get_num_from_string(tokens[2]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[3]);
        }
        else
        {
            ins->rs2 = get_num_from_string(tokens[3]);
        }
    }
    
    if (strcmp(ins->opcode, "sub") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->rs1 = get_num_from_string(tokens[2]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[3]);
        }
        else
        {
            ins->rs2 = get_num_from_string(tokens[3]);
        }
    }
    
    if (strcmp(ins->opcode, "mul") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->rs1 = get_num_from_string(tokens[2]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[3]);
        }
        else
        {
            ins->rs2 = get_num_from_string(tokens[3]);
        }
    }

    if (strcmp(ins->opcode, "div") == 0) 
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->rs1 = get_num_from_string(tokens[2]);
        if (ins->is_imm) 
        {
            ins->imm = get_num_from_string(tokens[3]);
        }
        else
        {
            ins->rs2 = get_num_from_string(tokens[3]);
        }
    }
    
    if (is_branch(ins->opcode))
    {
        ins->rd = get_num_from_string(tokens[1]);
        ins->imm = get_num_from_string(tokens[2]);
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
        //printf("%s",line);
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


void fetch(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[IF];
    PipelineStage* next_stage = &cpu->stage[ID];
    stage->stall = next_stage->stall;

    stage->if_branch = false;
    stage->if_taken = false;

    if ((stage->has_instr) && (!stage->stall))
    {   

        Instruction* current_instruction = get_cur_instruction(cpu);
        if (current_instruction == NULL)        
        {
            return ;
        }            
        stage->pc = cpu->pc;
        strcpy(stage->opcode, current_instruction->opcode);

        stage->rd = current_instruction->rd;
        stage->rs1 = current_instruction->rs1;
        stage->rs2 = current_instruction->rs2;
        
        stage->imm = current_instruction->imm;

        stage->is_imm = current_instruction->is_imm;
        

        int pt_index = (stage->pc)/4 % 16;
        int pc_tag = (stage->pc)/64;
        int btb_tag = cpu->btb[pt_index].tag;
        int target = cpu->btb[pt_index].target;

        if (btb_tag != -1)
        {
            if (btb_tag == pc_tag)
            {
                stage->if_branch = true;
                if (cpu->PT[pt_index]>=4)
                {
                    stage->if_taken = true;
                    cpu->pc = target;
                }
                else
                {
                    stage->if_taken = false;
                    cpu->pc += 4;
                }
            }
            else
            {
                stage->if_branch = false;
                cpu->pc += 4;
            }
        }
        else
        {
            cpu->pc += 4;
        }

        //printf("%s\n",stage->opcode);
    }

    if (stage->stall == 0)
    {
        cpu->stage[ID] = cpu->stage[IF];
        stage->has_instr = 0;
    }

}

void decode(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[ID];
    PipelineStage* next_stage = &cpu->stage[IA];
    stage->stall = next_stage->stall;
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
        if (strcmp(stage->opcode, "st") == 0) 
        {

        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        
    }


    if (stage->stall == 0)
    {
        cpu->stage[IA] = cpu->stage[ID];
        stage->has_instr = 0;
    }
}

void analyze(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[IA];
    PipelineStage* next_stage = &cpu->stage[RR];
    stage->stall = next_stage->stall;
    
    if ((stage->has_instr) && (!stage->stall))
    {   
        if (strcmp(stage->opcode, "set") == 0)
        {
           cpu->regs[stage->rd].will_be_written++ ;
        }
        if (strcmp(stage->opcode, "add") == 0 || strcmp(stage->opcode, "sub") == 0 || strcmp(stage->opcode, "mul") == 0 || strcmp(stage->opcode, "div") == 0)
        {                        
            cpu->regs[stage->rd].will_be_written++ ;            
        }
        
        if (strcmp(stage->opcode, "ld") == 0) 
        {   
            cpu->regs[stage->rd].will_be_written++ ;
        }
        if (strcmp(stage->opcode, "st") == 0) 
        {

        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }

               
    }
    
    if (stage->stall == 0)
    {
        cpu->stage[RR] = cpu->stage[IA];
        stage->has_instr = 0;
    }

}





void reg_read(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[RR];
    if (stage->stall == 1)
    {   
        cpu->data_hazard_count++;
        stage->stall = 0;
    }
    if ((stage->has_instr) && (!stage->stall))
    {   
        

        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        if (strcmp(stage->opcode, "add") == 0 || strcmp(stage->opcode, "sub") == 0 || strcmp(stage->opcode, "mul") == 0 || strcmp(stage->opcode, "div") == 0)
        {
            if (stage->is_imm == true)
            {
                if (cpu->regs[stage->rs1].is_writing == false)
                {
                    if (cpu->regs[stage->rs1].will_be_written == 0) //no data hazard
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].value;

                    }
                    else //dependent
                    {   
                        if (stage->rs1 == stage->rd && cpu->regs[stage->rs1].will_be_written == 1)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].value;
                        }
                        else if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                        }
                        else //no forwarding
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                
                }

                else // register is writing back 
                {
                    if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }
            }
            else // two registers read
            {   
            
                if (cpu->regs[stage->rs1].is_writing == false)
                {
                    if (cpu->regs[stage->rs1].will_be_written == 0) //no data hazard
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].value;
                    }
                    else //dependent
                    {   
                        if (stage->rd == stage->rs1 && cpu->regs[stage->rs1].will_be_written == 1)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].value;
                        }
                        else if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                        }
                        else //no forwarding
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                
                }
                else // register is writing back 
                {   
                    if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }

                if (cpu->regs[stage->rs2].is_writing == false)
                {   

                    if (cpu->regs[stage->rs2].will_be_written == 0) //no data hazard
                    {
                        stage->rs2_value = cpu->regs[stage->rs2].value;
                    }
                    else //dependent
                    {   
                        if (stage->rs2 == stage->rd && cpu->regs[stage->rs2].will_be_written == 1)
                        {
                            stage->rs2_value = cpu->regs[stage->rs2].value;
                        }
                        else if (cpu->regs[stage->rs2].forwarding > 0 && cpu->regs[stage->rs2].forwarding_control == true)
                        {
                            stage->rs2_value = cpu->regs[stage->rs2].forwarding_value;
                        }
                        else //no forwarding
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                    
                }
                else // register is writing back 
                {   
                    if (cpu->regs[stage->rs2].forwarding > 0 && cpu->regs[stage->rs2].forwarding_control == true)
                    {
                        stage->rs2_value = cpu->regs[stage->rs2].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }
            }
        }
        
        if (strcmp(stage->opcode, "st") == 0)
        {
            if (stage->is_imm == true)
            {
                if (cpu->regs[stage->rd].is_writing == false)
                {
                    if (cpu->regs[stage->rd].will_be_written == 0)
                    {
                        stage->rd_value = cpu->regs[stage->rd].value;
                    }
                    else
                    {
                        if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                        {
                            stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                        }
                        else
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                }
                else
                {   
                    if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                    {
                        stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }
            }
            else
            {   

                if (cpu->regs[stage->rd].is_writing == false)
                {
                    if (cpu->regs[stage->rd].will_be_written == 0)
                    {
                        stage->rd_value = cpu->regs[stage->rd].value;
                    }
                    else
                    {
                        if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                        {
                            stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                        }
                        else
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                }
                else
                {   
                    if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                    {
                        stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }



                if (cpu->regs[stage->rs1].is_writing == false)
                {
                    if (cpu->regs[stage->rs1].will_be_written == 0)
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].value;
                    }
                    else
                    {
                        if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                        }
                        else
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                }
                else
                {   
                    if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;        
                    }
                    
                }
            
            }
        

        }



        if (strcmp(stage->opcode, "ld") == 0)
        {
            if (stage->is_imm == false)
            {
                if (cpu->regs[stage->rs1].is_writing == false)
                {
                    if (cpu->regs[stage->rs1].will_be_written == 0) //no data hazard
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].value;
                    }
                    else //dependent
                    {   
                        if (stage->rs1 == stage->rd && cpu->regs[stage->rs1].will_be_written == 1)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].value;
                        }
                        else if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                        {
                            stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                        }
                        else //no forwarding
                        {
                            stage->stall = 1;
                            //cpu->data_hazard_count++;
                        }
                    }
                
                }
                else // register is writing back 
                {
                    if (cpu->regs[stage->rs1].forwarding > 0 && cpu->regs[stage->rs1].forwarding_control == true)
                    {
                        stage->rs1_value = cpu->regs[stage->rs1].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++;
                    }
                }
            }
        }
        
        if (is_branch(stage->opcode))
        {
            if (cpu->regs[stage->rd].is_writing == false)
            {
                if (cpu->regs[stage->rd].will_be_written == 0)
                {
                    stage->rd_value = cpu->regs[stage->rd].value;
                }
                else
                {
                    if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                    {
                        stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                    }
                    else
                    {
                        stage->stall = 1;
                        //cpu->data_hazard_count++; 
                    }
                }
            }
            else
            {   
                if (cpu->regs[stage->rd].forwarding > 0 && cpu->regs[stage->rd].forwarding_control == true)
                {
                    stage->rd_value = cpu->regs[stage->rd].forwarding_value;
                }
                else
                {
                    stage->stall = 1;
                    //cpu->data_hazard_count++;
                }
            }
        }
        
        if (stage->stall == 0)
        {   
            cpu->stage[ADD] = cpu->stage[RR];
            stage->has_instr = 0;
        }
            
    }
}

void add(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[ADD];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "set") == 0 || strcmp(stage->opcode, "add") == 0 || strcmp(stage->opcode, "sub") == 0 || strcmp(stage->opcode, "mul") == 0 || strcmp(stage->opcode, "div") == 0)
        {
            cpu->regs[stage->rd].forwarding_control = false;
        }
        if (strcmp(stage->opcode, "add") == 0)
        {   
            if (stage->is_imm == true)
            {
                stage->buffer = stage->rs1_value + stage->imm;
            }
            else
            {
                stage->buffer = stage->rs1_value + stage->rs2_value;
            }
            cpu->regs[stage->rd].forwarding++ ;
            cpu->regs[stage->rd].forwarding_value = stage->buffer;
            cpu->regs[stage->rd].forwarding_control = true;
        }

        if (strcmp(stage->opcode, "sub") == 0)
        {
            if (stage->is_imm == true)
            {
                stage->buffer = stage->rs1_value - stage->imm;
            }
            else
            {
                stage->buffer = stage->rs1_value - stage->rs2_value;
            }
            cpu->regs[stage->rd].forwarding++ ;
            cpu->regs[stage->rd].forwarding_value = stage->buffer;
            cpu->regs[stage->rd].forwarding_control = true;
        }
        if (strcmp(stage->opcode, "set") == 0)
        {
            stage->buffer = stage->imm;
            cpu->regs[stage->rd].forwarding++ ;
            cpu->regs[stage->rd].forwarding_value = stage->buffer;
            cpu->regs[stage->rd].forwarding_control = true;
        }
        if (is_branch(stage->opcode))
        {
            stage->buffer = stage->imm;
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
            if (stage->is_imm == true)
            {
                stage->buffer = stage->rs1_value * stage->imm;
            }
            else
            {
                stage->buffer = stage->rs1_value * stage->rs2_value;
            }
            cpu->regs[stage->rd].forwarding++;
            cpu->regs[stage->rd].forwarding_value = stage->buffer;
            cpu->regs[stage->rd].forwarding_control = true;
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
            if (stage->is_imm == true)
            {
                stage->buffer = stage->rs1_value / stage->imm;
            }
            else
            {
                stage->buffer = stage->rs1_value / stage->rs2_value;
            }
            cpu->regs[stage->rd].forwarding++;
            cpu->regs[stage->rd].forwarding_value = stage->buffer;
            cpu->regs[stage->rd].forwarding_control = true;
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
    if (strcmp(stage->opcode, "set") == 0 || strcmp(stage->opcode, "add") == 0 || strcmp(stage->opcode, "sub") == 0 || strcmp(stage->opcode, "mul") == 0 || strcmp(stage->opcode, "div") == 0)
    {
        if (cpu->regs[stage->rd].forwarding > 0)
        {
            cpu->regs[stage->rd].forwarding --;
        }
    }
    
    if ((stage->has_instr) && (!stage->stall))
    {   
        if (is_branch(stage->opcode) == false)
        {
            if (stage->if_branch == true)
            {   
                int pt_index = (stage->pc)/4 % 16;
                int target = cpu->btb[pt_index].target;

                if (stage->if_taken == true && target != (stage->pc)+4 )
                {   
                    cpu->updated_pc = (stage->pc)+4;
                    PipelineStage* squash_stage_div = &cpu->stage[DIV];
                    PipelineStage* squash_stage_mul = &cpu->stage[MUL];
                    PipelineStage* squash_stage_add = &cpu->stage[ADD];
                    PipelineStage* squash_stage_rr = &cpu->stage[RR];

                    PipelineStage* squash_stage_ia = &cpu->stage[IA];
                    PipelineStage* squash_stage_id = &cpu->stage[ID];
                    PipelineStage* squash_stage_if = &cpu->stage[IF];

                    if (squash_stage_div->has_instr == 1)
                    {
                        if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "div") == 0 || strcmp(squash_stage_div->opcode, "set") == 0 || strcmp(squash_stage_div->opcode, "ld") == 0)
                        {
                            cpu->regs[squash_stage_div->rd].will_be_written--;
                        }
                    }
                    if (squash_stage_mul->has_instr == 1)
                    {
                        if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "mul") == 0 || strcmp(squash_stage_mul->opcode, "div") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0 || strcmp(squash_stage_mul->opcode, "ld") == 0)
                        {
                            cpu->regs[squash_stage_mul->rd].will_be_written--;
                        }
                    }
                    if (squash_stage_add->has_instr == 1)
                    {
                        if (strcmp(squash_stage_add->opcode, "add") == 0 || strcmp(squash_stage_add->opcode, "sub") == 0 || strcmp(squash_stage_add->opcode, "mul") == 0 || strcmp(squash_stage_add->opcode, "div") == 0 || strcmp(squash_stage_add->opcode, "set") == 0 || strcmp(squash_stage_add->opcode, "ld") == 0)
                        {
                            cpu->regs[squash_stage_add->rd].will_be_written--;
                        }
                    }
                    if (squash_stage_rr->has_instr == 1)
                    {
                        if (strcmp(squash_stage_rr->opcode, "add") == 0 || strcmp(squash_stage_rr->opcode, "sub") == 0 || strcmp(squash_stage_rr->opcode, "mul") == 0 || strcmp(squash_stage_rr->opcode, "div") == 0 || strcmp(squash_stage_rr->opcode, "set") == 0 || strcmp(squash_stage_rr->opcode, "ld") == 0)
                        {
                            cpu->regs[squash_stage_rr->rd].will_be_written--;
                        }
                    }
                    squash_stage_div->has_instr = 0;
                    squash_stage_mul->has_instr = 0;
                    squash_stage_add->has_instr = 0;
                    squash_stage_rr->has_instr = 0;
                    squash_stage_ia->has_instr = 0;
                    squash_stage_id->has_instr = 0;
                    squash_stage_if->has_instr = 0;
                }
            

            }
        }

        if (is_branch(stage->opcode) == true)
        {   
            cpu->write_btb = stage->buffer; //update btb target
            cpu->pt_index = (stage->pc)/4 % 16; //pt update index
            cpu->pc_tag = (stage->pc)/64;

            int z = stage->rd_value;
            cpu->branch = 0; //branch not taken
            if (strcmp(stage->opcode, "bez") == 0 && z == 0) 
            {
                cpu->branch = 1; //branch taken
            }
            if (strcmp(stage->opcode, "bgez") == 0 && z >= 0) 
            {
                cpu->branch = 1;
            }
            if (strcmp(stage->opcode, "blez") == 0 && z <= 0) 
            {
                cpu->branch = 1;
            }
            if (strcmp(stage->opcode, "bgtz") == 0 && z > 0) 
            {
                cpu->branch = 1;
            }
            if (strcmp(stage->opcode, "bltz") == 0 && z < 0) 
            {
                cpu->branch = 1;
            }
            if (cpu->branch == 1) 
            {               
                cpu->updated_pt = 1; //pt will plus 1
                
                if (stage->if_branch == true)
                {   
                    int pt_index = (stage->pc)/4 % 16;
                    int target = cpu->btb[pt_index].target;                 
                    if (stage->if_taken == true)
                    {
                        if (stage->buffer != target)
                        {
                            cpu->updated_pc = stage->buffer;
                            PipelineStage* squash_stage_div = &cpu->stage[DIV];
                            PipelineStage* squash_stage_mul = &cpu->stage[MUL];
                            PipelineStage* squash_stage_add = &cpu->stage[ADD];
                            PipelineStage* squash_stage_rr = &cpu->stage[RR];

                            PipelineStage* squash_stage_ia = &cpu->stage[IA];
                            PipelineStage* squash_stage_id = &cpu->stage[ID];
                            PipelineStage* squash_stage_if = &cpu->stage[IF];
                            if (squash_stage_div->has_instr == 1)
                            {
                                if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "div") == 0 || strcmp(squash_stage_div->opcode, "set") == 0 || strcmp(squash_stage_div->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_div->rd].will_be_written--;
                                }
                                if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "set") == 0)
                                {
                                    cpu->regs[squash_stage_div->rd].forwarding--;
                                }
                            }
                            if (squash_stage_mul->has_instr == 1)
                            {
                                if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "mul") == 0 || strcmp(squash_stage_mul->opcode, "div") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0 || strcmp(squash_stage_mul->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_mul->rd].will_be_written--;
                                }
                                if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0)
                                {
                                    cpu->regs[squash_stage_mul->rd].forwarding--;
                                }                                
                            }
                            if (squash_stage_add->has_instr == 1)
                            {
                                if (strcmp(squash_stage_add->opcode, "add") == 0 || strcmp(squash_stage_add->opcode, "sub") == 0 || strcmp(squash_stage_add->opcode, "mul") == 0 || strcmp(squash_stage_add->opcode, "div") == 0 || strcmp(squash_stage_add->opcode, "set") == 0 || strcmp(squash_stage_add->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_add->rd].will_be_written--;
                                }
                            }
                            if (squash_stage_rr->has_instr == 1)
                            {
                                if (strcmp(squash_stage_rr->opcode, "add") == 0 || strcmp(squash_stage_rr->opcode, "sub") == 0 || strcmp(squash_stage_rr->opcode, "mul") == 0 || strcmp(squash_stage_rr->opcode, "div") == 0 || strcmp(squash_stage_rr->opcode, "set") == 0 || strcmp(squash_stage_rr->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_rr->rd].will_be_written--;
                                }
                            }
                            squash_stage_div->has_instr = 0;
                            squash_stage_mul->has_instr = 0;
                            squash_stage_add->has_instr = 0;
                            squash_stage_rr->has_instr = 0;
                            squash_stage_ia->has_instr = 0;
                            squash_stage_id->has_instr = 0;
                            squash_stage_if->has_instr = 0;
                        }
                    }
                    else //predict wrongly
                    {   
                        if ((stage->pc)+4 != stage->buffer)
                        {        
                            cpu->updated_pc = stage->buffer;
                            PipelineStage* squash_stage_div = &cpu->stage[DIV];
                            PipelineStage* squash_stage_mul = &cpu->stage[MUL];
                            PipelineStage* squash_stage_add = &cpu->stage[ADD];
                            PipelineStage* squash_stage_rr = &cpu->stage[RR];

                            PipelineStage* squash_stage_ia = &cpu->stage[IA];
                            PipelineStage* squash_stage_id = &cpu->stage[ID];
                            PipelineStage* squash_stage_if = &cpu->stage[IF];
                            if (squash_stage_div->has_instr == 1)
                            {
                                if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "div") == 0 || strcmp(squash_stage_div->opcode, "set") == 0 || strcmp(squash_stage_div->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_div->rd].will_be_written--;
                                }
                                if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "set") == 0)
                                {
                                    cpu->regs[squash_stage_div->rd].forwarding--;
                                }
                            }
                            if (squash_stage_mul->has_instr == 1)
                            {
                                if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "mul") == 0 || strcmp(squash_stage_mul->opcode, "div") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0 || strcmp(squash_stage_mul->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_mul->rd].will_be_written--;
                                }
                                if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0)
                                {
                                    cpu->regs[squash_stage_mul->rd].forwarding--;
                                }
                            }
                            if (squash_stage_add->has_instr == 1)
                            {
                                if (strcmp(squash_stage_add->opcode, "add") == 0 || strcmp(squash_stage_add->opcode, "sub") == 0 || strcmp(squash_stage_add->opcode, "mul") == 0 || strcmp(squash_stage_add->opcode, "div") == 0 || strcmp(squash_stage_add->opcode, "set") == 0 || strcmp(squash_stage_add->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_add->rd].will_be_written--;
                                }
                            }
                            if (squash_stage_rr->has_instr == 1)
                            {
                                if (strcmp(squash_stage_rr->opcode, "add") == 0 || strcmp(squash_stage_rr->opcode, "sub") == 0 || strcmp(squash_stage_rr->opcode, "mul") == 0 || strcmp(squash_stage_rr->opcode, "div") == 0 || strcmp(squash_stage_rr->opcode, "set") == 0 || strcmp(squash_stage_rr->opcode, "ld") == 0)
                                {
                                    cpu->regs[squash_stage_rr->rd].will_be_written--;
                                }
                            }
                            squash_stage_div->has_instr = 0;
                            squash_stage_mul->has_instr = 0;
                            squash_stage_add->has_instr = 0;
                            squash_stage_rr->has_instr = 0;
                            squash_stage_ia->has_instr = 0;
                            squash_stage_id->has_instr = 0;
                            squash_stage_if->has_instr = 0;
                        }
                    }
                }
                else
                {   
                    if ((stage->pc)+4 != stage->buffer)
                    {
                        cpu->updated_pc = stage->buffer;
                        PipelineStage* squash_stage_div = &cpu->stage[DIV];
                        PipelineStage* squash_stage_mul = &cpu->stage[MUL];
                        PipelineStage* squash_stage_add = &cpu->stage[ADD];
                        PipelineStage* squash_stage_rr = &cpu->stage[RR];

                        PipelineStage* squash_stage_ia = &cpu->stage[IA];
                        PipelineStage* squash_stage_id = &cpu->stage[ID];
                        PipelineStage* squash_stage_if = &cpu->stage[IF];
                        if (squash_stage_div->has_instr == 1)
                        {
                            if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "div") == 0 || strcmp(squash_stage_div->opcode, "set") == 0 || strcmp(squash_stage_div->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_div->rd].will_be_written--;
                            }
                            if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "set") == 0)
                            {
                                cpu->regs[squash_stage_div->rd].forwarding--;
                            }
                        }
                        if (squash_stage_mul->has_instr == 1)
                        {
                            if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "mul") == 0 || strcmp(squash_stage_mul->opcode, "div") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0 || strcmp(squash_stage_mul->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_mul->rd].will_be_written--;
                            }
                            if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0)
                            {
                                cpu->regs[squash_stage_mul->rd].forwarding--;
                            }
                        }
                        if (squash_stage_add->has_instr == 1)
                        {
                            if (strcmp(squash_stage_add->opcode, "add") == 0 || strcmp(squash_stage_add->opcode, "sub") == 0 || strcmp(squash_stage_add->opcode, "mul") == 0 || strcmp(squash_stage_add->opcode, "div") == 0 || strcmp(squash_stage_add->opcode, "set") == 0 || strcmp(squash_stage_add->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_add->rd].will_be_written--;
                            }
                        }
                        if (squash_stage_rr->has_instr == 1)
                        {
                            if (strcmp(squash_stage_rr->opcode, "add") == 0 || strcmp(squash_stage_rr->opcode, "sub") == 0 || strcmp(squash_stage_rr->opcode, "mul") == 0 || strcmp(squash_stage_rr->opcode, "div") == 0 || strcmp(squash_stage_rr->opcode, "set") == 0 || strcmp(squash_stage_rr->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_rr->rd].will_be_written--;
                            }
                        }
                        squash_stage_div->has_instr = 0;
                        squash_stage_mul->has_instr = 0;
                        squash_stage_add->has_instr = 0;
                        squash_stage_rr->has_instr = 0;
                        squash_stage_ia->has_instr = 0;
                        squash_stage_id->has_instr = 0;
                        squash_stage_if->has_instr = 0;
                    }                
                }
                
            }
            if (cpu->branch == 0)
            {   
                cpu->updated_pt = -1;
                
                if (stage->if_branch == true)
                {   
                    int pt_index = (stage->pc)/4 % 16;
                    int target = cpu->btb[pt_index].target;
                    
                    if (stage->if_taken == true && target != ((stage->pc)+4))
                    {   
                        cpu->updated_pc = (stage->pc)+4;
                        PipelineStage* squash_stage_div = &cpu->stage[DIV];
                        PipelineStage* squash_stage_mul = &cpu->stage[MUL];
                        PipelineStage* squash_stage_add = &cpu->stage[ADD];
                        PipelineStage* squash_stage_rr = &cpu->stage[RR];

                        PipelineStage* squash_stage_ia = &cpu->stage[IA];
                        PipelineStage* squash_stage_id = &cpu->stage[ID];
                        PipelineStage* squash_stage_if = &cpu->stage[IF];
                        if (squash_stage_div->has_instr == 1)
                        {
                            if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "div") == 0 || strcmp(squash_stage_div->opcode, "set") == 0 || strcmp(squash_stage_div->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_div->rd].will_be_written--;
                            }
                            if (strcmp(squash_stage_div->opcode, "add") == 0 || strcmp(squash_stage_div->opcode, "sub") == 0 || strcmp(squash_stage_div->opcode, "mul") == 0 || strcmp(squash_stage_div->opcode, "set") == 0)
                            {
                                cpu->regs[squash_stage_div->rd].forwarding--;
                            }
                        }
                        if (squash_stage_mul->has_instr == 1)
                        {
                            if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "mul") == 0 || strcmp(squash_stage_mul->opcode, "div") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0 || strcmp(squash_stage_mul->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_mul->rd].will_be_written--;
                            }
                            if (strcmp(squash_stage_mul->opcode, "add") == 0 || strcmp(squash_stage_mul->opcode, "sub") == 0 || strcmp(squash_stage_mul->opcode, "set") == 0)
                            {
                                cpu->regs[squash_stage_mul->rd].forwarding--;
                            }
                        }
                        if (squash_stage_add->has_instr == 1)
                        {
                            if (strcmp(squash_stage_add->opcode, "add") == 0 || strcmp(squash_stage_add->opcode, "sub") == 0 || strcmp(squash_stage_add->opcode, "mul") == 0 || strcmp(squash_stage_add->opcode, "div") == 0 || strcmp(squash_stage_add->opcode, "set") == 0 || strcmp(squash_stage_add->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_add->rd].will_be_written--;
                            }
                        }
                        if (squash_stage_rr->has_instr == 1)
                        {
                            if (strcmp(squash_stage_rr->opcode, "add") == 0 || strcmp(squash_stage_rr->opcode, "sub") == 0 || strcmp(squash_stage_rr->opcode, "mul") == 0 || strcmp(squash_stage_rr->opcode, "div") == 0 || strcmp(squash_stage_rr->opcode, "set") == 0 || strcmp(squash_stage_rr->opcode, "ld") == 0)
                            {
                                cpu->regs[squash_stage_rr->rd].will_be_written--;
                            }
                        }
                        squash_stage_div->has_instr = 0;
                        squash_stage_mul->has_instr = 0;
                        squash_stage_add->has_instr = 0;
                        squash_stage_rr->has_instr = 0;
                        squash_stage_ia->has_instr = 0;
                        squash_stage_id->has_instr = 0;
                        squash_stage_if->has_instr = 0;
                    }
                    else
                    {

                    }
                }
                else
                {

                }                
            }
        }
        if (strcmp(stage->opcode, "ret") == 0)
        {

        }
        cpu->stage[MEM] = cpu->stage[BR];
        stage->has_instr = 0;
    }
}

void memory_1(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[MEM];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "ld") == 0 || strcmp(stage->opcode, "st") == 0)
        {
            if (stage->is_imm)
            {
                stage->buffer = stage->imm;
            }
            else
            {
                stage->buffer = stage->rs1_value;
            }
        }
        cpu->stage[MEM2] = cpu->stage[MEM];
        stage->has_instr = 0;
    }
}


void memory_2(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[MEM2];
    if ((stage->has_instr) && (!stage->stall))
    {
        if (strcmp(stage->opcode, "ld") == 0)
        {                
            stage->buffer = cpu->data_memory[stage->buffer/4];

        }
        if (strcmp(stage->opcode, "st") == 0)
        {
            cpu->data_memory[stage->buffer/4] = stage->rd_value;
        }
        
        cpu->stage[WB] = cpu->stage[MEM2];
        stage->has_instr = 0;   
    }
    
}





void writeback(CPU* cpu)
{
    PipelineStage* stage = &cpu->stage[WB];
    if ((stage->has_instr) && (!stage->stall))
    {   
        //printf("%s\n", stage->opcode);
        if (is_branch(stage->opcode) || strcmp(stage->opcode, "st") == 0)
        {
            
        }
        else
        {
            if (stage->rd >=0 && stage->rd <16)
            {
                cpu->regs[stage->rd].value = stage->buffer;
                cpu->regs[stage->rd].is_writing = true;
                cpu->regs[stage->rd].will_be_written-- ;
                cpu->updated_reg_id = stage->rd;
            }
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
