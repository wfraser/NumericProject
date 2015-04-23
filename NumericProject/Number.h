#pragma once

#include <vector>
#include <iostream>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <assert.h>

class IPrintDecimalNumber
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

template <typename T>
class ISegmentedNumber
{
public:
    virtual T GetWord(size_t index) const = 0;
    virtual T SetWord(size_t index, const T& value) = 0;
    virtual size_t GetWordCount() const = 0;
    virtual void Resize(size_t newSize) = 0;
    virtual void SetOverflow(const T& value) = 0;
    virtual T GetOverflowSegment() const = 0;
    virtual bool IsZero() const = 0;

protected:
    void Add(const ISegmentedNumber& x, const ISegmentedNumber& y, ISegmentedNumber& result)
    {
        // Result must be either zero, or an alias of x.
        assert((&result == &x) || result.IsZero());

        result.Resize(std::max(x.GetWordCount(), y.GetWordCount()));

        T carry = 0;
        for (size_t i = 0, n = std::max(x.GetWordCount(), y.GetWordCount()); i < n; i++)
        {
            T xn = (i < x.GetWordCount()) ? x.GetWord(i) : 0;
            T yn = (i < y.GetWordCount()) ? y.GetWord(i) : 0;
            T rn = xn + yn + carry;

            carry = result.SetWord(i, rn);
        }

        carry += x.GetOverflowSegment() + y.GetOverflowSegment();
        result.SetOverflow(carry);
    }
};

template <typename TNum>
class BCD :
    public IPrintDecimalNumber,
    public ICheckForOverflow < BCD<TNum> >,
    protected ISegmentedNumber< TNum >
{
public:
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

    BCD operator+(const BCD& other)
    {
        BCD result(0);
        Add(*this, other, result);
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
    // IPrintDecimalNumber
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
        if (index > sizeof(TNum) * 8 / 4)
            throw std::invalid_argument("index is too high");

        TNum offset = 4 * index;
        return (m_value & (0xF << offset)) >> offset;
    }

    virtual TNum SetWord(size_t index, const TNum& value)
    {
        if (index > sizeof(TNum) * 8 / 4)
            throw std::invalid_argument("index is too high");

        TNum offset = 4 * index;
        TNum valueCorrected = value;

        TNum carry = 0;
        if (valueCorrected >= 10)
        {
            carry = valueCorrected / 10;
            valueCorrected %= 10;
        }

        m_value &= ~(0xF << offset);
        m_value |= valueCorrected << offset;

        return carry;
    }

    virtual void SetOverflow(const TNum& value)
    {
        m_overflow = value;
    }

    virtual size_t GetWordCount() const
    {
        return BCD<TNum>::GetStaticWordCount();
    }

    static size_t GetStaticWordCount()
    {
        return sizeof(TNum) * 8 / 4;
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
    void Init(TNum value)
    {
        // Special case for small values.
        if (value < 10)
        {
            m_value = value;
            m_overflow = 0;
            return;
        }

        m_value = 0;
        m_overflow = 0;

        TNum placeValue = 0;
        TNum nextPlaceValue = 10;
        for (size_t i = 0; i < GetWordCount(); i++)
        {
            TNum place = value % nextPlaceValue;
            value -= place;

            if (placeValue > 0)
                place /= placeValue;

            SetWord(i, place);

            if (value == 0)
                break;

            placeValue = nextPlaceValue;
            nextPlaceValue *= 10;
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
        m_words.push_back(0);
    }

    void Print(std::ostream& out) const
    {
        PrintImpl<TNum>(out);
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
            assert(m_words.back() == 0);
            m_words.back() = value;
            m_words.push_back(0);
        }
    }

    virtual size_t GetWordCount() const
    {
        return m_words.size();
    }

    virtual void Resize(size_t newSize)
    {
        if (m_words.size() < newSize)
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

    template <typename X>
    typename std::enable_if<std::is_base_of<IPrintDecimalNumber, X>::value, void>::type
        PrintImpl(std::ostream& out) const
    {
        bool first = true;
        for (std::vector<TNum>::const_reverse_iterator it = m_words.crbegin() + 1, end = m_words.crend(); it != end; ++it)
        {
            it->Print(out, !first);
            first = false;
        }
    }

    template <typename X>
    typename std::enable_if<!std::is_base_of<IPrintDecimalNumber, X>::value, void>::type
        PrintImpl(std::ostream& out) const
    {
        // convert to a BigInt<BCD<TNum>> somehow
        //TODO
        out << "BigInt::Print is not implemented.";
    }

private:
    std::vector<TNum> m_words;
};
