#ifndef BASE_SYSTEMPP_TYPEWRAPPER_H
#define BASE_SYSTEMPP_TYPEWRAPPER_H

#include "system++.h"

template <typename T, T Default>
class TypeWrapper
{
	T value;
public:
	static const T DEFAULT = Default;

	TypeWrapper()
	{
		value = Default;
	}

	TypeWrapper(T val)
	{
		value = val;
	}

	void reset() { value = Default; }

	operator T() { return value; }
	operator const T() const { return value; }
};

#define MAKE_TYPE(TYPE, NAME, DEFAULT) \
		template<TYPE Default> using NAME = TypeWrapper<TYPE, Default>; \
		typedef NAME<DEFAULT> NAME##Default;

MAKE_TYPE(int, Integer, 0)
MAKE_TYPE(bool, Bool, false)
#undef MAKE_TYPE


template <typename T>
class TriState
{
	T m_Value;
	bool m_IsSet;
public:
	TriState() : m_IsSet(false) {}
	TriState(T val) : m_Value(val), m_IsSet(true) {}
	TriState(const TriState<T>& other) : m_Value(other.m_Value), m_IsSet(other.m_IsSet) {}

	bool IsSet() const { return m_IsSet; }

	void Unset() { m_IsSet = false; }

	TriState<T>& operator=(const TriState<T>& other)
	{
		m_Value = other.m_Value;
		m_IsSet = other.m_IsSet;
		return *this;
	}

	T& operator=(const T& other)
	{
		m_Value = other;
		m_IsSet = true;
		return m_Value;
	}

	operator T()
	{
		dbg_assert(m_IsSet, "reading an unset tristate");
		return m_Value;
	}
};

#endif
