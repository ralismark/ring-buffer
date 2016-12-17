#include "include/radix_iterator.hpp"

#include "../unused.hpp"

/*
 * testing const-correctness
 * only compile check needed
 */

int main()
{
	using T = int;
	T vals[4] = { 1, 2, 3, 4 };

	{
		using C = radix_iterator<T*>;
		using C2 = radix_iterator<const T*>;
		const C x{ std::begin(vals), std::end(vals), &vals[0] };

		// call to check const-ness
		// no asserts needed
		x.invariants();
		x.range_empty();
		*x;
		x.operator->();
		x.begin();
		x.end();
		x.get();

		C2 y{x};

		bool a = x == y && x != x;
		unused(a);
	}
}
