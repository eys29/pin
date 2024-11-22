// Elise Song
// 11/2024
// Starter code from pinatrace

/**
build:
    make obj-intel64/approxmem.so TARGET=intel64

run:
    ../../../pin -t obj-intel64/approxmem.so -- test
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

FILE *trace;
FILE *stats;
ADDRINT smallest_addr;
ADDRINT largest_addr;
int start = 1;
int num_reads = 0;
int num_writes = 0;
int is_tracing = 0;

// from pinatrace /////////////////////////////////////////////
// Print a memory read record
VOID memOpRead(VOID *addr, UINT32 size)
{
    fprintf(trace, "op R %dB from addr %p \n", size, addr);
}

// Print a memory write record
VOID memOpWrite(VOID *addr, UINT32 size)
{
    fprintf(trace, "op W %dB from addr %p \n", size, addr);
}

///////////////////////////////////////////////////////////////

// helper function to collect statistics about addr range
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

VOID mem_r(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    char data[size];
    PIN_SafeCopy(&data, (VOID *)addr, size);
    fprintf(trace, "R %dB from addr %lx: \t", size, addr);
    for (int i = 0; i < (int)size; i++)
        fprintf(trace, "%x", data[i]);
    fprintf(trace, "\n");
    addr_stats(addr);
}

ADDRINT w_addr;
UINT32 w_size;
VOID mem_w_info(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    num_reads += 1;
    addr_stats(addr);
    w_addr = addr;
    w_size = size;
}

VOID mem_w_data()
{
    if (!is_tracing)
        return;
    char data[w_size];
    PIN_SafeCopy(&data, (VOID *)w_addr, w_size);
    fprintf(trace, "W %dB to   addr %lx: \t", w_size, w_addr);
    for (int i = 0; i < (int)w_size; i++)
        fprintf(trace, "%x", data[i]);
    fprintf(trace, "\n");
}

VOID mem_w_nodata(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    num_writes += 1;
    addr_stats(addr);
    fprintf(trace, "W %dB to   addr %lx \n", size, addr);
}

// set is_tracing boolean to !is_tracing
VOID set_tracing()
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
    
    UINT32 opcode = INS_Opcode(ins);
    // Handle magic instructions
    if (opcode == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_ECX)// && INS_OperandReg(ins, 1) == REG_ECX)
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)set_tracing, IARG_END);
        return;

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
    fprintf(stats, "addr space size: %lf TB \n", (largest_addr - smallest_addr) / pow(2, 40));
    fprintf(stats, "# reads: %d\n", num_reads);
    fprintf(stats, "# writes: %d\n", num_writes);
    fprintf(stats, "total mem instrs: %d\n", num_reads + num_writes);
    fclose(stats);
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

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
