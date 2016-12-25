#pragma once

// TODO(timmy): reduce header dependencies

#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept> // only for out_of_range (used once, but required)
#include <type_traits>
#include <utility>

#include "radix_iterator.hpp"

template <typename T, typename Allocator = std::allocator<T>>
class ring_buffer
{
private: // internal statics

	// constepr bool wrapper
	template <bool val>
	using cte_bool = integral_constant<bool, val>;

	using atraits = typename std::allocator_traits<Allocator>;

	// ensure correct allocator
	static_assert(std::is_same<T, typename atraits::value_type>::value,
	              "Allocator must use the same type as T");

	// allocator weirdness
	using pocca = cte_bool<atraits::propagate_on_container_copy_assignment::value>;
	using pocma = cte_bool<atraits::propagate_on_container_move_assignment::value>;
	using pocs  = cte_bool<atraits::propagate_on_container_swap::value>;

public: // statics

	// {{{ member types

	using allocator_type         = typename atraits::allocator_type;
	using value_type             = T;

	using size_type              = typename atraits::size_type;
	using difference_type        = typename atraits::difference_type;

	using reference              = value_type&;
	using const_reference        = const value_type&;

	using pointer                = typename atraits::pointer;
	using const_pointer          = typename atraits::const_pointer;

	using iterator               = radix_iterator<pointer>;
	using reverse_iterator       = std::reverse_iterator<iterator>;

	using const_iterator         = radix_iterator<const_pointer>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	// }}}

private: // internal statics

	// helper for insertion overloads (eg.g. assign)
	// to distinguish between iterator pairs and a value + count pair
	template <typename InputIt>
	using int_if_input_it = typename std::enable_if<
			std::is_base_of<
				std::input_iterator_tag,
				typename std::iterator_traits<InputIt>::iterator_category
			>::value,
		int>::type;

	// expansion rate
	// 1.5 is more optimal than 2,
	// best would be psi (golden ratio) ~= 1.68, but 1.5 is close enough
	static constexpr double expansion_ratio = 1.5;

	// positive wrap a value
	// wraps val to [0, wrap)
	static size_type pwrap(difference_type val, size_type wrap)
	{
		return ((val % wrap) + wrap) % wrap;
	}

	// defined at bottom, resolves dependency on interface
	static void destruct_memblk(ring_buffer* self, pointer ptr);

	struct mb_dtor
	{
		using pointer = ring_buffer::pointer;

		ring_buffer* self;

		// destroy all and free
		void operator()(pointer ptr)
		{
			destruct_memblk(self, ptr);
		}
	};

	// small implementation
	// removes <functional> dependency for reference_wrapper in insert
	struct cref_wrapper
	{
		const value_type* ptr;
		operator const value_type&() const
		{
			return *ptr;
		}
	};

	// clearer types
	using abs_offset = size_type;
	using idx_offset = size_type;
	using abs_offset_rel = difference_type;
	using idx_offset_rel = difference_type;

private: // variables

	// note: we always need one blank element. otherwise, we can't
	//       distinguish between a blank ring and one with all values
	//       allocated

	allocator_type mm; // `memory manager'
	// raw memblks
	size_type mb_size;
	std::unique_ptr<T[], mb_dtor> memblk;

	// begin and end idx of values
	size_type m_begin;
	size_type m_end;

private: // internal methods

	// {{{ internal methods

	// create a mb_dtor object
	constexpr
	mb_dtor make_mb_dtor()
		noexcept
	{
		return {this};
	}

	// destruct all objects
	void dtor_value(abs_offset idx)
	{
		atraits::destroy(mm, &memblk[this->abs_offset_of(idx)]);
	}

	void dtor_value(abs_offset begin, abs_offset end)
	{
		for(auto idx = begin; idx != end; idx = (idx + 1) % mb_size) {
			this->dtor_value(idx);
		}
	}

	void dtor_value_all()
	{
		this->dtor_value(m_begin, m_end);
		m_begin = m_end = 0;
	}

	// allocate a memory block
	std::unique_ptr<T[], mb_dtor> alloc_memblk(size_type size)
	{
		return { atraits::allocate(mm, size), this->make_mb_dtor() };
	}

	// construct element
	template <typename... Args>
	void ctor_value(abs_offset idx, Args&&... args)
	{
		atraits::construct(mm, memblk.get() + idx, std::forward<Args>(args)...);
	}

	// tag dispatch
	// because allocators may be weird
	// and we need different code because of this
	void move_assign_mm(ring_buffer& other, cte_bool<true> /* pocma */)
	{
		mm = std::move(other.mm);
	}

	void move_assign_mm(ring_buffer& other, cte_bool<false> /* pocma */)
	{
		// do nothing
	}

	void move_assign(ring_buffer&& other, cte_bool<true> /* pocma */)
	{
		// 1. deallocate
		memblk.reset();

		// 2. move allocator
		this->move_assign_mm(other, pocma());

		// 3. transfer ownership
		memblk = std::move(other.memblk);
		memblk.get_deleter() = this->make_mb_dtor();
		mb_size = other.mb_size;

		// other members
		m_begin = other.m_begin;
		m_end = other.m_end;
	}

	void move_assign(ring_buffer&& other, cte_bool<false> /* pocma */)
	{
		if(mm == other.mm) {
			move_assign(other, cte_bool<true>());
		} else {
			this->assign(std::make_move_iterator(other.begin()),
				     std::make_move_iterator(other.end()));
		}
	}

	void copy_assign_mm(const ring_buffer& other, cte_bool<true> /* pocca */)
	{
		mm = other.mm;
	}

	void copy_assign_mm(const ring_buffer& other, cte_bool<false> /* pocca */)
	{
		// do nothing
	}

	void swap_mm(ring_buffer& other, cte_bool<true> /* pocs */)
	{
		using std::swap;
		swap(mm, other.mm);
	}

	void swap_mm(ring_buffer& other, cte_bool<false> /* pocs */)
	{
		// ensure: mm == other.mm
		// do nothing
	}

	void ensure_alloc_blanked(size_type count)
	{
		this->dtor_value_all();
		if(count > this->capacity()) {
			memblk = this->alloc_memblk(count);
			mb_size = count + 1;

			m_begin = 0;
			m_end = count + 1;
		}
	}

	void ensure_alloc_blanked_extra(size_type count)
	{
		size_type new_size = count;
		if(count > this->capacity()) {
			// no rounding needed
			// if size not enough, it is increased to count
			new_size = mb_size * expansion_ratio;
			if(count > new_size) {
				new_size = count;
			}
		}
		this->ensure_alloc_blanked(new_size);
	}

	void ensure_alloc_copy(size_type count)
	{
		if(count > this->capacity()) { // implies count > this->size()
			ring_buffer new_blk(count);
			new_blk.m_begin = 0;
			new_blk.m_end = this->size();

			for(auto own_it = this->begin(), new_it = new_blk.begin();
			    own_it != this->end() && new_it != new_blk.end();
			    ++own_it, ++new_it) {
				// init from old
				new_blk.ctor_value(new_it.get() - new_blk.memblk.get(), std::move_if_noexcept(*own_it));
			}

			this->swap(new_blk);
		}
	}

	void ensure_alloc_copy_extra(size_type count)
	{
		size_type new_size = count;
		if(count > this->capacity()) {
			// see above (ensure_alloc_blanked_extra) for explanation
			// on the lack of rounding
			new_size = mb_size * expansion_ratio;
			if(count > new_size) {
				new_size = count;
			}
		}
		this->ensure_alloc_copy(new_size);
	}

	idx_offset idx_of(abs_offset off) const
	{
		auto idx = off - m_begin;
		return pwrap(idx, mb_size);
	}

	// offset of an element
	abs_offset offset_of(idx_offset_rel idx) const
	{
		return this->abs_offset_of(idx + m_begin);
	}

	// absolute offset, just wrap
	abs_offset abs_offset_of(abs_offset_rel idx) const
	{
		return pwrap(idx, mb_size);
	}

	template <typename U>
	abs_offset it_offset(radix_iterator<U> it) const
	{
		// ensure: 'memblk' contains 'it'
		return this->abs_offset_of(it.get() - memblk.get());
	}

	// false if failed
	bool range_check(idx_offset pos) const
	{
		if(pos >= this->size()) {
			throw std::out_of_range("ring_buffer::range_check: pos >= this->size()");
			return false;
		}
		return true;
	}

	iterator it_of(abs_offset idx)
	{
		return { memblk.get(), memblk.get() + mb_size, memblk.get() + idx };
	}

	const_iterator cit_of(abs_offset idx) const
	{
		return { memblk.get(), memblk.get() + mb_size, memblk.get() + idx };
	}

	// logic for this->resize()
	template <typename... Args>
	void resize_val(size_type count, Args&&... args)
	{
		if(count > this->size()) {
			this->ensure_alloc_copy_extra(count);

			auto it_old_end = this->end();
			m_end = this->offset_of(count); // one past the end

			for(auto it == it_old_end; it != this->end(); ++it) {
				this->ctor_value(this->it_offset(it), std::forward<Args>(args)...); // default init
			}
		} else if(count < this->size()) {
			auto old_end = m_end;
			m_end = this->abs_offset_of(m_begin + count);

			this->dtor_value(m_end, old_end);
		}
	}

	// interface, based on type of iterator
	// use tag dispatch
	template <typename InputIt>
	iterator it_insert(const_iterator pos, InputIt first, InputIt last)
	{
		using itraits = std::iterator_traits<InputIt>;
		constexpr bool is_fwd_it = std::is_base_of<std::forward_iterator_tag, typename itraits::iterator_category>::value;
		return it_insert(pos, first, last, cte_bool<is_fwd_it>());
	}

	// tag dispatch
	template <typename InputIt>
	iterator it_insert(const_iterator pos, InputIt first, InputIt last, cte_bool<true> /* is_fwd_it */)
	{
		return this->it_insert(pos, first, last, std::distance(first, last));
	}

	template <typename InputIt>
	iterator it_insert(const_iterator pos, InputIt first, InputIt last, cte_bool<false> /* is_fwd_it */)
	{
		// safely de-const the pointer
		// cit => abs_offset => idx_offset => it
		iterator insert_pos = std::next(this->begin(), this->idx_of(this->it_offset(pos)));

		for(auto it = first; it != last; ++it) {
			insert_pos = this->it_insert(pos, it, last, 1);
		}
		return insert_pos;
	}

	template <typename InputIt>
	iterator it_insert(const_iterator pos, InputIt first, InputIt last, size_type count)
	{
		// change begin instead of end
		// a bit of an optimisation, reduces the number of moves required
		bool expand_forward = std::distance(this->cbegin(), pos) < std::distance(pos, this->cend());

		auto buf_start = this->idx_of(this->it_offset(pos));
		this->ensure_alloc_copy_extra(this->size() + count);

		if(expand_forward) { // change front
			auto old_begin = this->begin();
			m_begin = this->it_offset(std::prev(this->end(), count));
			for(auto from = old_begin, to = this->begin(); from != pos; ++from, ++to) {
				atraits::construct(mm, to.get(), std::move(*from));
				atraits::destroy(mm, from.get());
			}
		} else { // change back
			auto old_end = this->rbegin();
			auto end_pos = const_reverse_iterator(std::prev(pos));

			m_end = this->it_offset(std::next(this->end(), count));
			auto new_end = this->rbegin();

			for(auto from = old_end, to = new_end; from != end_pos; ++from, ++to) {
				atraits::construct(mm, to.base().get(), std::move(*from));
				atraits::destroy(mm, from.base().get());
			}
		}

		// init values
		size_type num = 0;
		auto first_elem = std::next(this->begin(), buf_start);
		for(auto it = first_elem;
		    first != last && num < count;
		    ++first, ++it, ++num) {
			this->ctor_value(this->it_offset(it), *first);
		}
		return first_elem;
	}

	// }}}

public: // methods

	// TODO(timmy): add noexcept specifications
	// TODO(timmy): add exception guarantees
	// TODO(timmy): add UB checks and reporting

	// {{{ basic functions

	// {{{ ctors

	// blank
	ring_buffer()
		noexcept(noexcept(allocator_type())) // default construct
		: ring_buffer(allocator_type())
	{
	}

	// with alloc
	explicit ring_buffer(const allocator_type& alloc)
		noexcept
		: mm(alloc)
		, mb_size(0), memblk(nullptr)
		, m_begin(0), m_end(0)
	{
	}

	// blank, but with size
	explicit ring_buffer(size_type cap, const allocator_type& alloc = allocator_type())
		: mm(alloc)
		, mb_size(cap + 1), memblk(this->alloc_memblk(mb_size))
		, m_begin(0), m_end(0)
	{
	}

	// certain number of elements
	explicit ring_buffer(size_type count, const value_type& value, const allocator_type& alloc = allocator_type())
		: ring_buffer(alloc)
	{
		// use assign
		this->assign(count, value);
	}

	// from range
	template <typename InputIt, int_if_input_it<InputIt> = 0> // disambiguiation
	ring_buffer(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
		: ring_buffer(alloc)
	{
		// use assign
		this->assign(first, last);
	}

	// initialiser values
	ring_buffer(std::initializer_list<T> il, const allocator_type& alloc = allocator_type())
		: ring_buffer(alloc)
	{
		// use assign
		this->assign(il);
	}

	// copy
	ring_buffer(const ring_buffer& other, const allocator_type& alloc)
		: mm(alloc)
		  // uses new mm to create memblk
		, mb_size(other.size() + 1), memblk(this->alloc_memblk(mb_size))
		, m_begin(0), m_end(other.size())
	{
		// a zip iterator would be nice
		for(auto from = other.begin(), to = this->cbegin();
		    from != other.end() && to != this->cend();
		    ++from, ++to) {
			this->ctor_value(this->it_offset(to), *from);
		}
	}

	// copy
	ring_buffer(const ring_buffer& other)
		: ring_buffer(other,
			      atraits::select_on_container_copy_construction(other.mm))
	{
	}

	// move
	ring_buffer(ring_buffer&& other)
		noexcept(std::is_nothrow_move_constructible<allocator_type>::value)
		: mm(std::move(other.mm))
		, mb_size(other.mb_size), memblk(other.memblk.release(), this->make_mb_dtor())
		, m_begin(other.m_begin), m_end(other.m_end)
	{
	}

	// move
	ring_buffer(ring_buffer&& other, const allocator_type& alloc)
		: mm(alloc)
		, mb_size(other.mb_size), memblk(other.memblk.release(), this->make_mb_dtor())
		, m_begin(other.m_begin), m_end(other.m_end)
	{
	}

	// }}}

	// {{{ assignment

	// copy and swap assignment
	ring_buffer& operator=(const ring_buffer& other)
	{
		this->clear();
		this->copy_assign_mm(other, pocca());
		this->assign(other.begin(), other.end());
		return *this;
	}

	// plain swap assignment
	ring_buffer& operator=(ring_buffer&& other)
		noexcept(pocma::value
			&& std::is_nothrow_move_assignable<allocator_type>::value)
	{
		this->move_assign(std::move(other), pocma());
		return *this;
	}

	ring_buffer& operator=(std::initializer_list<T> il)
	{
		this->assign(il);
		return *this;
	}

	// }}}

	// invalidates: all
	void assign(size_type count, const T& val)
	{
		this->ensure_alloc_blanked(count);

		for(auto i = m_begin; i != m_end; ++i) {
			this->ctor_value(i, val);
		}
	}

	// invalidates: all
	template <typename InputIt, int_if_input_it<InputIt> = 0>
	void assign(InputIt first, InputIt last)
	{
		this->dtor_value_all();
		this->it_insert(this->cbegin(), first, last);
	}

	// invalidates: all
	void assign(std::initializer_list<T> il)
	{
		// il is effectively a range
		this->assign(il.begin(), il.end());
	}

	allocator_type get_allocator() const
		noexcept
	{
		return mm;
	}

	// }}}

	// {{{ element access

	reference at(idx_offset pos)
	{
		this->range_check(pos);
		return (*this)[pos];
	}

	const_reference at(idx_offset pos) const
	{
		this->range_check(pos);
		return (*this)[pos];
	}

	reference operator[](idx_offset pos)
	{
		// ensure: this->range_check(pos)
		return memblk[this->offset_of(pos)];
	}

	const_reference operator[](idx_offset pos) const
	{
		// ensure: this->range_check(pos)
		return memblk[this->offset_of(pos)];
	}

	reference front()
	{
		// ensure: this->size() > 0
		return *this->begin();
	}

	const_reference front() const
	{
		// ensure: this->size() > 0
		return *this->cbegin();
	}

	reference back()
	{
		// ensure: this->size() > 0
		return *this->rbegin();
	}

	const_reference back() const
	{
		// ensure: this->size() > 0
		return *this->crbegin();
	}

	// }}}

	// {{{ iterators

	iterator begin()
	{
		return this->it_of(m_begin);
	}

	const_iterator begin() const
	{
		return this->cbegin();
	}

	const_iterator cbegin() const
	{
		return this->cit_of(m_begin);
	}

	iterator end()
	{
		return this->it_of(m_end);
	}

	const_iterator end() const
	{
		return this->cend();
	}

	const_iterator cend() const
	{
		return this->cit_of(m_end);
	}

	// explicit cast to reverse iterator

	reverse_iterator rbegin()
	{
		return reverse_iterator(this->it_of(m_end - 1));
	}

	const_reverse_iterator rbegin() const
	{
		return this->crbegin();
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(this->cit_of(m_end - 1));
	}

	reverse_iterator rend()
	{
		return reverse_iterator(this->it_of(m_begin - 1));
	}

	const_reverse_iterator rend() const
	{
		return this->crend();
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator(this->cit_of(m_begin - 1));
	}

	// // from a certain index
	// iterator offset(idx_offset idx)
	// {
	// 	return this->it_of(this->offset_of(idx))
	// }

	// const_iterator offset(idx_offset idx) const
	// {
	// 	return this->coffset(idx);
	// }

	// const_iterator coffset(idx_offset idx) const
	// {
	// 	return this->cit_of(this->offset_of(idx))
	// }

	// reverse_iterator roffset(idx_offset idx)
	// {
	// 	return reverse_iterator(this->offset(idx));
	// }

	// const_reverse_iterator roffset(idx_offset idx) const
	// {
	// 	return this->croffset(idx);
	// }

	// const_reverse_iterator croffset(idx_offset idx) const
	// {
	// 	return const_reverse_iterator(this->coffset(idx));
	// }

	// }}}

	// {{{ capacity

	bool empty() const
	{
		return this->size() == 0;
	}

	size_type size() const
	{
		auto diff = m_end - m_begin;
		return diff >= 0 ? diff : mb_size - diff;
	}

	size_type max_size() const
	{
		return atraits::max_size(mm);
	}

	// invalidates: all (if capacity changes)
	//              none (otherwise)
	void reserve(size_type new_cap)
	{
		if(new_cap > this->capacity()) {
			// no extra alloc
			this->ensure_alloc_copy(new_cap);
		}
	}

	size_type capacity() const
	{
		return mb_size - 1;
	}

	// invalidates: all (if capacity changes)
	void shrink_to_fit()
	{
		size_type size = this->size();
		if(size == 0) {
			// reset all
			memblk.reset();
			mb_size = m_begin = m_end = 0;
		} else if(size < this->capacity()) {
			// logic is in copy ctor, but we can use the same allocator
			// reduce code duplication
			auto copy = ring_buffer(*this, mm);
			this->swap(copy);
		}
	}

	// }}}

	// {{{ modifiers

	// invalidates: all
	void clear()
	{
		// reset all
		memblk.reset();
		m_begin = m_end = mb_size = 0;
	}

	// invalidates: all (if capacity changes)
	//              before pos (if pos closer to front)
	//              pos + after pos (if pos closed to end)
	iterator insert(const_iterator pos, const value_type& value)
	{
		// hacky pointer arith
		// valid since end iterator must not be dereferenced
		return this->it_insert(pos, std::addressof(value), std::addressof(value) + 1);
	}

	// invalidates: all (if capacity changes)
	//              before pos (if pos closer to front)
	//              pos + after pos (if pos closed to end)
	iterator insert(const_iterator pos, value_type&& value)
	{
		return this->it_insert(pos,
			// since decltype(&value) == value* (losing rvalue-ness)
			std::make_move_iterator(std::addressof(value)),
			std::make_move_iterator(std::addressof(value) + 1));
	}

	iterator insert(const_iterator pos, size_type count, const value_type& value)
	{
		// TODO(timmy): find a way to get multiple inserts from one value
		// use ring_buffer memory management to create dynamic array of references
		using rbuf_type = ring_buffer<cref_wrapper, typename atraits::rebind_alloc<cref_wrapper>>;
		rbuf_type rbuf(count, cref_wrapper{std::addressof(value)}, )

		static_assert(false, "not implemented");
	}

	// invalidates: all (if capacity changes or side closer to pos != side closer to ret)
	//              before pos (if pos closer to front)
	//              pos + after pos (if pos closed to end)
	template <typename InputIt, int_if_input_it<InputIt> = 0> // diambiguate
	iterator insert(const_iterator pos, InputIt first, InputIt last)
	{
		return this->it_insert(pos, first, last);
	}

	// invalidates: all (if capacity changes or side closer to pos != side closer to next(ret))
	//              before pos (if pos closer to front)
	//              pos + after pos (if pos closed to end)
	iterator insert(const_iterator pos, std::initializer_list<value_type> il)
	{
		return this->it_insert(pos, il.begin(), il.end(), il.size());
	}

	// invalidates: all (if capacity changes)
	//              begin (otherwise)
	reference push_front(const value_type& value)
	{
		return this->emplace_front(value);
	}

	// invalidates: all (if capacity changes)
	//              begin (otherwise)
	reference push_front(value_type&& value)
	{
		return this->emplace_front(value);
	}

	// invalidates: all (if capacity changes)
	//              begin (otherwise)
	template <typename... Args>
	reference emplace_front(Args&&... args)
	{
		this->ensure_alloc_copy_extra(this->size() + 1);

		auto new_begin = this->it_offset(std::prev(this->begin()));
		this->ctor_value(new_begin, std::forward<Args>(args)...);
		m_begin = new_begin;

		return this->front();
	}

	// invalidates: begin
	void pop_front()
	{
		// ensure: this->size() > 0
		auto new_begin = this->it_offset(std::next(this->begin()));
		this->dtor_value(m_begin);
		m_begin = new_begin;
	}

	// invalidates: all (if capacity changes)
	//              end (otherwise)
	reference push_back(const value_type& value)
	{
		return this->emplace_back(value);
	}

	// invalidates: all (if capacity changes)
	//              end (otherwise)
	reference push_back(value_type&& value)
	{
		return this->emplace_back(value);
	}

	// invalidates: all (if capacity changes)
	//              end (otherwise)
	template <typename... Args>
	reference emplace_back(Args&&... args)
	{
		this->ensure_alloc_copy_extra(this->size() + 1);
		this->ctor_value(m_end, std::forward<Args>(args)...);
		m_end = this->it_offset(std::next(this->begin())); // increment

		return this->back(); // new back
	}

	// invalidates: end + one before end
	void pop_back()
	{
		// ensure: this->size() > 0
		auto new_end = this->it_offset(std::prev(this->end()));
		this->dtor_value(new_end);
		m_end = new_end;
	}

	// default init
	// invalidates: all (if capacity changes)
	void resize(size_type count)
	{
		this->resize_val(count);
	}

	// value init
	// invalidates: all (if capacity changes)
	void resize(size_type count, const value_type& value)
	{
		this->resize_val(count, value);
	}

	void swap(ring_buffer& other)
	{
		// for adl
		using std::swap;

		this->swap_mm(other, pocs());
		swap(memblk, other.memblk);
		swap(mb_size, other.mb_size);

		// deleter is bound to ring_buffer; undo swap
		swap(memblk.get_deleter(), other.memblk.get_deleter());
		swap(m_begin, other.m_begin);
		swap(m_end, other.m_end);
	}

	// }}}

	friend void swap(ring_buffer& lhs, ring_buffer& rhs)
	{
		lhs.swap(rhs);
	}

	// TODO(timmy): add member functions

};

template <typename T, typename Allocator>
void ring_buffer<T, Allocator>::destruct_memblk(ring_buffer* self, pointer ptr)
{
	self->dtor_value_all();
	atraits::deallocate(self->mm, ptr, self->mb_size);
}

// for testing compilation
struct O
{
	int x;
	double y;
};

inline
void ct_chk_ri()
{
	using T = ring_buffer<O>::iterator;
	O buf[4];

	T a, b(&buf[0], &buf[3], &buf[0]); // init
	*a = O{1, 1.1};
	a->x = 2;

	++a;
	a++;
	--a;
	a--;

	void(a == b);
	void(a != b);
}

inline
void ct_chk_rb()
{
	using T = ring_buffer<O>;
	O vals[4];

	// {{{ basic fns

	// testing inits
	T a,
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
	a = { O(), O() };

	// assign memfn
	a.assign(5, O());
	a.assign(std::begin(vals), std::end(vals));
	a.assign({O(), O()});

	// get allocator
	void(a.get_allocator());

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
