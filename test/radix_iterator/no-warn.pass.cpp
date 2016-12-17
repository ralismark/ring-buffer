#include "include/radix_iterator.hpp"

/*
 * ensure that code compiles cleanly
 * preferrably, use -Weverything -Wno-c++98-compat
 */

#include "../unused.hpp"

template <typename T>
void test()
{
	T vals[4] = { 1, 2, 3, 4 };
	using C = radix_iterator<T*>;

	C x,
	  y{ std::begin(vals), std::end(vals), &vals[0] };

	y.invariants();
	y.range_empty();

	*y;
	y.operator->();

	++y; y++;
	--y; y--;

	y.begin(); y.get(); y.end();
	unused(y == y);
	unused(y != y);

	using C2 = radix_iterator<const T*>;
	unused(C2(x));
}

void check();

void check()
{
	// don't actually call it
	// but still instantiate the function
	test<int>();
}

int main()
{
}
