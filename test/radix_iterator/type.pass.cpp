#include "include/radix_iterator.hpp"

/*
 * Check the member types, and handling of unusual pointers
 */

#include <memory>
#include <iterator>
#include <type_traits>

template <typename T>
void test()
{
	using C = radix_iterator<T>;

	static_assert(std::is_base_of<std::bidirectional_iterator_tag, typename C::iterator_category>::value,
	              "must be at least a bidirection iterator");
	static_assert(std::is_same<typename C::value_type, typename std::pointer_traits<T>::element_type>::value,
	              "value type does not match type shown through pointer traits");
	static_assert(std::is_same<typename C::reference, typename std::pointer_traits<T>::element_type&>::value,
	              "reference does not match reference to type shown through pointer traits");
	static_assert(std::is_same<typename C::pointer, typename std::pointer_traits<T>::pointer>::value,
	              "pointer does not match pointer shown through pointer traits");
	static_assert(std::is_same<typename C::difference_type, typename std::pointer_traits<T>::difference_type>::value,
	              "difference type does not match difference type shown through pointer traits");
}

struct odd_ptr
{
	// odd types that wouldn't normally be used
	using element_type = int* const *;
	using difference_type = signed char;
};

int main()
{
	test<int*>();
	test<odd_ptr>();
	test<const volatile int*>();
}
