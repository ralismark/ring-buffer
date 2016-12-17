#include "include/radix_iterator.hpp"

/*
 * Check constructors and valid initalising of values
 */

#include <cassert>
#include <iterator>

int main()
{
	using T = int;
	{
		using C = radix_iterator<T*>;
		C null, null2;

		assert((null.begin() == nullptr) && "default init, should be null");
		assert((null.end() == nullptr) && "default init, should be null");
		assert((null.get() == nullptr) && "default init, should be null");

		assert(null.invariants() && "null should be valid");
	}
	{
		using C = radix_iterator<T*>;
		T vals[4] = { 1, 2, 3, 4 };
		C a{std::begin(vals),     std::end(vals),     &vals[1]},
		  b{std::begin(vals),     std::end(vals) - 1, &vals[2]},
		  c{std::begin(vals) + 1, std::end(vals),     &vals[2]};

		assert((a.begin() == b.begin()
		     && a.begin() != c.begin()
		     && b.begin() != c.begin())
		       && "same init should result in the same value");

		assert((a.end() != b.end()
		     && a.end() == c.end()
		     && b.end() != c.end())
		       && "same init should result in the same value");

		assert((a.get() != b.get()
		     && a.get() != c.get()
		     && b.get() == c.get())
		       && "same init should result in the same value");

		assert(a.invariants() && "initialised should be valid");
		assert(b.invariants() && "initialised should be valid");
		assert(c.invariants() && "initialised should be valid");

	}
}
