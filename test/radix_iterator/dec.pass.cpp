#include "include/radix_iterator.hpp"

/*
 * check the pre- and post-decrement operators
 */

#include <cassert>

int main()
{
	using T = int;
	T vals[4] = { 1, 2, 3, 4 };

	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), std::begin(vals) };

		C y{--x};

		assert((x == y) && "pre-decrement should return itself");
		assert((x.invariants()) && "invariants");

		assert((x.get() == &vals[3]) && "decrement past end should wrap");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[2] };

		--x;

		assert((x.get() == &vals[1]) && "decrement should go to the previous element");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), &vals[1] };
		C y{x};

		C z{x--};

		assert((x != y) && "post-decrement should not result in the same value");
		assert((z == y) && "post-decrement return should be old value");
		assert((x.invariants()) && "invariants");

		assert((y.begin() == z.begin() && y.end() == z.end()) && "returned range should be equal to the old range");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::end(vals), std::begin(vals) };
		C y{x};

		C z{x--};

		assert((x != y) && "post-increment should not result in the same value");
		assert((z == y) && "post-increment return should be old value");
		assert((y.invariants()) && "invariants");
	}
	{
		using C = radix_iterator<T*>;
		C x{ std::begin(vals), std::begin(vals), std::begin(vals) };
		C y{x};

		--x;

		assert((x == y) && "increment empty range should not change");
	}
}
