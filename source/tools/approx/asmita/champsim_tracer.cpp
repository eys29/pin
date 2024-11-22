/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "gen_random_mem.cpp"

#define NUM_INSTR_DESTINATIONS 2
#define NUM_INSTR_SOURCES 4

using namespace std;

typedef struct trace_instr_format {
    unsigned long long int ip;  // instruction pointer (program counter) value

    unsigned char is_branch;    // is this branch
    unsigned char branch_taken; // if so, is this taken

    unsigned char destination_registers[NUM_INSTR_DESTINATIONS]; // output registers
    unsigned char source_registers[NUM_INSTR_SOURCES];           // input registers

    unsigned long long int destination_memory[NUM_INSTR_DESTINATIONS]; // output memory
    unsigned long long int source_memory[NUM_INSTR_SOURCES];           // input memory
} trace_instr_format_t;

/* ================================================================== */
// Global variables 
/* ================================================================== */

UINT64 instrCount = 0, mod_access = 0, total_access = 0, camouflage_access = 0;

FILE* out;
FILE* outMOD;

bool output_file_closed = false;
bool tracing_on = false;
int track_store = 0;
int track_load = 0;
bool flag_load = false, flag_store = false, start_set = false, flag_array = false;

unsigned long long int diff = 0, modifier = 0, start_address=0, end_address=0;


trace_instr_format_t curr_instr;
trace_instr_format_t mod_instr;

unsigned int NNodes;
unsigned long long int *lfsr_arr;
long int position = 0;

std::map<unsigned long long int, unsigned long long int> read_set;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool", "o", "champsim.trace", 
        "specify file name for Champsim tracer output");

KNOB<string> KnobOutputFile2(KNOB_MODE_WRITEONCE,  "pintool", "om", "champsim.modified.trace", 
        "specify file name for Champsim tracer modified output");

KNOB<UINT64> KnobSkipInstructions(KNOB_MODE_WRITEONCE, "pintool", "s", "0", 
        "How many instructions to skip before tracing begins");

KNOB<UINT64> KnobSpecifyNodes(KNOB_MODE_WRITEONCE, "pintool", "n", "1", 
        "specify number of nodes in linkedlist/tree/graph; No need for array");

KNOB<UINT64> KnobTraceInstructions(KNOB_MODE_WRITEONCE, "pintool", "t", "100000000", 
        "How many instructions to trace");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool creates a register and memory access trace" << endl 
        << "Specify the output trace file with -o" << endl 
        << "Specify the number of instructions to skip before tracing with -s" << endl
        << "Specify the number of instructions to trace with -t" << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

void BeginInstruction(VOID *ip, UINT32 op_code, VOID *opstring)
{
    //cout << op_code << endl;
    //printf("[%p %u %s ", ip, opcode, (char*)opstring);

    if(instrCount > KnobSkipInstructions.Value()) 
    {
        tracing_on = true;

    	if(instrCount > (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
            tracing_on = false;
    }

    if(!tracing_on) 
        return;

    instrCount++;
    // reset the current instruction
    curr_instr.ip = (unsigned long long int)ip;

    curr_instr.is_branch = 0;
    curr_instr.branch_taken = 0;
    mod_instr.ip = (unsigned long long int)ip;

    mod_instr.is_branch = 0;
    mod_instr.branch_taken = 0;

    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++) 
    {
        curr_instr.destination_registers[i] = 0;
        curr_instr.destination_memory[i] = 0;
        mod_instr.destination_registers[i] = 0;
        mod_instr.destination_memory[i] = 0;
    }

    for(int i=0; i<NUM_INSTR_SOURCES; i++) 
    {
        curr_instr.source_registers[i] = 0;
        curr_instr.source_memory[i] = 0;
        mod_instr.source_registers[i] = 0;
        mod_instr.source_memory[i] = 0;
    }
}

void Camouflage_Access(VOID *ip) {
	if(flag_store) {
		//tracing_on = false;
		flag_store = false;
		//track_store = 0;
		//printf("End MAGIC ROI Exit EBX -----------\n");
    		//cout << " ip end of magic instruction : " << hex << curr_instr.ip <<"\n";
	}
	else
	{
		//tracing_on = true;
		flag_store = true;
		track_store += 1;
		//printf("Enter MAGIC ROI Begin EBX -----------\n");
    		//cout << " ip beginning of magic instruction : " << hex << curr_instr.ip <<"\n";
	}
}
void Camouflage_Array(VOID *ip) {
	if(flag_array) {
		//tracing_on = false;
		flag_array = false;
		//track_store = 0;
		//printf("End MAGIC ROI Exit EBX -----------\n");
    		//cout << " ip end of magic instruction : " << hex << curr_instr.ip <<"\n";
	}
	else
	{
		//tracing_on = true;
		flag_array = true;
		//printf("Enter MAGIC ROI Begin EBX -----------\n");
    		//cout << " ip beginning of magic instruction : " << hex << curr_instr.ip <<"\n";
	}
}

void Trace_Control(VOID *ip) {
	if(tracing_on) {
		tracing_on = false;
		//tracing_on = false;
		//printf("End MAGIC ROI Exit -----------\n");
    		//cout << " ip end of magic instruction : " << hex << curr_instr.ip <<"\n";
	}
	else
	{
		tracing_on = true;
		//track_load = 1;
		//printf("Enter MAGIC ROI Begin -----------\n");
	}
}

void EndInstruction()
{
    /*if(instrCount > KnobSkipInstructions.Value())
    {
        tracing_on = true;

        if(instrCount <= (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
        {
            // keep tracing
            fwrite(&curr_instr, sizeof(trace_instr_format_t), 1, out);
	    fwrite(&mod_instr, sizeof(trace_instr_format_t), 1, outMOD);
        }
        else
        {
            tracing_on = false;
		std::cout << "Instr count " << instrCount << std::endl;
            // close down the file, we're done tracing
            if(!output_file_closed)
            {
                fclose(out);
		fclose(outMOD);
                output_file_closed = true;
            }
            exit(0);
        }
    }*/
    if(tracing_on){
	//cout << "curr instr ip : " << hex << curr_instr.ip << "\n";
	//fwrite(&curr_instr, sizeof(trace_instr_format_t), 1, out);
	//fwrite(&mod_instr, sizeof(trace_instr_format_t), 1, outMOD);
    	/*if(instrCount > (KnobTraceInstructions.Value()+KnobSkipInstructions.Value()))
            tracing_on = false;*/
    }
    else if(instrCount >= (KnobTraceInstructions.Value()+KnobSkipInstructions.Value())){
            tracing_on = false;
            //std::cout << dec << "Instr count " << instrCount << std::endl;
        // close down the file, we're done tracing
        if(!output_file_closed)
        {
            fclose(out);
            fclose(outMOD);
            output_file_closed = true;
        }
	exit(0);
    }
}

void BranchOrNot(UINT32 taken)
{
    //printf("[%d] ", taken);

    curr_instr.is_branch = 1;
    mod_instr.is_branch = 1;
    if(taken != 0)
    {
        curr_instr.branch_taken = 1;
        mod_instr.branch_taken = 1;
    }
}

void RegRead(UINT32 i, UINT32 index)
{
    if(!tracing_on) return;

    REG r = (REG)i;

    /*
       if(r == 26)
       {
    // 26 is the IP, which is read and written by branches
    return;
    }
    */

    //cout << r << " " << REG_StringShort((REG)r) << " " ;
    //cout << REG_StringShort((REG)r) << " " ;

    //printf("%d ", (int)r);

    // check to see if this register is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(curr_instr.source_registers[i] == ((unsigned char)r))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(curr_instr.source_registers[i] == 0)
            {
                curr_instr.source_registers[i] = (unsigned char)r;
                mod_instr.source_registers[i] = (unsigned char)r;
		//cout << "register load : " << hex << curr_instr.source_registers[i] <<endl ;
                break;
            }
        }
    }
}

void RegWrite(REG i, UINT32 index)
{
    if(!tracing_on) return;

    REG r = (REG)i;

    /*
       if(r == 26)
       {
    // 26 is the IP, which is read and written by branches
    return;
    }
    */

    //cout << "<" << r << " " << REG_StringShort((REG)r) << "> ";
    //cout << "<" << REG_StringShort((REG)r) << "> ";

    //printf("<%d> ", (int)r);

    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(curr_instr.destination_registers[i] == ((unsigned char)r))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(curr_instr.destination_registers[i] == 0)
            {
                curr_instr.destination_registers[i] = (unsigned char)r;
                mod_instr.destination_registers[i] = (unsigned char)r;
                break;
            }
        }
    }
    /*
       if(index==0)
       {
       curr_instr.destination_register = (unsigned long long int)r;
       }
       */
}

void MemoryRead(VOID* addr, UINT32 index, UINT32 read_size)
{
    /*if(flag_store && !start_set) {
	for(int i=0; i<NUM_INSTR_SOURCES; i++)
	{
	    if(curr_instr.source_memory[i] == 0)
	    {
	            curr_instr.source_memory[i] = (unsigned long long int)addr;
	    	if(track_store == 1) {
	    		start_address = curr_instr.source_memory[i] - 9316;
	    		modifier  = lfsr_arr[position];
			position++;
	    		end_address = start_address + 440000;
	    		mod_instr.source_memory[i] = modifier + 9316;
	    	    	start_set = true;
	    		cout << hex << curr_instr.source_memory[i] << " Start address :  " << start_address << " End Address : " << end_address << " new instr = " << mod_instr.source_memory[i] <<endl;
		    	//cout <<"Load : " << hex << curr_instr.source_memory[i] << " modified : " << mod_instr.source_memory[i] <<endl ;
		    	mod_access++;
			total_access++;
	        	}
	        }
	}
   }*/
    if(!tracing_on) return;

    //printf("0x%llx,%u ", (unsigned long long int)addr, read_size);

    // check to see if this memory read location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_SOURCES; i++)
    {
        if(curr_instr.source_memory[i] == ((unsigned long long int)addr))
        {
            already_found = 1;
            break;
        }
    }
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_SOURCES; i++)
        {
            if(curr_instr.source_memory[i] == 0)
            {
                curr_instr.source_memory[i] = (unsigned long long int)addr;
		total_access++;
		/*if(read_set.find(curr_instr.source_memory[i]) != read_set.end())
		    mod_instr.source_memory[i] = read_set[curr_instr.source_memory[i]];
		else {
		    if(curr_instr.source_memory[i] >= start_address && curr_instr.source_memory[i] <= end_address) {
		        diff = curr_instr.source_memory[i] - start_address;
		        mod_instr.source_memory[i] = modifier + diff;
		    }
		    else {
		        mod_instr.source_memory[i] = lfsr_arr[position] << 6;
			position++;
		    }
		    if(read_set.find(curr_instr.source_memory[i]) == read_set.end())
		    	read_set[curr_instr.source_memory[i]] = mod_instr.source_memory[i];
		}
		if(curr_instr.source_memory[i] != mod_instr.source_memory[i]) {
			mod_access++;
		}*/
		if(flag_store)
			cout <<"Load : " << hex << curr_instr.source_memory[i] << endl ;
		if(flag_array)
			cout <<"Array : " << hex << curr_instr.source_memory[i] << endl ;
                break;
            }
        }
    }
    if(position >= NNodes)
	position=0;
}

void MemoryWrite(VOID* addr, UINT32 index)
{
    /*if(flag_store && !start_set) {
	for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
	{
	    if(curr_instr.destination_memory[i] == 0)
	    {
	            curr_instr.destination_memory[i] = (unsigned long long int)addr;
	    	if(track_store == 1) {
	    		start_address = curr_instr.destination_memory[i] - 48;
	    		modifier  = start_address ^ lfsr_arr[position];
	    		end_address = start_address + 440000;
	    		mod_instr.destination_memory[i] = modifier + 48;
	    	    	start_set = true;
	    		cout << hex << curr_instr.destination_memory[i] << " Start address :  " << start_address << " End Address : " << end_address << " new instr = " << mod_instr.destination_memory[i] <<endl;
	        	}
	        }
	}
   }*/
    if(!tracing_on) return;

    //printf("(0x%llx) ", (unsigned long long int) addr);

    // check to see if this memory write location is already in the list
    int already_found = 0;
    for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
    {
        if(curr_instr.destination_memory[i] == ((unsigned long long int)addr))
        {
            already_found = 1;
            break;
        }
    }
    //cout << "Already found " << already_found << endl;
    if(already_found == 0)
    {
        for(int i=0; i<NUM_INSTR_DESTINATIONS; i++)
        {
            if(curr_instr.destination_memory[i] == 0)
            {
                curr_instr.destination_memory[i] = (unsigned long long int)addr;
		total_access++;
		/*if(read_set.find(curr_instr.destination_memory[i]) != read_set.end())
		    mod_instr.destination_memory[i] = read_set[curr_instr.destination_memory[i]];
		else{
		    if(curr_instr.destination_memory[i] >= start_address && curr_instr.destination_memory[i] <= end_address) {
		        diff = curr_instr.destination_memory[i] - start_address;
		        mod_instr.destination_memory[i] = modifier + diff;
		    }
		    else {
		        mod_instr.destination_memory[i] = lfsr_arr[position] << 6;
			position++;
		    }
		    if(read_set.find(curr_instr.destination_memory[i]) == read_set.end())
		    	read_set[curr_instr.destination_memory[i]] = mod_instr.destination_memory[i];
		}
		if(curr_instr.destination_memory[i] != mod_instr.destination_memory[i]) {
			mod_access++;
		}*/
		if(flag_store)
			cout << "Store : " << hex  << curr_instr.destination_memory[i]  <<endl ;
		if(flag_array)
			cout << "Array : " << hex  << curr_instr.destination_memory[i]  <<endl ;
		break;
            }
        }
    }
    if(position >= NNodes)
	position=0;
    /*
       if(index==0)
       {
       curr_instr.destination_memory = (long long int)addr;
       }
       */
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{

    UINT32 opcode = INS_Opcode(ins);
    //Handle magic instructions
    if (opcode == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_ECX && 
			INS_OperandReg(ins, 1) == REG_ECX) {
       INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Trace_Control, IARG_INST_PTR, IARG_END);
   }
    if (opcode == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EAX && 
			INS_OperandReg(ins, 1) == REG_EAX) {
       INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Camouflage_Access, IARG_INST_PTR, IARG_END);
   }
    if (opcode == XED_ICLASS_XCHG && INS_OperandReg(ins, 0) == REG_EDX && 
			INS_OperandReg(ins, 1) == REG_EDX) {
       INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Camouflage_Array, IARG_INST_PTR, IARG_END);
   }


    // begin each instruction with this function
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BeginInstruction, IARG_INST_PTR, IARG_UINT32, opcode, IARG_END);


    // instrument branch instructions
    if(INS_IsBranch(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)BranchOrNot, IARG_BRANCH_TAKEN, IARG_END);

    // instrument register reads
    UINT32 readRegCount = INS_MaxNumRRegs(ins);
    for(UINT32 i=0; i<readRegCount; i++) 
    {
        UINT32 regNum = INS_RegR(ins, i);

        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegRead,
                IARG_UINT32, regNum, IARG_UINT32, i,
                IARG_END);
    }

    // instrument register writes
    UINT32 writeRegCount = INS_MaxNumWRegs(ins);
    for(UINT32 i=0; i<writeRegCount; i++) 
    {
        UINT32 regNum = INS_RegW(ins, i);

        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RegWrite,
                IARG_UINT32, regNum, IARG_UINT32, i,
                IARG_END);
    }

    // instrument memory reads and writes
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++) 
    {
        if (INS_MemoryOperandIsRead(ins, memOp)) 
        {
            UINT32 read_size = INS_MemoryReadSize(ins);

            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryRead,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp, IARG_UINT32, read_size,
                    IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MemoryWrite,
                    IARG_MEMORYOP_EA, memOp, IARG_UINT32, memOp,
                    IARG_END);
        }
    }

    // finalize each instruction with this function
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)EndInstruction, IARG_END);
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
    cout << dec << "Total Instructions : " << instrCount << endl;
    cout << dec << "Total Access " << total_access << " Modified Access " << mod_access << " Camouflaged Access :" << camouflage_access << endl;
    // close the file if it hasn't already been closed
    if(!output_file_closed) 
    {
        fclose(out);
        fclose(outMOD);
        output_file_closed = true;
    }
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) )
        return Usage();

    const char* fileName = KnobOutputFile.Value().c_str();

    out = fopen(fileName, "w");
    if (!out) 
    {
        cout << "Couldn't open output trace file. Exiting." << endl;
        exit(1);
    }

    const char* fileName_mod = KnobOutputFile2.Value().c_str();

    outMOD = fopen(fileName_mod, "w");
    if (!outMOD) 
    {
        cout << "Couldn't open output trace file. Exiting." << endl;
        exit(1);
    }

    NNodes = KnobSpecifyNodes.Value();
    //cout << "Number of Nodes : " << NNodes << endl;
    //lfsr_arr = (unsigned long long int*) calloc(NNodes, sizeof(unsigned long long int));
    //lfsr_arr = get_modifier(NNodes);

    /*for(unsigned int i = 0; i< NNodes;i++)
        std::cout << std::hex << lfsr_arr[i] << "\n";*/

    // Register function to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    //cerr <<  "===============================================" << endl;
    //cerr <<  "This application is instrumented by the Champsim Trace Generator" << endl;
    //cerr <<  "Trace saved in " << KnobOutputFile.Value() << endl;
    //cerr <<  "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
