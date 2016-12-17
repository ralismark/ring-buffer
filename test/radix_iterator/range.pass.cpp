#include "include/radix_iterator.hpp"

/*
 * check member function range_empty()
 */

#include <cassert>

int main()
{
	using T = int;
	T vals[] = { 1, 2, 3, 4 };

	{
		using C = radix_iterator<T*>;
		C blank{ std::begin(vals), std::begin(vals), std::begin(vals) };

		assert((blank.range_empty()) && "empty range should be empty");

		assert((blank.invariants()) && "constructed correctly");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[1] };

		assert((!x.range_empty()) && "non-empty range should not be empty");
		assert((x.invariants()) && "constructed correctly");

		assert((x.begin() == std::begin(vals)) && "range begin should be correct");
		assert((x.end() == std::end(vals)) && "range begin should be correct");

	}
}
