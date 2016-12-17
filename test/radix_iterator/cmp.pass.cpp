#include "include/radix_iterator.hpp"

#include <cassert>

int main()
{
	using T = int;
	T vals[4] = { 1, 2, 3, 4 };

	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[0] },
		  y{ std::begin(vals), std::end(vals), &vals[0] };

		assert((x == y) && "equal init should be equal");
		assert((!(x != y)) && "not equal should be complement of equal");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[0] },
		  y{ std::begin(vals), std::end(vals), &vals[1] };

		assert((!(x == y)) && "different init shoud be different");
		assert((x != y) && "not equal should be complement of equal");
	}
}
