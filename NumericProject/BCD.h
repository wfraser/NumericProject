#pragma once

#include "CeilLog2.h"
#include "BitsMask.h"
#include "BaseInterfaces.h"

template <typename TNum, int Base = 10, typename Enable = void>
class BCD
{
};

//
// Binary Coded Digits
//
// a.k.a. Binary Coded Decimal, but this one can be used for any number base.
//
template <typename TNum, int Base>
class BCD<TNum, Base, typename std::enable_if<std::is_unsigned<TNum>::value>::type> :
    public IPrintNumberInBase<Base>,
    public ICheckForOverflow<BCD<TNum, Base>>,
    protected ISegmentedNumber<TNum>
{
public:
    static const size_t BitsPerDigit = CeilLog2<Base + 1>::value;
    static const size_t DigitsPerWord = sizeof(TNum) * 8 / BitsPerDigit;
    static const TNum MaxWordValue = static_cast<TNum>((1 << BitsPerDigit) - 1);

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
        this->Add(*this, other, *this);
        return *this;
    }

    BCD operator+(const BCD& other) const
    {
        BCD result(0);
        this->Add(*this, other, result);
        return result;
    }

    BCD& operator*=(const TNum& word)
    {
        this->Multiply(word, *this, *this);
        return *this;
    }

    BCD operator*(const TNum& word) const
    {
        BCD result(0);
        this->Multiply(word, *this, result);
        return result;
    }

    BCD& operator*=(const BCD& other)
    {
        assert(m_overflow == 0);
        this->Multiply(*this, other, *this);
        return *this;
    }

    BCD operator*(const BCD& other) const
    {
        assert(m_overflow == 0);
        BCD result(0);
        this->Multiply(*this, other, result);
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

        TNum offset = IntCast(BitsPerDigit * index);
        TNum mask = BitsMask<BitsPerDigit>::value << offset;
        return (m_value & mask) >> offset;
    }

    virtual TNum SetWord(size_t index, const TNum& value)
    {
        if (index > GetWordCount())
            throw std::invalid_argument("index is too high");

        TNum offset = IntCast(BitsPerDigit * index);
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
                BCD::Add(result, partialResult, result);
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

    static TNum IntCast(uintmax_t value)
    {
        assert(value <= MaxWordValue);
        return static_cast<TNum>(value);
    }

private:
    TNum m_value;
    TNum m_overflow;
};