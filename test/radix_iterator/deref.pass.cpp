#include "include/radix_iterator.hpp"

/*
 * test the deferencing and pointer access member
 */

#include <cassert>

struct S
{
	int a, b;
};

int main()
{
	{
		using T = radix_iterator<S*>;
		S val;

		T x{&val, &val + 1, &val};

		assert(&*x == &val);
		assert(&x->a == &val.a);
		assert(&x->b == &val.b);

		assert(x.invariants());
	}
	{
		using T = radix_iterator<S*>;
		S val{0, 1};

		T x{&val, &val + 1, &val};

		assert(x->a == 0 && x->b == 1);
		x->a = 2;
		assert(x->a == 2 && x->b == 1);
		*x = S{3, 4};
		assert(x->a == 3 && x->b == 4);

		assert(x.invariants());
	}
}
