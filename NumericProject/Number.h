#pragma once

#include <vector>
#include <iostream>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <memory>
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

//
// Binary Coded Digits
//
// a.k.a. Binary Coded Decimal, but this one can be used for any number base.
//
template <typename TNum, int Base = 10>
class BCD :
    public IPrintNumberInBase<Base>,
    public ICheckForOverflow<BCD<TNum, Base>>,
    protected ISegmentedNumber<TNum>
{
public:
    static const size_t BitsPerDigit = CeilLog2<Base + 1>::value;
    static const size_t DigitsPerWord = sizeof(TNum) * 8 / BitsPerDigit;

    BCD() :
        m_value(0),
        m_overflow(0)
    {
    }

    BCD(TNum value)
    {
        Init(value);
    }

    BCD& operator+=(const BCD& other)
    {
        Add(*this, other, *this);
        return *this;
    }

    BCD operator+(const BCD& other) const
    {
        BCD result(0);
        Add(*this, other, result);
        return result;
    }

    BCD& operator*=(const TNum& word)
    {
        Multiply(word, *this, *this);
        return *this;
    }

    BCD operator*(const TNum& word) const
    {
        BCD result(0);
        Multiply(word, *this, result);
        return result;
    }

    BCD& operator*=(const BCD& other)
    {
        assert(m_overflow == 0);
        Multiply(*this, other, *this);
        return *this;
    }

    BCD operator*(const BCD& other) const
    {
        assert(m_overflow == 0);
        BCD result(0);
        Multiply(*this, other, result);
        return result;
    }

    bool operator==(const BCD& other) const
    {
        return ((m_value == other.m_value) && (m_overflow == other.m_overflow));
    }

    bool operator!=(const BCD& other) const
    {
        return !operator==(other);
    }

    //
    // IPrintNumberInBase
    //

    void Print(std::ostream& out, bool leadingZeroes = false) const
    {
        bool havePrinted = leadingZeroes;
        for (size_t i = GetWordCount(); i >= 1; i--)
        {
            TNum place = GetWord(i - 1);

            if (havePrinted || (place != 0))
            {
                out << (int)place;
                havePrinted = true;
            }
        };
    }

    //
    // ICheckForOverflow:
    //

    virtual BCD GetAndClearOverflow()
    {
        BCD overflow = GetOverflowSegment();
        m_overflow = 0;
        return overflow;
    }

    virtual BCD PeekOverflow() const
    {
        return m_overflow;
    }

protected:
    //
    // ISegmentedNumber
    //

    virtual TNum GetWord(size_t index) const
    {
        if (index > GetWordCount())
            throw std::invalid_argument("index is too high");

        TNum offset = BitsPerDigit * index;
        TNum mask = BitsMask<BitsPerDigit>::value << offset;
        return (m_value & mask) >> offset;
    }

    virtual TNum SetWord(size_t index, const TNum& value)
    {
        if (index > GetWordCount())
            throw std::invalid_argument("index is too high");

        TNum offset = BitsPerDigit * index;
        TNum valueCorrected = value;

        TNum carry = 0;
        if (valueCorrected >= Base)
        {
            carry = valueCorrected / Base;
            valueCorrected %= Base;
        }

        m_value &= ~(BitsMask<BitsPerDigit>::value << offset);
        m_value |= valueCorrected << offset;

        return carry;
    }

    virtual void SetOverflow(const TNum& value)
    {
        m_overflow = value;
    }

    virtual size_t GetWordCount() const
    {
        return DigitsPerWord;
    }

    virtual void Resize(size_t newSize)
    {
        if (newSize != GetWordCount())
        {
            throw std::logic_error("class BCD cannot be resized.");
        }
    }

    virtual TNum GetOverflowSegment() const
    {
        return m_overflow;
    }

    virtual bool IsZero() const
    {
        return (m_value == 0) && (m_overflow == 0);
    }

private:
    static void Multiply(const BCD& x, const BCD& y, BCD& result)
    {
        assert((&x == &result) || result.IsZero());

        TNum placeValue = 1;
        for (size_t i = 0, n = x.GetWordCount(); i <= n; i++, placeValue *= Base)
        {
            TNum xword = (i == n) ? x.GetOverflowSegment() : x.GetWord(i);
            if (xword != 0)
            {
                BCD partialResult(0);
                ISegmentedNumber<TNum>::Multiply(xword, y, partialResult);
                ISegmentedNumber<TNum>::Multiply(placeValue, partialResult, partialResult);
                Add(result, partialResult, result);
            }
        }
    }

    void Init(TNum value)
    {
        // Special case for small values.
        if (value < Base)
        {
            m_value = value;
            m_overflow = 0;
            return;
        }

        m_value = 0;
        m_overflow = 0;

        TNum placeValue = 0;
        TNum nextPlaceValue = Base;
        for (size_t i = 0, n = GetWordCount(); i < n; i++)
        {
            TNum place = value % nextPlaceValue;
            value -= place;

            if (placeValue > 0)
                place /= placeValue;

            SetWord(i, place);

            if (value == 0)
                break;

            placeValue = nextPlaceValue;
            nextPlaceValue *= Base;
        }

        if (value != 0)
            m_overflow = value / placeValue;
    }

private:
    TNum m_value;
    TNum m_overflow;
};

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

//
// Compile-time constant Ceil(Log2(x)) values :)
//

template <size_t N>
struct CeilLog2
{
    enum { value = 1 + CeilLog2<(N >> 1)>::value };
};

template <>
struct CeilLog2<0>
{
    // Log2(0) is undefined.
};

template <>
struct CeilLog2<1>
{
    enum { value = 1 };
};

//
// Compile-time constant bit mask
//

template <size_t N>
struct BitsMask
{
    enum { value = BitsMask<N - 1>::value << 1 | 1 };
};

template <>
struct BitsMask<0>
{
    enum { value = 0 };
};