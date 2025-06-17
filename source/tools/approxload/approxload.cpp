// Elise Song
// 2/2025
// Starter code from pinatrace

/**
build:
make obj-intel64/approxload.so TARGET=intel64 

run:
../../../pin -t obj-intel64/approxload.so -- ./memtest

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

FILE *debug;

int instr_counter = 0;
int is_tracing = 0;

int instr_num = -1;
int bit_loc = -1;
int rand_setting = 1;



VOID mem_instr(ADDRINT addr, UINT32 size, char rw)
{
    uint8_t data[size];
    PIN_SafeCopy(&data, (VOID *)addr, size);
    fprintf(debug, "\tBEFORE: #%d %c %d %lu [ ", instr_counter, rw, size, (unsigned long)addr);
    for (int i = 0; i < (int)size; i++)
        fprintf(debug, "%u ", data[i]);
    fprintf(debug, "]\n");
    // for blocks 0...size/2, set them to 0 to truncate the data
    for (int i = 0; i < (int) size / 2; i++){
        data[i] = 0;
    }
    PIN_SafeCopy((VOID *)addr, &data, size);
    //for debugging
    uint8_t check_write[size];
    PIN_SafeCopy(&check_write, (VOID *)addr, size);

    fprintf(debug, "\tAFTER: #%d %c %d %lu [ ", instr_counter, rw, size, (unsigned long)addr);
    for (int i = 0; i < (int)size; i++)
        fprintf(debug, "%u ", check_write[i]);
    fprintf(debug, "]\n");
    
    

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
    } else if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EBX && INS_OperandReg(ins, 1) == REG_EBX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    } else if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_ECX && INS_OperandReg(ins, 1) == REG_ECX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    } else if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EDX && INS_OperandReg(ins, 1) == REG_EDX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    } 


    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            // pinatrace instrument
            // INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memOpRead, IARG_MEMORYOP_EA, memOp, IARG_MEMORYOP_SIZE, memOp, IARG_END);

            // my instrument
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_r, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fclose(debug);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool implements approximate load on annotated instructions\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();

    debug = fopen("approxload.debug", "w");

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
