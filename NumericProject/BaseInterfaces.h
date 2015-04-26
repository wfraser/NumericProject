#pragma once

#include <iostream>
#include <algorithm>
#include <assert.h>

template <int Base>
class IPrintNumberInBase
{
public:
    virtual void Print(std::ostream& out, bool leadingZeroes = false) const = 0;
};

template <typename T>
class ICheckForOverflow
{
public:
    // Return the overflow amount and clear it.
    virtual T GetAndClearOverflow() = 0;

    // Return the overflow amount.
    virtual T PeekOverflow() const = 0;
};

template <typename TNum>
class ISegmentedNumber
{
public:
    virtual TNum GetWord(size_t index) const = 0;
    virtual TNum SetWord(size_t index, const TNum& value) = 0;
    virtual size_t GetWordCount() const = 0;
    virtual void Resize(size_t newSize) = 0;
    virtual void SetOverflow(const TNum& value) = 0;
    virtual TNum GetOverflowSegment() const = 0;
    virtual bool IsZero() const = 0;

protected:
    static ISegmentedNumber* MakeZero();

    //
    // result = x + y
    //
    static void Add(const ISegmentedNumber& x, const ISegmentedNumber& y, ISegmentedNumber& result)
    {
        // Result must be either zero, or an alias of x.
        assert((&result == &x) || result.IsZero());

        result.Resize(std::max(x.GetWordCount(), y.GetWordCount()));

        TNum carry = 0;
        for (size_t i = 0, n = std::max(x.GetWordCount(), y.GetWordCount()); i < n; i++)
        {
            TNum xn = (i < x.GetWordCount()) ? x.GetWord(i) : 0;
            TNum yn = (i < y.GetWordCount()) ? y.GetWord(i) : 0;
            TNum rn = xn + yn + carry;

            carry = result.SetWord(i, rn);
        }

        carry += x.GetOverflowSegment() + y.GetOverflowSegment();
        result.SetOverflow(carry);
    }

    //
    // result = x * y
    //
    static void Multiply(const TNum& word, const ISegmentedNumber& x, ISegmentedNumber& result)
    {
        assert((&result == &x) || result.IsZero());

        result.Resize(x.GetWordCount());

        TNum carry = 0;
        for (size_t i = 0, n = x.GetWordCount(); i < n; i++)
        {
            TNum xn = x.GetWord(i);
            TNum rn = xn * word + carry;

            carry = result.SetWord(i, rn);
        }

        carry += word * x.GetOverflowSegment();
        result.SetOverflow(carry);
    }
};