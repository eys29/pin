// Elise Song
// 2/2025
// Starter code from pinatrace

/**
build:
make obj-intel64/bitflip.so TARGET=intel64 

run:
../../../pin -t obj-intel64/bitflip.so -- ./memtest 5 4 1

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
    if (instr_counter == instr_num){
        uint8_t data[size];
        PIN_SafeCopy(&data, (VOID *)addr, size);
        fprintf(debug, "\tBEFORE: #%d %c %d %lu [ ", instr_counter, rw, size, (unsigned long)addr);
        for (int i = 0; i < (int)size; i++)
            fprintf(debug, "%u ", data[i]);
        fprintf(debug, "]\n");
        UINT32 block;
        int offset;
        if (rand_setting == 0){
            block = bit_loc / 8; // 8 bits
            offset = bit_loc % 8;
            if (block < size){
                
                uint8_t bitmask = 1 << offset;
                data[block] = data[block] ^ bitmask;
                
            }
        } if (rand_setting == 1){
            srand(time(NULL));
            for (int i = 0; i < bit_loc; i++){
                int random_loc = rand() % (8);
                block = random_loc / 8; // 8 bits
                offset = random_loc % 8;
                uint8_t bitmask = 1 << offset;
                data[block] = data[block] ^ bitmask;
            }
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
    instr_counter += 1;

}

// prints addr and data of mem read
VOID mem_r(ADDRINT r_addr, UINT32 r_size)
{
    if (!is_tracing)
        return;
    mem_instr(r_addr, r_size, 'R');
    
}

ADDRINT w_addr;
UINT32 w_size;

// stores write addr and data size for mem_w_data to print later
//(addr and data size is not available for after instruction instrumentation)
VOID mem_w_info(ADDRINT addr, UINT32 size)
{
    if (!is_tracing)
        return;
    w_addr = addr;
    w_size = size;
}

// prints addr and data of mem write
VOID mem_w_data()
{
    if (!is_tracing)
        return;
    mem_instr(w_addr, w_size, 'W');

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
        // if (INS_MemoryOperandIsWritten(ins, memOp))
        // {
        //     //  pinatrace instrument
        //     // INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)memOpWrite, IARG_MEMORYOP_EA, memOp, IARG_MEMORYOP_SIZE, memOp, IARG_END);

        //     // my instrument
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
    fclose(debug);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool flips specified bit of specificed read instruction\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return Usage();

    if (argc == 9){
        instr_num = atoi(argv[7]);
        bit_loc = atoi(argv[8]);
    } else {
        return Usage();
    }

    debug = fopen("bitflip.debug", "w");
    fprintf(debug, "instr %d - bit %d\n", instr_num, bit_loc);

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
