#include "include/ring_buffer.hpp"

/*
 * check that compilation produces no warnings
 * preferrably, use -Weverything -Wno-c++98-compat
 */

#include <memory>

template <typename T, typename A>
void test()
{
	using C = ring_buffer<T, A>;
	T vals[4];

	// {{{ basic fns

	// testing inits
	C a,
	  b(5),
	  c(std::begin(vals), std::end(vals)),
	  d{vals[0], vals[1], vals[2], vals[3]},
	  e(a),
	  f(std::move(b));

	// for const overloads
	const auto ca = a;

	// assign op
	a = e;
	e = std::move(a);
	a = { T(), T() };

	// assign memfn
	a.assign(5, T());
	a.assign(std::begin(vals), std::end(vals));
	a.assign({T(), T()});

	// get allocator
	a.get_allocator();

	// }}}

	// {{{ element access

	a.at(0);
	ca.at(0);
	a[0];
	ca[0];

	a.front();
	ca.front();
	a.back();
	ca.back();

	// }}}

	// {{{ iterators

	a.begin();
	ca.begin();
	ca.cbegin();
	a.end();
	ca.end();
	ca.cend();

	a.rbegin();
	ca.rbegin();
	ca.crbegin();
	a.rend();
	ca.rend();
	ca.crend();

	// }}}

	// {{{ capacity

	a.empty();
	a.size();
	a.max_size();
	a.reserve(50);
	a.capacity();
	a.shrink_to_fit();

	// }}}

	// {{{ modifiers

	a.clear();
	a.insert(a.begin(), vals[0]);
	a.insert(a.begin(), std::move(vals[0]));
	// a.insert(a.begin(), 5, vals[0])
	a.insert(a.begin(), std::begin(vals), std::end(vals));
	a.insert(a.begin(), {vals[0], vals[1]});

	a.push_front(vals[0]);
	a.push_front(std::move(vals[0]));
	a.emplace_front();
	a.pop_front();

	a.push_back(vals[0]);
	a.push_back(std::move(vals[0]));
	a.emplace_back();
	a.pop_back();

	a.swap(b);

	// }}}
}

void check();

void check()
{
	// don't actually call it
	// but still instantiate the function
	test<int, std::allocator<int>>();
}

int main()
{
}
