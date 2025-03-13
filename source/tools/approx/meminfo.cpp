// Elise Song
// 2/2025
// Starter code from pinatrace

/**
build:
    make obj-intel64/meminfo.so TARGET=intel64

run:
    ../../../pin -t obj-intel64/meminfo.so -- ./memtest
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

FILE *out;
FILE *assembly;
FILE *info;
int mem_instr_counter = 0;
int is_tracing = 0;

int last_instr = 100;



// debug function that prints assembly instructions
VOID print_instr(UINT32 opcode, REG reg0, REG reg1) //, UINT32 reg2)
{
    fprintf(assembly, "%d %s %s, %s\n", mem_instr_counter, OPCODE_StringShort(opcode).c_str(), REG_StringShort(reg0).c_str(), REG_StringShort(reg1).c_str());
}

//helper function for mem_r and mem_w
VOID mem_instr(ADDRINT addr, UINT32 size)
{
    if (!is_tracing) 
        return;
    if (mem_instr_counter >= last_instr)
    {
        mem_instr_counter += 1;
        return;
    }
    uint8_t data[size];
    PIN_SafeCopy(&data, (VOID *)addr, size);
    // instr_counter | size (bytes) | address 
    fprintf(out, "%d %d %ld\n", mem_instr_counter, size, addr);
    
    mem_instr_counter += 1;
}


// prints addr and data of mem read
VOID mem_r(ADDRINT r_addr, UINT32 r_size)
{
    mem_instr(r_addr, r_size);
}

ADDRINT w_addr;
UINT32 w_size;

// stores write addr and data size for mem_w_data to print later
//(addr and data size is not available for after instruction instrumentation)
VOID mem_w_info(ADDRINT addr, UINT32 size)
{
    if (!is_tracing || mem_instr_counter >= last_instr)
        return;
    w_addr = addr;
    w_size = size;
}

// prints addr and data of mem write
VOID mem_w_data()
{
    mem_instr(w_addr, w_size);
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

    if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EAX && INS_OperandReg(ins, 1))
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
        // Note that in some architectures a single memory operand can be
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        // if (INS_MemoryOperandIsWritten(ins, memOp))
        // {
        //     if (INS_IsValidForIpointAfter(ins))
        //     {
        //         INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_w_info, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
        //         INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)mem_w_data, IARG_END);
        //     }
        // }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(info, "%d", mem_instr_counter);
    fclose(out);
    fclose(assembly);
    fclose(info);

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a read and write instruction information to meminfo.out\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();

    out = fopen("meminfo.out", "w");
    assembly = fopen("meminfo.s", "w");
    info = fopen("meminfo.info", "w");
    

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
