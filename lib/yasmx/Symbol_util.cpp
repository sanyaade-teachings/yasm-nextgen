//
// Symbol utility implementation.
//
//  Copyright (C) 2001-2008  Michael Urman, Peter Johnson
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
#include "yasmx/Symbol_util.h"

#include <util.h>

#include "YAML/emitter.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/AssocData.h"
#include "yasmx/Directive.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"


namespace
{

using namespace yasm;

class ObjextNameValues : public AssocData
{
public:
    static const char* key;

    ObjextNameValues(NameValues& nvs) { m_nvs.swap(nvs); }
    ~ObjextNameValues();

    void Write(YAML::Emitter& out) const;

    const NameValues& get() const { return m_nvs; }
    NameValues& get() { return m_nvs; }

private:
    NameValues m_nvs;
};

const char* ObjextNameValues::key = "ObjextNameValues";

ObjextNameValues::~ObjextNameValues()
{
}

void
ObjextNameValues::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "namevalues" << YAML::Value << m_nvs;
    out << YAML::EndMap;
}


class CommonSize : public AssocData
{
public:
    static const char* key;

    CommonSize(const Expr& e) : m_expr(e) {}
    ~CommonSize();

    void Write(YAML::Emitter& out) const;

    const Expr* get() const { return &m_expr; }
    Expr* get() { return &m_expr; }

private:
    Expr m_expr;
};

const char* CommonSize::key = "CommonSize";

CommonSize::~CommonSize()
{
}

void
CommonSize::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "size" << YAML::Value << m_expr;
    out << YAML::EndMap;
}

} // anonymous namespace

namespace yasm
{

void
setObjextNameValues(Symbol& sym, NameValues& objext_namevals)
{
    sym.AddAssocData(std::auto_ptr<ObjextNameValues>
                     (new ObjextNameValues(objext_namevals)));
}

const NameValues*
getObjextNameValues(const Symbol& sym)
{
    const ObjextNameValues* x = sym.getAssocData<ObjextNameValues>();
    if (!x)
        return 0;
    return &x->get();
}

NameValues*
getObjextNameValues(Symbol& sym)
{
    ObjextNameValues* x = sym.getAssocData<ObjextNameValues>();
    if (!x)
        return 0;
    return &x->get();
}

void
setCommonSize(Symbol& sym, const Expr& common_size)
{
    sym.AddAssocData(std::auto_ptr<CommonSize>(new CommonSize(common_size)));
}

const Expr*
getCommonSize(const Symbol& sym)
{
    const CommonSize* x = sym.getAssocData<CommonSize>();
    if (!x)
        return 0;
    return x->get();
}

Expr*
getCommonSize(Symbol& sym)
{
    CommonSize* x = sym.getAssocData<CommonSize>();
    if (!x)
        return 0;
    return x->get();
}

void
DirExtern(DirectiveInfo& info)
{
    Object& object = info.getObject();
    SymbolRef sym = object.getSymbol(info.getNameValues().front().getId());
    sym->Declare(Symbol::EXTERN, info.getLine());

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

void
DirGlobal(DirectiveInfo& info)
{
    Object& object = info.getObject();
    SymbolRef sym = object.getSymbol(info.getNameValues().front().getId());
    sym->Declare(Symbol::GLOBAL, info.getLine());

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

void
DirCommon(DirectiveInfo& info)
{
    NameValues& namevals = info.getNameValues();
    if (namevals.size() < 2)
        throw SyntaxError(N_("no size specified in COMMON declaration"));
    if (!namevals[1].isExpr())
        throw SyntaxError(N_("common size is not an expression"));

    Object& object = info.getObject();
    SymbolRef sym = object.getSymbol(namevals.front().getId());
    sym->Declare(Symbol::COMMON, info.getLine());

    setCommonSize(*sym, namevals[1].getExpr(object, info.getLine()));

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

} // namespace yasm
