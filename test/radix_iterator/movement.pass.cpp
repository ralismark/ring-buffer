#include "include/radix_iterator.hpp"

/*
 * testing increment and decrement together
 */

#include <cassert>

int main()
{
	using T = int;
	T vals[4] = { 1, 2, 3, 4 };

	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[0] };
		C y{ x };

		--++x;

		assert((x == y) && "increment and decrement should cancel each other out");
	}
}
