#include "include/radix_iterator.hpp"

/*
 * check that a cast fails with incompatible type
 */

#include "../unused.hpp"

int main()
{
	using T1 = radix_iterator<int*>;
	using T2 = radix_iterator<char*>;

	T1 x;
	T2 y{T2(x)}; // error
	unused(y);
}
