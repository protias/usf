/*
 * Copyright (C) 2010-2011, David Eklov
 * Copyright (C) 2011, Andreas Sandberg
 * Copyright (C) 2011, Nikos Nikoleris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uart/usf.h>
#include <sys/time.h>

#include "pin.H"
#include "instlib.H"

KNOB<string> knob_filename(KNOB_MODE_WRITEONCE, "pintool", "o",
			   "foo.usf", "Output filename");
KNOB<BOOL> knob_early_out(KNOB_MODE_WRITEONCE, "pintool", "d", "0",
			  "Stop pin at stop address");
KNOB<BOOL> knob_bzip2(KNOB_MODE_WRITEONCE,
                      "pintool", "c", "0", "Enable BZip2 compression");
KNOB<BOOL> knob_inst_time(KNOB_MODE_WRITEONCE,
                          "pintool", "i", "0", "Use instruction count as time base");

using namespace INSTLIB;

LOCALVAR INT32 enabled = 0;

static usf_file_t *usf_file;
static usf_atime_t usf_time;

static VOID fini(INT32 code, VOID *v);

INSTLIB::ICOUNT icount;
CONTROL control(false);

LOCALFUN VOID
Handler(CONTROL_EVENT ev, VOID *, CONTEXT * ctxt, VOID *, THREADID)
{
    switch(ev) {
    case CONTROL_START:
	cerr << "Tracing started: " << icount.Count() << " instr" << endl;
        PIN_RemoveInstrumentation();
        enabled = 1;
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        // So that the rest of the current trace is re-instrumented.
        if (ctxt) PIN_ExecuteAt (ctxt);
#endif
        break;
    case CONTROL_STOP:
	cerr << "Tracing finished: " << icount.Count() << " instr" << endl;
        PIN_RemoveInstrumentation();
	if (knob_early_out) {
	    cerr << "Exiting due to -early_out" << endl;
	    fini(0, NULL);
	    exit(0);
	}
	PIN_Detach();
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        // So that the rest of the current trace is re-instrumented.
        if (ctxt) PIN_ExecuteAt (ctxt);
#endif
        break;
    default:
        ASSERTX(false);
    }
}

static VOID
log_access(VOID *pc, VOID *addr, ADDRINT size, THREADID tid, UINT32 type)
{
    usf_event_t event;
    usf_access_t *access;
    
    event.type = USF_EVENT_TRACE;
    access = &event.u.trace.access;
    
    access->pc   = (usf_addr_t)pc;
    access->addr = (usf_addr_t)addr;
    access->time = usf_time;
    access->tid  = (usf_tid_t)tid;
    access->len  = (usf_alen_t)size;
    access->type = (usf_atype_t)type;

    if (usf_append(usf_file, &event) != USF_ERROR_OK) {
        cerr << "USF: Failed to append event."  << endl;
        abort();
    }
}

static VOID PIN_FAST_ANALYSIS_CALL
inc_time()
{
    usf_time++;
}

static VOID
instruction(INS ins, VOID *not_used)
{
    if (!enabled)
        return;

    UINT32 no_ops = INS_MemoryOperandCount(ins);

    for (UINT32 op = 0; op < no_ops; op++) {
        const UINT32 size = INS_MemoryOperandSize(ins, op);
	const bool is_rd = INS_MemoryOperandIsRead(ins, op);
	const bool is_wr = INS_MemoryOperandIsWritten(ins, op);
	const UINT32 atype =
	    is_rd && is_wr ? USF_ATYPE_RW :
	    (is_wr ? USF_ATYPE_WR : USF_ATYPE_RD);

	INS_InsertCall(ins, IPOINT_BEFORE,
		       (AFUNPTR)log_access,
		       IARG_INST_PTR,
		       IARG_MEMORYOP_EA, op,
		       IARG_UINT32, size,
		       IARG_THREAD_ID,
		       IARG_UINT32, atype,
		       IARG_END); 
	
        if (!knob_inst_time) {
            /* Increase the time counter if the time base is memory accesses */
            INS_InsertCall(ins, IPOINT_BEFORE,
			   AFUNPTR(inc_time),
			   IARG_FAST_ANALYSIS_CALL,
			   IARG_END);
        }
    }

    if (knob_inst_time) {
        /* Increase the time counter if the time base is instructions.
         * NOTE: This means that multiple memory acceses may
         * happen at the same time */
        INS_InsertCall(ins, IPOINT_BEFORE,
		       AFUNPTR(inc_time),
		       IARG_FAST_ANALYSIS_CALL,
		       IARG_END);
    }
}

static int
init(int argc, char *argv[])
{
    const char *filename = knob_filename.Value().c_str();
    usf_header_t header;
    struct timeval tv;
    int target_argc = argc;
    char **target_argv = argv;

    memset(&header, 0, sizeof(header));
    
    header.version = USF_VERSION_CURRENT;
    header.compression = knob_bzip2.Value() ? USF_COMPRESSION_BZIP2 : 
	USF_COMPRESSION_NONE;
    header.flags = USF_FLAG_NATIVE_ENDIAN | USF_FLAG_TRACE | USF_FLAG_DELTA |
        (knob_inst_time ? USF_FLAG_TIME_INSTRUCTIONS : USF_FLAG_TIME_ACCESSES);

    if (gettimeofday(&tv, NULL) == 0)
	header.time_begin = tv.tv_sec;
    else
	cerr << "Warning: Failed to get time of day, "
	     << "information not included in trace file." << endl;

    for (int i = 0; i < argc; i++) {
	if (!strcmp("--", argv[i])) {
	    target_argc = argc - i - 1;
	    target_argv = argv + i + 1;
	}
    }
    header.argc = target_argc;
    header.argv = target_argv;

    return usf_create(&usf_file, filename, &header) == USF_ERROR_OK ? 0 : -1;
}

static VOID
fini(INT32 code, VOID *v)
{
    usf_close(usf_file);
}

static int
usage()
{
    cerr <<
        "This tool is a PIN tool to generate USF trace files.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv))
        return usage();

   if (init(argc, argv))
        return -1;

    control.CheckKnobs(Handler, 0);
    icount.Activate();
    INS_AddInstrumentFunction(instruction, 0);
    PIN_AddFiniFunction(fini, 0);

    PIN_StartProgram();
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
