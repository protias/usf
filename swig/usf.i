/*
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

%module pyusf

%{
#include "uart/usf_types.h"
#include "uart/usf.h"
#include "uart/usf_events.h"

class Usf {
  public:
    void open(const char *path);
    void create(const char *path, const usf_header_t *header);
    const usf_header_t* get_header();
    void close();
    usf_event_t* read();

  private:
    usf_file_t *file;
};

static usf_error_t error_status = USF_ERROR_OK;

void throw_exception(usf_error_t err) {
    error_status = err;
}

void clear_exception() {
    error_status = USF_ERROR_OK;
}

usf_error_t check_exception() {
    return error_status;
}

void
Usf::open(const char *path)
{
    usf_error_t e = usf_open(&file, path);
    if (e != USF_ERROR_OK) {
        throw_exception(e);
    }
}

void
Usf::close()
{
    usf_close(file);
}

void
Usf::create(const char *path, const usf_header_t *header)
{
    usf_error_t err;

    err = usf_create(&file, path, header);
    if (err != USF_ERROR_OK) {
        throw_exception(err);
    }
}

usf_event_t*
Usf::read()
{
    usf_event_t *e;
    usf_error_t err;

    e = new usf_event_t;

    err = usf_read(file, e);
    if (err != USF_ERROR_OK) {
        throw_exception(err);
    }

    return e;
}

const usf_header_t*
Usf::get_header()
{
    const usf_header_t *h;
    usf_error_t err;

    h = new usf_header_t;
    err = usf_header(&h, file);
    if (err != USF_ERROR_OK) {
        throw_exception(err);
    }

    return h;
}

%}

%include <stdint.i>
//%ignore USF_EVENT_SAMPLE;
//%ignore USF_EVENT_DANGLING;
//%ignore USF_EVENT_BURST;
//%ignore USF_EVENT_TRACE;
%rename(Event) usf_event_t;
%rename(_Event) usf_event_t_u;
%rename(Sample) usf_event_sample_t;
%rename(Burst) usf_event_burst_t;
%rename(Trace) usf_event_trace_t;
%rename(Dangling) usf_event_dangling_t;
%rename(Access) usf_access_t;
%rename(Header) usf_header_t;
%include "uart/usf_events.h"
%include "uart/usf_types.h"

%ignore usf_header;
%ignore usf_append;
%ignore usf_read;
%ignore usf_create;
%ignore usf_close;
%ignore usf_open;
%ignore usf_stratype;
%ignore usf_strcompr;
%ignore usf_strerror;
%ignore USF_ERROR_OK;
%ignore USF_ERROR_PARAM;
%ignore USF_ERROR_SYS;
%ignore USF_ERROR_MEM;
%ignore USF_ERROR_EOF;
%ignore USF_ERROR_FILE;
%ignore USF_ERROR_UNSOPPORTED;
%ignore USF_VERSION_MAJOR;
%ignore USF_VERSION_MINOR;
%include "uart/usf.h"

%extend usf_event_t {
    usf_event_t() {
        usf_event_t *e;
        e = (usf_event_t *) calloc(1, sizeof(usf_event_t));
        return e;
    }
    ~usf_event_t() {
        free(self);
    }

/*
    bool is_trace() {
        return self->type == USF_EVENT_TRACE;
    }
    bool is_sample() {
        return self->type == USF_EVENT_SAMPLE;
    }
    bool is_dangling() {
        return self->type == USF_EVENT_DANGLING;
    }
    bool is_burst() {
        return self->type == USF_EVENT_BURST;
    }
*/

    usf_event_trace_t get_trace() {
        return self->u.trace;
    }
    usf_event_sample_t get_sample() {
        return self->u.sample;
    }
    usf_event_dangling_t get_dangling() {
        return self->u.dangling;
    }
    usf_event_burst_t get_burst() {
        return self->u.burst;
    }
 }

%extend usf_event_trace_t {
    usf_event_trace_t() {
        usf_event_trace_t *t;
        t = (usf_event_trace_t *) calloc(1, sizeof(usf_event_trace_t));
        return t;
    }
    ~usf_event_trace_t() {
        free(self);
    }
 }

%extend usf_event_sample_t {
    usf_event_sample_t() {
        usf_event_sample_t *t;
        t = (usf_event_sample_t *) calloc(1, sizeof(usf_event_sample_t));
        return t;
    }
    ~usf_event_sample_t() {
        free(self);
    }

    usf_access_t get_access_begin() {
        return self->begin;
    }

    usf_access_t get_access_end() {
        return self->end;
    }
 }

%extend usf_event_dangling_t {
    usf_event_dangling_t() {
        usf_event_dangling_t *t;
        t = (usf_event_dangling_t *) calloc(1, sizeof(usf_event_dangling_t));
        return t;
    }
    ~usf_event_dangling_t() {
        free(self);
    }

    usf_access_t get_access() {
        return self->begin;
    }
 }

%extend usf_access_t {
    usf_access_t() {
        usf_access_t *a;
        a = (usf_access_t *) calloc(1, sizeof(usf_access_t));
        return a;
    }
    ~usf_access_t() {
        free(self);
    }
}

%exception {
    const char *errormsg;
    usf_error_t error;

    clear_exception();
    $action

    error = check_exception();
    if (error != USF_ERROR_OK) {
        errormsg = usf_strerror(error);
        switch (error) {
        case USF_ERROR_PARAM:
            PyErr_SetString(PyExc_TypeError, errormsg);
            break;
        case USF_ERROR_SYS:
            PyErr_SetString(PyExc_OSError, errormsg);
            break;
        case USF_ERROR_MEM:
            PyErr_SetString(PyExc_MemoryError, errormsg);
            break;
        case USF_ERROR_EOF:
            PyErr_SetString(PyExc_EOFError, errormsg);
            break;
        case USF_ERROR_FILE:
            PyErr_SetString(PyExc_StandardError, errormsg);
            break;
        case USF_ERROR_UNSUPPORTED:
            PyErr_SetString(PyExc_NotImplementedError, errormsg);
            break;
        default:
            PyErr_SetString(PyExc_NotImplementedError, "Check python bindings");
            break;
        }

        return NULL;
    }
}

%newobject Usf::read;
%newobject Usf::get_header;
class Usf {
  public:
    void open(const char *path);
    void close();
    const usf_header_t* get_header();
    usf_event_t* read();
};

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "k&r"
 * End:
 */
