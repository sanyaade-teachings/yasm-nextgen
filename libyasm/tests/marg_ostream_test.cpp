//
//  Copyright (C) 2008  Peter Johnson
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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE marg_ostream_test
#include <boost/test/unit_test.hpp>

#include "marg_ostream.h"

#include <sstream>
#include <string>

using namespace yasm;


static void
t2(yasm::marg_ostream& os)
{
    ++os;
    os << "function indented\n";
    --os;
}

static void
t1(yasm::marg_ostream& os)
{
    os << "not indented\n";
    ++os;
    os << "indented\n";
    --os;
    os << "unindented\n";
    t2(os);
}

BOOST_AUTO_TEST_CASE(TestCase1)
{
    std::ostringstream oss;
    std::string golden;

    yasm::marg_ostream os(oss.rdbuf());
    os << "begin\n";
    ++os;
    t1(os);
    --os;
    os << "end\n";

    golden = 
        "begin\n"
        "  not indented\n"
        "    indented\n"
        "  unindented\n"
        "    function indented\n"
        "end\n";
    BOOST_CHECK_EQUAL(oss.str(), golden);
}