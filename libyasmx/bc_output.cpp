//
// YASM bytecode output implementation
//
//  Copyright (C) 2001-2008  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "bc_output.h"

#include "errwarn.h"
#include "util.h"


namespace yasm
{

BytecodeOutput::BytecodeOutput()
{
}

BytecodeOutput::~BytecodeOutput()
{
}

void
BytecodeOutput::output(SymbolRef sym,
                       Bytes& bytes,
                       Bytecode& bc,
                       unsigned int valsize,
                       int warn)
{
    output(bytes);
}

BytecodeNoOutput::~BytecodeNoOutput()
{
}

void
BytecodeNoOutput::output(Value& value, Bytes& bytes, Location loc, int warn)
{
    // unnecessary; we don't actually output it anyway
}

void
BytecodeNoOutput::output_gap(unsigned int size)
{
    // expected
}

void
BytecodeNoOutput::output(const Bytes& bytes)
{
    warn_set(WARN_GENERAL,
        N_("initialized space declared in nobits section: ignoring"));
}

} // namespace yasm