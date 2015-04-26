#pragma once

#include <vector>
#include <iostream>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <memory>
#include <assert.h>

#include "CeilLog2.h"
#include "BitsMask.h"
#include "BaseInterfaces.h"

template <typename TNum, int Base>
class BCD;

template <typename TNum>
class BigInt :
    protected ISegmentedNumber<TNum>
{
public:
    BigInt()
    {
        m_words.push_back(0);
    }

    BigInt(TNum value)
    {
        TNum carry = CheckOverflowImpl(value);
        m_words.push_back(value);
        if (carry != 0)
            m_words.push_back(carry);
    }

    template <int NumberBase = 10>
    void Print(std::ostream& out) const
    {
        PrintImpl<TNum, NumberBase>(out);
    }

    BigInt& operator+=(const BigInt& other)
    {
        Add(*this, other, *this);
        return *this;
    }

    BigInt operator+(const BigInt& other)
    {
        BigInt result;
        Add(*this, other, result);
        return result;
    }

    BigInt& operator*=(const TNum& word)
    {
        Multiply(word, *this, *this);
        return *this;
    }

    BigInt& operator*(const TNum& word)
    {
        BigInt result;
        Multiply(word, *this, result);
        return result;
    }

protected:
    //
    // ISegmentedNumber
    //

    virtual TNum GetWord(size_t index) const
    {
        return m_words[index];
    }

    virtual TNum SetWord(size_t index, const TNum& value)
    {
        TNum valueCorrected = value;
        TNum carry = CheckOverflowImpl(valueCorrected);
        m_words[index] = valueCorrected;
        return carry;
    }

    virtual void SetOverflow(const TNum& value)
    {
        if (value != 0)
        {
            m_words.push_back(value);
        }
    }

    virtual size_t GetWordCount() const
    {
        return m_words.size();
    }

    virtual void Resize(size_t newSize)
    {
        if (newSize < m_words.size())
        {
            throw std::logic_error("Cannot shrink class BigInt.");
        }

        m_words.resize(newSize);
    }

    virtual TNum GetOverflowSegment() const
    {
        // BigInt doesn't have an overflow segment; it resizes itself instead.
        return 0;
    }

    virtual bool IsZero() const
    {
        return (m_words.size() == 1) && (m_words.front() == 0);
    }

private:
    template <typename X>
    typename std::enable_if<std::is_base_of<ICheckForOverflow<X>, X>::value, X>::type
        CheckOverflowImpl(X& num)
    {
        // If TNum implements ICheckForOverflow, use its method instead.
        return num.GetAndClearOverflow();
    }

    template <typename X>
    typename std::enable_if<!std::is_base_of<ICheckForOverflow<X>, X>::value, X>::type
        CheckOverflowImpl(X& num)
    {
        // For primitive numeric types, we leave the top bit open as a carry bit.
        static const int bits = sizeof(X) * 8 - 1;
        static const X mask = static_cast<X>(1) << bits;

        if (0 != (num & mask))
        {
            num &= ~mask;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    template <typename X, int Base>
    typename std::enable_if<std::is_base_of<IPrintNumberInBase<Base>, X>::value, void>::type
        PrintImpl(std::ostream& out) const
    {
        bool first = true;
        for (std::vector<TNum>::const_reverse_iterator it = m_words.crbegin(), end = m_words.crend(); it != end; ++it)
        {
            it->Print(out, !first);
            first = false;
        }
    }

    template <typename X, int Base>
    typename std::enable_if<!std::is_base_of<IPrintNumberInBase<Base>, X>::value, void>::type
        PrintImpl(std::ostream& out) const
    {
        // convert to a BCD-based BigNum

        BigInt<BCD<TNum, Base>> converted;
        BigInt<BCD<TNum, Base>> placeValue = 1;

        for (const TNum& word : m_words)
        {
            TNum bitMask = 1;
            for (size_t bit = 0; bit < sizeof(TNum) * 8 - 1; bit++)
            {
                if ((word & bitMask) != 0)
                {
                    converted += placeValue;
                }

                bitMask <<= 1;
                placeValue *= 2;
            }
        }

        // Now this will call the other implementation.
        converted.Print(out);
    }

private:
    std::vector<TNum> m_words;
};