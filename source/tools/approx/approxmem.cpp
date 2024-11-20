// Elise Song
// 11/2024
// Starter code from pinatrace

// build:
//  make all TARGET=intel64

// run:
//  ../../../pin -t obj-intel64/memtrace.so -- test
/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include "pin.H"

FILE *trace;

// from pinatrace
// Print a memory read record
VOID memOpRead(VOID *addr, UINT32 size)
{
    fprintf(trace, "op R %dB from addr %p \n", size, addr);
}

// from pinatrace
// Print a memory write record
VOID memOpWrite(VOID *addr, UINT32 size)
{
    fprintf(trace, "op W %dB from addr %p \n", size, addr);
}

// same as memOpRead
VOID memRead(ADDRINT addr, UINT32 size){
    char data[size];
    PIN_SafeCopy(&data, (VOID *) addr, size);
    fprintf(trace, "\t R %dB from addr %lx: \t %s \n", size, addr, data);
}

// same as memOpWrite
VOID memWrite(ADDRINT addr, UINT32 size){
    fprintf(trace, "\t W %dB from addr %lx \n", size, addr);
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            // from pinatrace
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memOpRead, IARG_MEMORYOP_EA, memOp, IARG_MEMORYOP_SIZE, memOp, IARG_END);
            // my call
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memRead, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
        }
        // Note that in some architectures a single memory operand can be
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            // from pinatrace
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memOpWrite, IARG_MEMORYOP_EA, memOp, IARG_MEMORYOP_SIZE, memOp, IARG_END);
            // my call
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memWrite, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
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

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
