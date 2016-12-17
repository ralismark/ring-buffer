#pragma once

/**
 * \file
 *
 * Iterator which wrapping around after going out of bounds.
 *
 * This kind of iterator wraps around to the front after reaching the end, and
 * goes around to the end once the front is passed. As a result, it is suitable
 * iterator for the ring buffer, whose elements 'wrap around' past a certain
 * index.
 *
 * The iterator for a ring buffers is implemented this way to reduce code
 * duplication between the const and mutable variation.
 *
 * note: the template argument describes the pointer type, not the value type.
 */

/*
 * synopsis
 *
 * \code
 *
 * template <typename P>
 * class radix_iterator
 * {
 * public:
 *
 *      typedef std::bidirectional_iterator_tag iterator_category; // or greater
 *      typedef typename P::element_type        value_type;
 *      typedef value_type&                     reference;
 *      typedef P                               pointer;
 *      typedef typename P::difference_type     difference_type;
 *
 *      radix_iterator();
 *      radix_iterator(pointer front, pointer back, pointer pos);
 *
 *      reference operator*() const;
 *      pointer operator->() const;
 *
 *      radix_iterator& operator++();
 *      radix_iterator operator++(int);
 *      radix_iterator& operator--()
 *      radix_iterator operator--(int);
 *
 *      template <typename L, typename R>
 *      friend bool operator==(const radix_iterator<L>& lhs, const radix_iterator<R>& rhs);
 *
 *      template <typename L, typename R>
 *      friend bool operator!=(const radix_iterator<L>& lhs, const radix_iterator<R>& rhs);
 *
 *      // type cast operator
 *      template <typename U2>
 *      operator radix_iterator<U2>() const;
 *
 *      pointer get() const;
 *      pointer begin() const;
 *      pointer end() const;
 *
 *      bool invariants() const;
 * };
 *
 * \endcode
 */

// TODO(timmy): rework to allow for random access

#include <iterator>
#include <memory>

template <typename P>
class radix_iterator
{ // {{{ impl
private: // internal statics

	/// Allocator traits alias
	using ptraits = std::pointer_traits<P>;

public: // statics

	/*
	 * @typedef iterator_categury
	 *      Type of iterator. radix_iterator is a bidirectional iterator
	 *
	 * @typedef value_type
	 *      Type pointed to by the iterator. In this case, it is the first template argument
	 *
	 * @typedef difference_type
	 *      A type representing the offset between any two compatible iterators
	 *
	 * @typedef pointer
	 *      A pointer to the type iterated over (dependent on allocator)
	 *
	 * @typedef reference
	 *      A reference to the type iterated over (usually value_type&)
	 */

	using iterator_category = std::bidirectional_iterator_tag;
	using value_type        = typename ptraits::element_type;
	using difference_type   = typename ptraits::difference_type; // offset between two iterators
	using pointer           = P; // just a pointer
	using reference         = value_type&; // reference to contained type

private: // variables

	pointer front;   // first element of the range
	pointer back;    // one past the end of the range
	pointer current; // current element
	// always: current in [front, back)

public: // methods


	// TODO(timmy): add noexcept where appropriate

	/// default constructor, iterator not associated with any container
	radix_iterator()
		// noexcept(noexcept(pointer())) // is default init noexcept
		: front(nullptr), back(nullptr), current(nullptr)
	{
	}

	/// construct from a pointer range and a pointer in the range
	radix_iterator(pointer i_front, pointer i_back, pointer i_current)
		// noexcept(std::is_nothrow_copy_constructible<pointer>::value)
		: front(i_front), back(i_back), current(i_current)
	{
	}

	// default the other ctor & assign ops

	/// check to see if invariants hold
	bool invariants() const
	{
		if(back < front) {
			// container reversed
			return false;
		}
		if(this->range_empty()) {
			if(front != current || current != back) {
				// all should be equal is range is empty
				return false;
			}
		} else {
			if(current < front || back <= current) {
				// current location not in range
				return false;
			}
		}
		return true;
	}

	/// does the range contain no values
	bool range_empty() const
	{
		return front == back;
	}

	/// dereferences the iterator
	reference operator*() const
		// we're literally just defererencing a pointer
		// noexcept(noexcept(*std::declval<pointer>()))
	{
		// check: front <= current < back
		return *current;
	}

	/// pointer member access
	pointer operator->() const
		// noexcept(std::is_nothrow_copy_constructible<pointer>::value)
	{
		return current;
	}

	/// prefix increment, wraps around from end to begin if reached
	radix_iterator& operator++()
	{
		if(this->range_empty()) {
			return *this;
		}

		++current;
		if(current == back) {
			current = front;
		}
		return *this;
	}

	/// postfix increment
	radix_iterator operator++(int)
	{
		auto cpy = *this;
		++*this;
		return cpy;
	}

	/// prefix decrement, wraps from begin to end if passed
	radix_iterator& operator--()
	{
		if(this->range_empty()) {
			return *this;
		}

		if(current == front) {
			current = back;
		}
		--current;
		return *this;
	}

	/// postfix decrement
	radix_iterator operator--(int)
	{
		auto cpy = *this;
		--*this;
		return cpy;
	}

	pointer begin() const
	{
		return front;
	}

	pointer end() const
	{
		return back;
	}

	/// get address of underlying value currently pointed to
	pointer get() const
		// noexcept(std::is_nothrow_copy_constructible<pointer>::value)
	{
		return current;
	}

	// no need to check underlying range
	// as dictated by the standard (for BidiIterator)

	template <typename L, typename R>
	friend constexpr
	bool operator==(const radix_iterator<L>& lhs, const radix_iterator<R>& rhs)
		noexcept(noexcept(lhs.current == rhs.current))
	{
		// ensure: lhs.front == rhs.front, lhs.back == rhs.back
		return lhs.current == rhs.current;
	}

	template <typename L, typename R>
	friend constexpr
	bool operator!=(const radix_iterator<L>& lhs, const radix_iterator<R>& rhs)
		noexcept(noexcept(lhs.current == rhs.current))
	{
		return !(lhs == rhs);
	}

	// convert
	// used to convert iterator to const_iterator
	template <typename P2>
	operator radix_iterator<P2>() const
	{
		// will err if not convertible
		return { static_cast<P2>(front),
			 static_cast<P2>(back),
			 static_cast<P2>(current) };
	}

}; // }}}
