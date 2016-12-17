#include "include/radix_iterator.hpp"

/*
 * check that casting works for compatible types
 */

#include <cassert>

template <typename From, typename To>
void test(From ptr)
{
	using T1 = radix_iterator<From>;
	using T2 = radix_iterator<To>;

	T1 x{ptr, ptr, ptr}; // empty is still valid
	T2 y{static_cast<T2>(x)};

	assert((x.begin() == y.begin()) && "casting should not change range");
	assert((x.end() == y.end()) && "casting should not change range");
	assert((x.get() == y.get()) && "casting should not change location");

	assert(x.invariants());
	assert(y.invariants());
}

int main()
{
	int x = 0;
	test<int*, const volatile int*>(&x);
}
