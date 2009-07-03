//
// Optimizer implementation.
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
#define DEBUG_TYPE "Optimize"

#include "yasmx/Object.h"

#include "util.h"

#include <algorithm>
#include <list>
#include <memory>
#include <queue>
#include <vector>

#include "llvm/ADT/Statistic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/IntervalTree.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytecode_util.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Section.h"
#include "yasmx/Value.h"


STATISTIC(num_span_terms, "Number of span terms created");
STATISTIC(num_spans, "Number of spans created");
STATISTIC(num_step1d, "Number of spans after step 1b");
STATISTIC(num_itree, "Number of span terms added to interval tree");
STATISTIC(num_offset_setters, "Number of offset setters");
STATISTIC(num_expansions, "Number of expansions performed");
STATISTIC(num_initial_qb, "Number of spans on initial QB");

//
// Robertson (1977) optimizer
// Based (somewhat loosely) on the algorithm given in:
//   MRC Technical Summary Report # 1779
//   CODE GENERATION FOR SHORT/LONG ADDRESS MACHINES
//   Edward L. Robertson
//   Mathematics Research Center
//   University of Wisconsin-Madison
//   610 Walnut Street
//   Madison, Wisconsin 53706
//   August 1977
//
// Key components of algorithm:
//  - start assuming all short forms
//  - build spans for short->long transition dependencies
//  - if a long form is needed, walk the dependencies and update
// Major differences from Robertson's algorithm:
//  - detection of cycles
//  - any difference of two locations is allowed
//  - handling of alignment/org gaps (offset setting)
//  - handling of multiples
//
// Data structures:
//  - Interval tree to store spans and associated data
//  - Queues QA and QB
//
// Each span keeps track of:
//  - Associated bytecode (bytecode that depends on the span length)
//  - Active/inactive state (starts out active)
//  - Sign (negative/positive; negative being "backwards" in address)
//  - Current length in bytes
//  - New length in bytes
//  - Negative/Positive thresholds
//  - Span ID (unique within each bytecode)
//
// How org and align and any other offset-based bytecodes are handled:
//
// Some portions are critical values that must not depend on any bytecode
// offset (either relative or absolute).
//
// All offset-setters (ORG and ALIGN) are put into a linked list in section
// order (e.g. increasing offset order).  Each span keeps track of the next
// offset-setter following the span's associated bytecode.
//
// When a bytecode is expanded, the next offset-setter is examined.  The
// offset-setter may be able to absorb the expansion (e.g. any offset
// following it would not change), or it may have to move forward (in the
// case of align) or error (in the case of org).  If it has to move forward,
// following offset-setters must also be examined for absorption or moving
// forward.  In either case, the ongoing offset is updated as well as the
// lengths of any spans dependent on the offset-setter.
//
// Alignment/ORG value is critical value.
// Cannot be combined with TIMES.
//
// How times is handled:
//
// TIMES: Handled separately from bytecode "raw" size.  If not span-dependent,
//      trivial (just multiplied in at any bytecode size increase).  Span
//      dependent times update on any change (span ID 0).  If the resultant
//      next bytecode offset would be less than the old next bytecode offset,
//      error.  Otherwise increase offset and update dependent spans.
//
// To reduce interval tree size, a first expansion pass is performed
// before the spans are added to the tree.
//
// Basic algorithm outline:
//
// 1. Initialization:
//  a. Number bytecodes sequentially (via bc_index) and calculate offsets
//     of all bytecodes assuming minimum length, building a list of all
//     dependent spans as we go.
//     "minimum" here means absolute minimum:
//      - align/org (offset-based) bumps offset as normal
//      - times values (with span-dependent values) assumed to be 0
//  b. Iterate over spans.  Set span length based on bytecode offsets
//     determined in 1a.  If span is "certainly" long because the span
//     is an absolute reference to another section (or external) or the
//     distance calculated based on the minimum length is greater than the
//     span's threshold, expand the span's bytecode, and if no further
//     expansion can result, mark span as inactive.
//  c. Iterate over bytecodes to update all bytecode offsets based on new
//     (expanded) lengths calculated in 1b.
//  d. Iterate over active spans.  Add span to interval tree.  Update span's
//     length based on new bytecode offsets determined in 1c.  If span's
//     length exceeds long threshold, add that span to Q.
// 2. Main loop:
//   While Q not empty:
//     Expand BC dependent on span at head of Q (and remove span from Q).
//     Update span:
//       If BC no longer dependent on span, mark span as inactive.
//       If BC has new thresholds for span, update span.
//     If BC increased in size, for each active span that contains BC:
//       Increase span length by difference between short and long BC length.
//       If span exceeds long threshold (or is flagged to recalculate on any
//       change), add it to tail of Q.
// 3. Final pass over bytecodes to generate final offsets.
//
namespace
{

using namespace yasm;

class OffsetSetter
{
public:
    OffsetSetter();
    ~OffsetSetter() {}

    Bytecode* m_bc;
    unsigned long m_cur_val;
    unsigned long m_new_val;
    unsigned long m_thres;
};

OffsetSetter::OffsetSetter()
    : m_bc(0),
      m_cur_val(0),
      m_new_val(0),
      m_thres(0)
{
}

class Optimizer;

class Span
{
    friend class Optimizer;
public:
    class Term
    {
    public:
        Term();
        Term(unsigned int subst,
             Location loc,
             Location loc2,
             Span* span,
             long new_val);
        ~Term() {}

        Location m_loc;
        Location m_loc2;
        Span* m_span;       // span this term is a member of
        long m_cur_val;
        long m_new_val;
        unsigned int m_subst;
    };

    Span(Bytecode& bc,
         int id,
         const Value& value,
         long neg_thres,
         long pos_thres,
         size_t os_index);
    ~Span();

    void CreateTerms(Optimizer* optimize);
    bool RecalcNormal();

private:
    Span(const Span&);                  // not implemented
    const Span& operator=(const Span&); // not implemented

    void AddTerm(unsigned int subst, Location loc, Location loc2);

    Bytecode& m_bc;

    Value m_depval;

    // span terms in absolute portion of value
    typedef std::vector<Term> Terms;
    Terms m_span_terms;
    ExprTerms m_expr_terms;

    long m_cur_val;
    long m_new_val;

    long m_neg_thres;
    long m_pos_thres;

    int m_id;

    enum { INACTIVE = 0, ACTIVE, ON_Q } m_active;

    // Spans that led to this span.  Used only for
    // checking for circular references (cycles) with id=0 spans.
    std::vector<Span*> m_backtrace;

    // Index of first offset setter following this span's bytecode
    size_t m_os_index;
};

class Optimizer
{
public:
    Optimizer();
    ~Optimizer();
    void AddSpan(Bytecode& bc,
                 int id,
                 const Value& value,
                 long neg_thres,
                 long pos_thres);
    void AddOffsetSetter(Bytecode& bc);

    bool Step1b(Errwarns& errwarns);
    bool Step1d();
    bool Step1e(Errwarns& errwarns);
    bool Step2(Errwarns& errwarns);

private:
    void ITreeAdd(Span& span, Span::Term& term);
    void CheckCycle(IntervalTreeNode<Span::Term*> * node, Span& span);
    void ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff);

    std::list<Span*> m_spans;   // ownership list
    std::queue<Span*> m_QA, m_QB;
    IntervalTree<Span::Term*> m_itree;
    std::vector<OffsetSetter> m_offset_setters;
};

Span::Term::Term()
    : m_span(0),
      m_cur_val(0),
      m_new_val(0),
      m_subst(0)
{
}

Span::Term::Term(unsigned int subst,
                 Location loc,
                 Location loc2,
                 Span* span,
                 long new_val)
    : m_loc(loc),
      m_loc2(loc2),
      m_span(span),
      m_cur_val(0),
      m_new_val(new_val),
      m_subst(subst)
{
    ++num_span_terms;
}

Span::Span(Bytecode& bc,
           int id,
           /*@null@*/ const Value& value,
           long neg_thres,
           long pos_thres,
           size_t os_index)
    : m_bc(bc),
      m_depval(value),
      m_cur_val(0),
      m_new_val(0),
      m_neg_thres(neg_thres),
      m_pos_thres(pos_thres),
      m_id(id),
      m_active(ACTIVE),
      m_os_index(os_index)
{
    ++num_spans;
}

void
Optimizer::AddSpan(Bytecode& bc,
                   int id,
                   const Value& value,
                   long neg_thres,
                   long pos_thres)
{
    m_spans.push_back(new Span(bc, id, value, neg_thres, pos_thres,
                               m_offset_setters.size()-1));
}

void
Span::AddTerm(unsigned int subst, Location loc, Location loc2)
{
    IntNum intn;
    bool ok = CalcDist(loc, loc2, &intn);
    ok = ok;    // avoid warning due to assert usage
    assert(ok && "could not calculate bc distance");

    if (subst >= m_span_terms.size())
        m_span_terms.resize(subst+1);
    m_span_terms[subst] = Term(subst, loc, loc2, this, intn.getInt());
}

void
Span::CreateTerms(Optimizer* optimize)
{
    // Split out sym-sym terms in absolute portion of dependent value
    if (m_depval.hasAbs())
    {
        SubstDist(*m_depval.getAbs(),
                  BIND::bind(&Span::AddTerm, this, _1, _2, _3));
        if (m_span_terms.size() > 0)
        {
            for (Terms::iterator i=m_span_terms.begin(),
                 end=m_span_terms.end(); i != end; ++i)
            {
                // Create expression terms with dummy value
                m_expr_terms.push_back(ExprTerm(0));

                // Check for circular references
                if (m_id <= 0 &&
                    ((m_bc.getIndex() > i->m_loc.bc->getIndex()-1 &&
                      m_bc.getIndex() <= i->m_loc2.bc->getIndex()-1) ||
                     (m_bc.getIndex() > i->m_loc2.bc->getIndex()-1 &&
                      m_bc.getIndex() <= i->m_loc.bc->getIndex()-1)))
                    throw ValueError(N_("circular reference detected"));
            }
        }
    }
}

// Recalculate span value based on current span replacement values.
// Returns True if span needs expansion (e.g. exceeded thresholds).
bool
Span::RecalcNormal()
{
    m_new_val = 0;

    if (m_depval.hasAbs())
    {
        std::auto_ptr<Expr> abs_copy(m_depval.getAbs()->clone());

        // Update sym-sym terms and substitute back into expr
        for (Terms::iterator i=m_span_terms.begin(), end=m_span_terms.end();
             i != end; ++i)
            m_expr_terms[i->m_subst].getIntNum()->set(i->m_new_val);
        abs_copy->Substitute(m_expr_terms);
        abs_copy->Simplify();
        if (abs_copy->isIntNum())
            m_new_val = abs_copy->getIntNum().getInt();
        else
            m_new_val = LONG_MAX;   // too complex; force to longest form
    }

    if (m_depval.isRelative())
        m_new_val = LONG_MAX;       // too complex; force to longest form

    if (m_new_val == LONG_MAX)
        m_active = INACTIVE;

    // If id<=0, flag update on any change
    if (m_id <= 0)
        return (m_new_val != m_cur_val);

    return (m_new_val < m_neg_thres || m_new_val > m_pos_thres);
}

Span::~Span()
{
}

Optimizer::Optimizer()
{
    // Create an placeholder offset setter for spans to point to; this will
    // get updated if/when we actually run into one.
    m_offset_setters.push_back(OffsetSetter());
}

Optimizer::~Optimizer()
{
    while (!m_spans.empty())
    {
        delete m_spans.back();
        m_spans.pop_back();
    }
}

void
Optimizer::AddOffsetSetter(Bytecode& bc)
{
    // Remember it
    OffsetSetter& os = m_offset_setters.back();
    os.m_bc = &bc;
    os.m_thres = bc.getNextOffset();

    // Create new placeholder
    m_offset_setters.push_back(OffsetSetter());
}

void
Optimizer::ITreeAdd(Span& span, Span::Term& term)
{
    long precbc_index, precbc2_index;
    unsigned long low, high;

    // Update term length
    if (term.m_loc.bc)
        precbc_index = term.m_loc.bc->getIndex();
    else
        precbc_index = span.m_bc.getIndex()-1;

    if (term.m_loc2.bc)
        precbc2_index = term.m_loc2.bc->getIndex();
    else
        precbc2_index = span.m_bc.getIndex()-1;

    if (precbc_index < precbc2_index)
    {
        low = precbc_index+1;
        high = precbc2_index;
    }
    else if (precbc_index > precbc2_index)
    {
        low = precbc2_index+1;
        high = precbc_index;
    }
    else
        return;     // difference is same bc - always 0!

    m_itree.Insert(static_cast<long>(low), static_cast<long>(high), &term);
    ++num_itree;
}

void
Optimizer::CheckCycle(IntervalTreeNode<Span::Term*> * node, Span& span)
{
    Span::Term* term = node->getData();
    Span* depspan = term->m_span;

    // Only check for cycles in id=0 spans
    if (depspan->m_id > 0)
        return;

    // Check for a circular reference by looking to see if this dependent
    // span is in our backtrace.
    std::vector<Span*>::iterator iter;
    iter = std::find(span.m_backtrace.begin(), span.m_backtrace.end(),
                     depspan);
    if (iter != span.m_backtrace.end())
        throw ValueError(N_("circular reference detected"));

    // Add our complete backtrace and ourselves to backtrace of dependent
    // span.
    std::copy(span.m_backtrace.begin(), span.m_backtrace.end(),
              depspan->m_backtrace.end());
    depspan->m_backtrace.push_back(&span);
}

void
Optimizer::ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff)
{
    Span::Term* term = node->getData();
    Span* span = term->m_span;
    long precbc_index, precbc2_index;

    // Don't expand inactive spans
    if (span->m_active == Span::INACTIVE)
        return;

    // Update term length
    if (term->m_loc.bc)
        precbc_index = term->m_loc.bc->getIndex();
    else
        precbc_index = span->m_bc.getIndex()-1;

    if (term->m_loc2.bc)
        precbc2_index = term->m_loc2.bc->getIndex();
    else
        precbc2_index = span->m_bc.getIndex()-1;

    if (precbc_index < precbc2_index)
        term->m_new_val += len_diff;
    else
        term->m_new_val -= len_diff;

    // If already on Q, don't re-add
    if (span->m_active == Span::ON_Q)
        return;

    // Update term and check against thresholds
    if (!span->RecalcNormal())
        return; // didn't exceed thresholds, we're done

    // Exceeded thresholds, need to add to Q for expansion
    if (span->m_id <= 0)
        m_QA.push(span);
    else
        m_QB.push(span);
    span->m_active = Span::ON_Q;    // Mark as being in Q
}

bool
Optimizer::Step1b(Errwarns& errwarns)
{
    bool saw_error = false;

    std::list<Span*>::iterator spani = m_spans.begin();
    while (spani != m_spans.end())
    {
        Span* span = *spani;
        bool terms_okay = true;

        try
        {
            span->CreateTerms(this);
        }
        catch (Error& err)
        {
            errwarns.Propagate(span->m_bc.getLine(), err);
            saw_error = true;
            terms_okay = false;
        }

        if (terms_okay && span->RecalcNormal())
        {
            bool still_depend =
                Expand(span->m_bc, span->m_id, span->m_cur_val,
                       span->m_new_val, &span->m_neg_thres,
                       &span->m_pos_thres, errwarns);
            if (errwarns.getNumErrors() > 0)
                saw_error = true;
            else if (still_depend)
            {
                if (span->m_active == Span::INACTIVE)
                {
                    errwarns.Propagate(span->m_bc.getLine(),
                        ValueError(N_("secondary expansion of an external/complex value")));
                    saw_error = true;
                }
            }
            else
            {
                delete *spani;
                spani = m_spans.erase(spani);
                continue;
            }
        }
        span->m_cur_val = span->m_new_val;
        ++spani;
    }

    return saw_error;
}

bool
Optimizer::Step1d()
{
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        ++num_step1d;
        Span* span = *spani;

        // Update span terms based on new bc offsets
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
        {
            IntNum intn;
            bool ok = CalcDist(term->m_loc, term->m_loc2, &intn);
            ok = ok;    // avoid warning due to assert usage
            assert(ok && "could not calculate bc distance");
            term->m_cur_val = term->m_new_val;
            term->m_new_val = intn.getInt();
        }

        if (span->RecalcNormal())
        {
            // Exceeded threshold, add span to QB
            m_QB.push(&(*span));
            span->m_active = Span::ON_Q;
            ++num_initial_qb;
        }
    }

    // Do we need step 2?  If not, go ahead and exit.
    return m_QB.empty();
}

bool
Optimizer::Step1e(Errwarns& errwarns)
{
    bool saw_error = false;

    // Update offset-setters values
    for (std::vector<OffsetSetter>::iterator os=m_offset_setters.begin(),
         osend=m_offset_setters.end(); os != osend; ++os)
    {
        if (!os->m_bc)
            continue;
        os->m_thres = os->m_bc->getNextOffset();
        os->m_new_val = os->m_bc->getOffset();
        os->m_cur_val = os->m_new_val;
        ++num_offset_setters;
    }

    // Build up interval tree
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        Span* span = *spani;
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
            ITreeAdd(*span, *term);
    }

    // Look for cycles in times expansion (span.id==0)
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        Span* span = *spani;
        if (span->m_id > 0)
            continue;
        try
        {
            m_itree.Enumerate(static_cast<long>(span->m_bc.getIndex()),
                              static_cast<long>(span->m_bc.getIndex()),
                              BIND::bind(&Optimizer::CheckCycle, this, _1,
                                         REF::ref(*span)));
        }
        catch (Error& err)
        {
            errwarns.Propagate(span->m_bc.getLine(), err);
            saw_error = true;
        }
    }

    return saw_error;
}

bool
Optimizer::Step2(Errwarns& errwarns)
{
    bool saw_error = false;

    while (!m_QA.empty() || !m_QB.empty())
    {
        Span* span;

        // QA is for TIMES, update those first, then update non-TIMES.
        // This is so that TIMES can absorb increases before we look at
        // expanding non-TIMES BCs.
        if (!m_QA.empty())
        {
            span = m_QA.front();
            m_QA.pop();
        }
        else
        {
            span = m_QB.front();
            m_QB.pop();
        }

        if (span->m_active == Span::INACTIVE)
            continue;
        span->m_active = Span::ACTIVE;  // no longer in Q

        // Make sure we ended up ultimately exceeding thresholds; due to
        // offset BCs we may have been placed on Q and then reduced in size
        // again.
        if (!span->RecalcNormal())
            continue;

        ++num_expansions;

        unsigned long orig_len = span->m_bc.getTotalLen();

        bool still_depend =
            Expand(span->m_bc, span->m_id, span->m_cur_val, span->m_new_val,
                   &span->m_neg_thres, &span->m_pos_thres, errwarns);

        if (errwarns.getNumErrors() > 0)
        {
            // error
            saw_error = true;
            continue;
        }
        else if (still_depend)
        {
            // another threshold, keep active
            for (Span::Terms::iterator term=span->m_span_terms.begin(),
                 endterm=span->m_span_terms.end(); term != endterm; ++term)
                term->m_cur_val = term->m_new_val;
            span->m_cur_val = span->m_new_val;
        }
        else
            span->m_active = Span::INACTIVE;    // we're done with this span

        long len_diff = span->m_bc.getTotalLen() - orig_len;
        if (len_diff == 0)
            continue;   // didn't increase in size

        // Iterate over all spans dependent across the bc just expanded
        m_itree.Enumerate(static_cast<long>(span->m_bc.getIndex()),
                          static_cast<long>(span->m_bc.getIndex()),
                          BIND::bind(&Optimizer::ExpandTerm, this, _1,
                                     len_diff));

        // Iterate over offset-setters that follow the bc just expanded.
        // Stop iteration if:
        //  - no more offset-setters in this section
        //  - offset-setter didn't move its following offset
        std::vector<OffsetSetter>::iterator os =
            m_offset_setters.begin() + span->m_os_index;
        long offset_diff = len_diff;
        while (os != m_offset_setters.end()
               && os->m_bc
               && os->m_bc->getContainer() == span->m_bc.getContainer()
               && offset_diff != 0)
        {
            unsigned long old_next_offset =
                os->m_cur_val + os->m_bc->getTotalLen();

            assert((offset_diff >= 0 ||
                    static_cast<unsigned long>(-offset_diff) <= os->m_new_val)
                   && "org/align went to negative offset");
            os->m_new_val += offset_diff;

            orig_len = os->m_bc->getTailLen();
            long neg_thres_temp, pos_thres_temp;
            Expand(*os->m_bc, 1, static_cast<long>(os->m_cur_val),
                   static_cast<long>(os->m_new_val), &neg_thres_temp,
                   &pos_thres_temp, errwarns);
            os->m_thres = static_cast<long>(pos_thres_temp);

            offset_diff =
                os->m_new_val + os->m_bc->getTotalLen() - old_next_offset;
            len_diff = os->m_bc->getTailLen() - orig_len;
            if (len_diff != 0)
                m_itree.Enumerate(static_cast<long>(os->m_bc->getIndex()),
                                  static_cast<long>(os->m_bc->getIndex()),
                                  BIND::bind(&Optimizer::ExpandTerm, this, _1,
                                             len_diff));

            os->m_cur_val = os->m_new_val;
            ++os;
        }
    }

    return saw_error;
}

} // anonymous namespace

namespace yasm {

void
Object::UpdateBytecodeOffsets(Errwarns& errwarns)
{
    for (section_iterator sect=m_sections.begin(), end=m_sections.end();
         sect != end; ++sect)
        sect->UpdateOffsets(errwarns);
}

void
Object::Optimize(Errwarns& errwarns)
{
    Optimizer opt;
    unsigned long bc_index = 0;
    bool saw_error = false;

    // Step 1a
    for (section_iterator sect=m_sections.begin(), sectend=m_sections.end();
         sect != sectend; ++sect)
    {
        unsigned long offset = 0;

        // Set the offset of the first (empty) bytecode.
        sect->bytecodes_first().setIndex(bc_index++);
        sect->bytecodes_first().setOffset(0);

        // Iterate through the remainder, if any.
        for (Section::bc_iterator bc=sect->bytecodes_begin(),
             bcend=sect->bytecodes_end(); bc != bcend; ++bc)
        {
            bc->setIndex(bc_index++);
            bc->setOffset(offset);

            CalcLen(*bc, BIND::bind(&Optimizer::AddSpan, &opt,
                                    _1, _2, _3, _4, _5),
                    errwarns);
            if (errwarns.getNumErrors() > 0)
                saw_error = true;
            else
            {
                if (bc->getSpecial() == Bytecode::Contents::SPECIAL_OFFSET)
                    opt.AddOffsetSetter(*bc);

                offset = bc->getNextOffset();
            }
        }
    }

    if (saw_error)
        return;

    // Step 1b
    if (opt.Step1b(errwarns))
        return;

    // Step 1c
    UpdateBytecodeOffsets(errwarns);
    if (errwarns.getNumErrors() > 0)
        return;

    // Step 1d
    if (opt.Step1d())
        return;

    // Step 1e
    if (opt.Step1e(errwarns))
        return;

    // Step 2
    if (opt.Step2(errwarns))
        return;

    // Step 3
    UpdateBytecodeOffsets(errwarns);
}

} // namespace yasm
