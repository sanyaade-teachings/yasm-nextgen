//
//  Copyright (C) 2007  Peter Johnson
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
#include <cxxtest/TestSuite.h>

#include "Compose.h"

#include <iomanip>

class ComposeTestSuite : public CxxTest::TestSuite
{
public:
    void testByClass()
    {
        std::string out;

        // demonstrate basic usage
        out = String::Compose("There are %1 cows in them %2.") % 15 % "fields";
        TS_ASSERT_EQUALS(out, "There are 15 cows in them fields.");

        using String::Compose;

        // demonstrate argument repetition
        out = Compose("To %1, or not to %1... is actually not a question.") % "be";
        TS_ASSERT_EQUALS(out, "To be, or not to be... is actually not a question.");

        // demonstrate leaving out arguments
        out = Compose("Primetime: %2  %3  %5  %7") % 1 % 2 % 3 % 4 % 5 % 6 % 7;
        TS_ASSERT_EQUALS(out, "Primetime: 2  3  5  7");

        // demonstrate % escaping
        out = Compose("Using % before a %%1 causes the %1 to be escaped") % "%1";
        TS_ASSERT_EQUALS(out, "Using % before a %1 causes the %1 to be escaped");

        out = Compose("Four percent in a row: %%%%%%%%");
        TS_ASSERT_EQUALS(out, "Four percent in a row: %%%%");

        // demonstrate use of manipulators
        // parenthesis around String::format are required
        out = Compose("With lots of precision, %2 equals %1%!") %
            (String::format(std::setprecision(15), (1.0 / 3 * 100))) % "one third";
        TS_ASSERT_EQUALS(out, "With lots of precision, one third equals 33.3333333333333%!");

        // test % escaping at the string ends
        out = Compose("%% This is like a LaTeX comment %%");
        TS_ASSERT_EQUALS(out, "% This is like a LaTeX comment %");

        // test % specs at the string ends
        out = Compose("%1 %2") % "Hello" % "World!";
        TS_ASSERT_EQUALS(out, "Hello World!");

        // test a bunch of arguments
        out = Compose("%1 %2 %3 %4 %5 %6, %7 %8!") % "May" % "the" % "Force" %
            "be" % "with" % "you" % "Woung" % "Skytalker";
        TS_ASSERT_EQUALS(out, "May the Force be with you, Woung Skytalker!");
    }

    void testByFunction()
    {
        std::string out;

        // demonstrate basic usage
        out = String::compose("There are %1 cows in them %2.", 15, "fields");
        TS_ASSERT_EQUALS(out, "There are 15 cows in them fields.");

        using String::compose;

        // demonstrate argument repetition
        out = compose("To %1, or not to %1... is actually not a question.", "be");
        TS_ASSERT_EQUALS(out, "To be, or not to be... is actually not a question.");

        // demonstrate leaving out arguments
        out = compose("Primetime: %2  %3  %5  %7", 1, 2, 3, 4, 5, 6, 7);
        TS_ASSERT_EQUALS(out, "Primetime: 2  3  5  7");

        // demonstrate % escaping
        out = compose("Using % before a %%1 causes the %1 to be escaped", "%1");
        TS_ASSERT_EQUALS(out, "Using % before a %1 causes the %1 to be escaped");

        out = compose("Four percent in a row: %%%%%%%%");
        TS_ASSERT_EQUALS(out, "Four percent in a row: %%%%");

        // demonstrate use of manipulators
        out = compose("With lots of precision, %2 equals %1%!",
            String::format(std::setprecision(15), (1.0 / 3 * 100)), "one third");
        TS_ASSERT_EQUALS(out, "With lots of precision, one third equals 33.3333333333333%!");

        // test % escaping at the string ends
        out = compose("%% This is like a LaTeX comment %%");
        TS_ASSERT_EQUALS(out, "% This is like a LaTeX comment %");

        // test % specs at the string ends
        out = compose("%1 %2", "Hello", "World!");
        TS_ASSERT_EQUALS(out, "Hello World!");

        // test a bunch of arguments
        out = compose("%1 %2 %3 %4 %5 %6, %7 %8!", "May", "the", "Force",
                      "be", "with", "you", "Woung", "Skytalker");
        TS_ASSERT_EQUALS(out, "May the Force be with you, Woung Skytalker!");
    }

    void testEmpty()
    {
        std::string out;

        out = String::Compose("1: %1 2: %2 3: %3") % "" % "b" % "c";
        TS_ASSERT_EQUALS(out, "1:  2: b 3: c");

        out = String::Compose("1: %1 2: %2 3: %3") % "a" % "" % "c";
        TS_ASSERT_EQUALS(out, "1: a 2:  3: c");

        out = String::Compose("1: %1 2: %2 3: %3") % "a" % "b" % "";
        TS_ASSERT_EQUALS(out, "1: a 2: b 3: ");

        out = String::Compose("1: %1 2: %2 3: %3 ") % "a" % "b" % "";
        TS_ASSERT_EQUALS(out, "1: a 2: b 3:  ");
    }

    std::string
    func(const std::string& arg)
    {
        return arg;
    }

    void testFuncOut()
    {
        using String::Compose;
        TS_ASSERT_EQUALS(func(Compose("composing in a %1 is fun!") % "function"),
                          "composing in a function is fun!");
    }

    void testStreamOut()
    {
        std::ostringstream os;
        os << "Here's some " << String::Compose("formatted %1 %2") % "text" % "for"
            << " you!";
        TS_ASSERT_EQUALS(os.str(), "Here's some formatted text for you!");
    }

    void testCopy()
    {
        using String::Compose;

        Compose x("%1 %2");
        x.auto_arg("foo");

        Compose a = x, b = x;
        a.auto_arg("bar");
        b.auto_arg("baz");

        TS_ASSERT_EQUALS(a.str(), "foo bar");
        TS_ASSERT_EQUALS(b.str(), "foo baz");
    }

    void testPartial()
    {
        TS_ASSERT_EQUALS(String::compose("%1 %2 %3", "foo", "bar"), "foo bar %3");
    }
};
