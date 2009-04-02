#ifndef YASM_ELFSYMBOL_H
#define YASM_ELFSYMBOL_H
//
// ELF object format symbol
//
//  Copyright (C) 2003-2007  Michael Urman
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

#include <iosfwd>
#include <vector>

#include <yasmx/Support/marg_ostream_fwd.h>
#include <yasmx/Support/scoped_ptr.h>
#include <yasmx/AssocData.h>
#include <yasmx/Bytes.h>
#include <yasmx/IntNum.h>
#include <yasmx/Reloc.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>
#include <yasmx/SymbolRef.h>

#include "ElfTypes.h"


namespace yasm
{

class Errwarns;
class Expr;
class Object;
class Section;
class StringTable;

namespace objfmt
{
namespace elf
{

class ElfSymbol : public AssocData
{
public:
    static const char* key;

    // Constructor that reads from bytes (e.g. from file)
    ElfSymbol(const ElfConfig&  config,
              Bytes&            bytes,
              ElfSymbolIndex    index,
              Section*          sections[]);

    ElfSymbol();
    ~ElfSymbol();

    SymbolRef create_symbol(Object& object, const StringTable& strtab) const;

    void put(marg_ostream& os) const;

    void finalize(Symbol& sym, Errwarns& errwarns);
    void write(Bytes& bytes, const ElfConfig& config);

    void set_section(Section* sect) { m_sect = sect; }
    void set_name(ElfStringIndex index) { m_name_index = index; }
    void set_index(ElfSectionIndex index) { m_index = index; }
    void set_visibility(ElfSymbolVis vis) { m_vis = ELF_ST_VISIBILITY(vis); }
    void set_binding(ElfSymbolBinding bind) { m_bind = bind; }
    void set_type(ElfSymbolType type) { m_type = type; }
    void set_size(std::auto_ptr<Expr> size, unsigned long line);
    void set_value(ElfAddress value) { m_value = value; }
    void set_symindex(ElfSymbolIndex symindex) { m_symindex = symindex; }
    ElfSymbolIndex get_symindex() const { return m_symindex; }

    bool is_local() const { return m_bind == STB_LOCAL; }

private:
    Section*            m_sect;
    ElfStringIndex      m_name_index;
    IntNum              m_value;
    util::scoped_ptr<Expr> m_xsize;
    unsigned long       m_size_line;
    IntNum              m_size;
    ElfSectionIndex     m_index;
    ElfSymbolBinding    m_bind;
    ElfSymbolType       m_type;
    ElfSymbolVis        m_vis;
    ElfSymbolIndex      m_symindex;
};

inline ElfSymbol*
get_elf(Symbol& sym)
{
    return static_cast<ElfSymbol*>(sym.get_assoc_data(ElfSymbol::key));
}

void insert_local_sym(Object& object,
                      std::auto_ptr<Symbol> sym,
                      std::auto_ptr<ElfSymbol> entry);
ElfSymbolIndex assign_sym_indices(Object& object);

}}} // namespace yasm::objfmt::elf

#endif