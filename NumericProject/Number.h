#pragma once

#include <vector>
#include <iostream>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <assert.h>

template <typename T>
class ICheckForOverflow
{
public:
    // Return the overflow amount and clear it.
    virtual T GetOverflow() = 0;

    // Return the overflow amount.
    virtual T PeekOverflow() const = 0;
};

template <typename TNum>
class BCD : public ICheckForOverflow < BCD<TNum> >
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

    void Print(std::ostream& out, bool leadingZeroes = false) const
    {
        bool havePrinted = leadingZeroes;
        for (int i = sizeof(TNum) * 8 / 4 - 1; i >= 0; i--)
        {
            TNum offset = i * 4;
            TNum mask = 0xF << offset;
            TNum place = (m_value & mask) >> offset;

            if (havePrinted || (place != 0))
            {
                out << (int)place;
                havePrinted = true;
            }
        };
    }

    virtual BCD GetOverflow()
    {
        TNum temp = m_overflow;
        m_overflow = 0;
        return temp;
    }

    virtual BCD PeekOverflow() const
    {
        return m_overflow;
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
        for (int i = 0; i < sizeof(TNum) * 8 / 4; i++)
        {
            TNum place = value % nextPlaceValue;
            value -= place;

            if (placeValue > 0)
                place /= placeValue;

            m_value |= place << (4 * i);

            if (value == 0)
                break;

            placeValue = nextPlaceValue;
            nextPlaceValue *= 10;
        }

        if (value != 0)
            m_overflow = value / placeValue;
    }

    static void Add(const BCD& x, const BCD& y, BCD& result)
    {
        // Result must be either zero, or an alias of x.
        assert((&result == &x)
            || ((result.m_value == 0) && (result.m_overflow == 0)));

        TNum carry = 0;
        for (int i = 0; i < sizeof(TNum) * 8 / 4; i++)
        {
            TNum offset = 4 * i;
            TNum mask = 0xF << offset;
            TNum digitX = (x.m_value & mask) >> offset;
            TNum digitY = (y.m_value & mask) >> offset;
            TNum digitResult = digitX + digitY + carry;
            if (digitResult >= 10)
            {
                carry = digitResult / 10;
                digitResult %= 10;
            }
            else
            {
                carry = 0;
            }

            result.m_value &= ~mask;
            result.m_value |= digitResult << offset;
        }

        result.m_overflow = x.m_overflow + y.m_overflow + carry;
    }

private:
    TNum m_value;
    TNum m_overflow;
};

template <typename TNum>
class BigInt
{
public:
    BigInt()
    {
        m_words.push_back(0);
    }

    BigInt(TNum value)
    {
        m_words.push_back(value);
        CheckOverflow();
    }

    template <typename X>
    typename std::enable_if<std::is_same<BCD<X>, TNum>::value, void>::type
        Print(std::ostream& out) const
    {
        bool first = true;
        for (std::vector<BCD<X>>::const_reverse_iterator it = m_words.crbegin() + 1, end = m_words.crend(); it != end; ++it)
        {
            it->Print(out, !first);
            first = false;
        }
    }

    template <typename X>
    typename std::enable_if<!std::is_same<BCD<X>, TNum>::value, void>::type
        Print(std::ostream& out) const
    {
        // convert to a BigInt<BCD<TNum>> somehow
        //TODO
        out << "BigInt::Print is not implemented.";
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

private:
    void CheckOverflow(size_t startAt = 0)
    {
        for (size_t i = startAt, n = m_words.size(); i < n; i++)
        {
            TNum& word = m_words[i];
            TNum overflow = GetOverflow(word);
            if (overflow == 0)
            {
                break;
            }
            else
            {
                if (i == n - 1)
                    m_words.push_back(0);

                m_words[i + 1] += overflow;
            }
        }

        if (m_words.back() != 0)
            m_words.push_back(0);
    }

    template <typename T>
    typename std::enable_if<
        std::is_base_of<ICheckForOverflow<TNum>, T>::value,
        TNum>::type
        GetOverflow(T& num)
    {
        return num.GetOverflow();
    }

    template <typename T>
    typename std::enable_if<
        !std::is_base_of<ICheckForOverflow<TNum>, T>::value,
        TNum>::type
        GetOverflow(T& num)
    {
        // For primitive numeric types, we leave the top bit open as a carry bit.
        const int bits = sizeof(T) * 8 - 1;
        const T mask = static_cast<T>(1) << bits;

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

    static void Add(const BigInt& x, const BigInt& y, BigInt& result)
    {
        // Result must be either zero, or an alias of x.
        assert((&result == &x)
            || ((result.m_words.size() == 1) && (result.m_words[0] == 0)));

        if (result.m_words.size() == 1)
            result.m_words.resize(std::max(x.m_words.size(), y.m_words.size()));

        for (size_t i = 0, n = std::max(x.m_words.size(), y.m_words.size()); i < n; i++)
        {
            TNum xn = (i < x.m_words.size()) ? x.m_words[i] : 0;
            TNum yn = (i < y.m_words.size()) ? y.m_words[i] : 0;

            result.m_words[i] = xn + yn;

            result.CheckOverflow(i);
        }
    }

private:
    std::vector<TNum> m_words;
};

