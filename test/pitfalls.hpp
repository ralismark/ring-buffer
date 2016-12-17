#pragma once

/**
 * @file
 *
 * Checks pitfalls and logic errors with containers
 *
 * A class, pitfall, is provided which eases testing for several pitfalls when
 * implementing containers. It tests:
 *  1. address of operator (which may be overloaded)
 *  2. logical operators (may also be overloaded)
 *  3. comma operator (could be overloaded)
 *  3. raw copying (bypassing the copy constructors and such)
 *  4. skipped destructors (freeing memory without destroying the contained objects)
 */

#include <cassert>

class pitfall
{
public: // statics
private: // internal statics
	friend struct verifier;
	struct verifier
	{
		~verifier()
		{
			assert(pitfall::live_count == 0);
		}
	};

	// check at end of program
	static verifier global_check;

	static int live_count;
private: // variables
	pitfall* self;
public: // methods
	pitfall()
		noexcept
		: self(this)
	{
		++live_count;
	}

	pitfall(const pitfall&)
		noexcept
		: self(this)
	{
		++live_count;
	}

	~pitfall()
	{
		assert(self == this);
		--live_count;
	}

	pitfall& operator=(const pitfall& other)
	{
		assert(self == this);
		assert(other.self == &other);
		return *this;
	}

	void check() const
	{
		assert(self == this);
	}

	const pitfall* operator&() const
	{
		assert(false && "addressof operator");
		return this;
	}

	template <typename T>
	const pitfall& operator,(T&& rhs) const
	{
		assert(false && "comma operator");
		return *this;
	}

	template <typename T>
	friend T operator&&(T&& lhs, const pitfall& rhs)
	{
		assert(false && "logical and operator");
		return lhs;
	}

	template <typename T>
	friend T operator&&(const pitfall& lhs, T&& rhs)
	{
		assert(false && "logical and operator");
		return rhs;
	}

	template <typename T>
	friend T operator||(T&& lhs, const pitfall& rhs)
	{
		assert(false && "logical or operator");
		return lhs;
	}

	template <typename T>
	friend T operator||(const pitfall& lhs, T&& rhs)
	{
		assert(false && "logical or operator");
		return rhs;
	}

};

int pitfall::live_count = 0;
