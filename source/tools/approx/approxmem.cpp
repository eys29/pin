// Elise Song
// 11/2024
// Starter code from pinatrace

/**
build:
    make obj-intel64/approxmem.so TARGET=intel64

run:
    ../../../pin -t obj-intel64/approxmem.so -- ./memtest
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

FILE *trace;
FILE *stats;
FILE *instrdump;
ADDRINT smallest_addr;
ADDRINT largest_addr;
int start = 1;
int num_reads = 0;
int num_writes = 0;
int is_tracing = 0;

// helper function that collects statistics about addr range of mem instructions
VOID addr_stats(ADDRINT addr)
{
    if (start)
    {
        largest_addr = addr;
        smallest_addr = addr;
        start = 0;
    }
    else
    {
        if (addr > largest_addr)
            largest_addr = addr;
        if (addr < smallest_addr)
            smallest_addr = addr;
    }
}

// prints addr and data of mem read
VOID mem_r(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    num_reads += 1;
    char data[size];
    PIN_SafeCopy(&data, (VOID *)addr, size);
    fprintf(trace, "R %dB from addr %lx: \t", size, addr);
    for (int i = 0; i < (int)size; i++)
        fprintf(trace, "%02hhx ", data[size-1-i]);
    fprintf(trace, "\n");
    addr_stats(addr);
}

ADDRINT w_addr;
UINT32 w_size;

//stores write addr and data size for mem_w_data to print later 
//(addr and data size is not available for after instruction instrumentation)
VOID mem_w_info(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    num_writes += 1;
    addr_stats(addr);
    w_addr = addr;
    w_size = size;
}

//prints addr and data of mem write
VOID mem_w_data()
{
    if (!is_tracing)
        return;
    char data[w_size];
    PIN_SafeCopy(&data, (VOID *)w_addr, w_size);
    fprintf(trace, "W %dB to   addr %lx: \t", w_size, w_addr);
    for (int i = 0; i < (int)w_size; i++)
        fprintf(trace, "%02hhx ", data[w_size-1-i]);
    fprintf(trace, "\n");
}

// if data is not available, just print the address and size
VOID mem_w_nodata(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    num_writes += 1;
    addr_stats(addr);
    fprintf(trace, "W %dB to   addr %lx \n", size, addr);
}

// debug function that prints assembly instructions
VOID print_instr(UINT32 opcode, REG reg0, REG reg1) //, UINT32 reg2)
{
    fprintf(instrdump, "%s %s, %s\n", OPCODE_StringShort(opcode).c_str(), REG_StringShort(reg0).c_str(), REG_StringShort(reg1).c_str());
}

// set is_tracing boolean to !is_tracing
VOID encounter_magic_instr()
{
    is_tracing = (is_tracing + 1) % 2;
    fprintf(stats, "is_tracing=%d\n", is_tracing);
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

    if (INS_Opcode(ins) == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EAX && INS_OperandReg(ins, 1)){
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)encounter_magic_instr, IARG_END);
    }

    // debug: prints assembly instrs
    // if (INS_OperandCount(ins) == 2)
    // {
    //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)print_instr, IARG_UINT32, INS_Opcode(ins), IARG_UINT32, REG(INS_OperandReg(ins, 0)), IARG_UINT32, REG(INS_OperandReg(ins, 1)), IARG_END); // INS_OperandReg(ins, 2),
    // }

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
        // Note that in some architectures a single memory operand can be
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            //  pinatrace instrument
            // INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memOpWrite, IARG_MEMORYOP_EA, memOp, IARG_MEMORYOP_SIZE, memOp, IARG_END);

            // my instrument
            if (INS_IsValidForIpointAfter(ins))
            {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_w_info, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
                INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)mem_w_data, IARG_END);
            }
            else
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)mem_w_nodata, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
    fprintf(stats, "largest addr: %lx \n", largest_addr);
    fprintf(stats, "smallest addr: %lx \n", smallest_addr);
    fprintf(stats, "addr space size: %ld B \n", (largest_addr - smallest_addr));
    fprintf(stats, "# reads: %d\n", num_reads);
    fprintf(stats, "# writes: %d\n", num_writes);
    fprintf(stats, "total mem instrs: %d\n", num_reads + num_writes);

    fclose(stats);
    fclose(instrdump);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of memory addresses\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();

    trace = fopen("approxtrace.out", "w");
    stats = fopen("approxstats.out", "w");
    instrdump = fopen("approxinstrs.out", "w");

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
