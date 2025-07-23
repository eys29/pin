// Elise Song
// 2/2025
// Starter code from pinatrace

/**
build:
make obj-intel64/bucketbitflip.so TARGET=intel64 

run:
../../../pin -t obj-intel64/bucketbitflip.so -- ./memtest 5 4 1

*/
/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include "pin.H"
#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

ifstream meminfo;
int instr_counter = 0;
int is_tracing = 0;

int num_bitflips = 0;

vector<int> load_ids;

VOID mem_instr(ADDRINT addr, UINT32 size, char rw)
{
    if (binary_search(load_ids.cbegin(), load_ids.cend(), instr_counter)){
        uint8_t data[size];
        PIN_SafeCopy(&data, (VOID *)addr, size);
        // fprintf(debug, "\tBEFORE: #%d %c %d %lu [ ", instr_counter, rw, size, (unsigned long)addr);
        // for (int i = 0; i < (int)size; i++)
        //     fprintf(debug, "%u ", data[i]);
        // fprintf(debug, "]\n");
        UINT32 block;
        int offset;
        int random_loc;
        for (int i = 0; i < num_bitflips; i++){
            random_loc = rand() % (8);
            block = random_loc / 8; // 8 bits
            offset = random_loc % 8;
            uint8_t bitmask = 1 << offset;
            data[block] = data[block] ^ bitmask;
        }
    
        PIN_SafeCopy((VOID *)addr, &data, size);
        //for debugging
        uint8_t check_write[size];
        PIN_SafeCopy(&check_write, (VOID *)addr, size);
        

        // fprintf(debug, "\tAFTER: #%d %c %d %lu [ ", instr_counter, rw, size, (unsigned long)addr);
        // for (int i = 0; i < (int)size; i++)
        //     fprintf(debug, "%u ", check_write[i]);
        // fprintf(debug, "]\n");
        
    }
    instr_counter += 1;

}

// prints addr and data of mem read
VOID mem_r(ADDRINT r_addr, UINT32 r_size)
{
    if (!is_tracing)
        return;
    mem_instr(r_addr, r_size, 'R');
    
}

// set is_tracing boolean to !is_tracing
VOID encounter_magic_instr()
{
    is_tracing = (is_tracing + 1) % 2;
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
    // prefixed instructions appear as predicated instructions in Pin.

    // Handle magic instructions

    if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EAX && INS_OperandReg(ins, 1) == REG_EAX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    } else if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_ECX && INS_OperandReg(ins, 1) == REG_ECX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    }

    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_r, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    meminfo.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool flips # of random bits of a bucket of load instructions \n"  + KNOB_BASE::StringKnobSummary() + "\n");
    printf("requires 4 arguments: #random_bitflips, bucket_start, bucket_size, meminfo_filename\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();



    if (argc != 12){
        return Usage();
    }
    srand(time(NULL));            
    num_bitflips = atoi(argv[7]);
    int bucket_start = atoi(argv[8]);
    int bucket_size = atoi(argv[9]);
    int prob = atoi(argv[10]);
    char *filename = argv[11];


    

    meminfo.open(filename);
    std::string str; 
    int counter = 0;
    while (getline(meminfo, str))
    {
        if (counter >= bucket_start && counter < bucket_start + bucket_size){
            auto pos = str.find(" ");
            // distribution here
            if ((rand() % 100) < prob){
                load_ids.push_back(stoi(str.substr(0, pos)));
            }  
        }
        counter++;
    }

    // for (int i : load_ids) cout << i << " ";

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
