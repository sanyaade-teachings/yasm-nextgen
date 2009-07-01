//
// NASM-compatible parser
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "NasmParser.h"

#include <util.h>

#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/Arch.h>
#include <yasmx/Directive.h>
#include <yasmx/Expr.h>
#include <yasmx/Object.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol_util.h>


namespace yasm
{
namespace parser
{
namespace nasm
{

NasmParser::NasmParser(const ParserModule& module, Errwarns& errwarns)
    : Parser(module, errwarns)
{
}

NasmParser::~NasmParser()
{
}

void
NasmParser::parse(Object& object,
                  Preprocessor& preproc,
                  bool save_input,
                  Directives& dirs,
                  Linemap& linemap)
{
    init_mixin(object, preproc, save_input, dirs, linemap);

    m_locallabel_base = "";

    m_bc = 0;

    m_absstart.clear();
    m_abspos.clear();

    m_state = INITIAL;

    do_parse();

    // Check for undefined symbols
    object.symbols_finalize(m_errwarns, false);
}

std::vector<const char*>
NasmParser::get_preproc_keywords()
{
    // valid preprocessors to use with this parser
    static const char* keywords[] = {"raw", "nasm"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

void
NasmParser::add_directives(Directives& dirs, const char* parser)
{
    static const Directives::Init<NasmParser> nasm_dirs[] =
    {
        {"absolute", &NasmParser::dir_absolute, Directives::ARG_REQUIRED},
        {"align", &NasmParser::dir_align, Directives::ARG_REQUIRED},
        {"default", &NasmParser::dir_default, Directives::ANY},
    };

    if (String::nocase_equal(parser, "nasm"))
    {
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
        dirs.add("extern", &dir_extern, Directives::ID_REQUIRED);
        dirs.add("global", &dir_global, Directives::ID_REQUIRED);
        dirs.add("common", &dir_common, Directives::ID_REQUIRED);
    }
}

void
do_register()
{
    register_module<ParserModule, ParserModuleImpl<NasmParser> >("nasm");
}

}}} // namespace yasm::parser::nasm
