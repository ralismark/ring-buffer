# ring-buffer

An allocator-aware STL-style circular buffer. WIP

## Description

This library contains 2 parts: the ring buffer, and its iterator.

The iterator, called `radix_iterator`, takes a range along with a pointer. When
traversing past either end of the range, the underlying pointer wraps around to
the other end.

The ring buffer (also called a circular buffer), called `ring_buffer`, stores a
series of values in a block of memory. The initialised values may begin
anywhere, so operations on either end (e.g. pushing or popping) do not
invalidate all iterators unless the capacity changes.

The ring buffer supports common container operations (e.g. emplace_front/back),
as well as a user-provided allocator.

## Install

This is a header-only library. To install, copy the files in include to your
include directory. No fancy building needs to be done.

## Usage

Include `<ring_buffer.hpp>` in your project, and use the ring_buffer class.

_// TODO: Further documentation and examples_

## Tests

_Currently, only the radix iterator is completely tested. Tests for the ring
buffer are currently being added._

Tests are run with the LLVM integration tester (lit), which can be obtained
through `pip`. As a result, python 2 must be installed.

Tests are located in `/tests`, and have either `.pass.cpp` or `.fail.cpp`
extensions. Pass tests must compile without warnings (even with `-Weverything
-Wno-c++98-compat`), and return 0 when run.  Fail tests must produce errors when
compiled. Other files are not test files.

To run tests, run `lit -v .` while in `/tests`.

## Implementation Notes

These are the notes on the implementation of the classes, which may be
interesting to some.

### Radix Iterator

Note that the iterator is not a random access iterator, but a bidirectional
iterator. Due to the wrapping nature of the iterator, it is impossible to
determine whether another iterator is ahead or behind it without requiring
additional markers. This may be added in the future, but for now, it does not
check to see if it is valid.

The wrapping nature of the iterator also allows it to be unending, if that is
ever wanted. Once it gets to an end of the range, it wraps around to the other
end, allowing infinite traversal.

This iterator takes the pointer type instead of the value type. In the ring
buffer, this is used to support custom pointer types with allocator. By itself,
this makes it also usable as a iterator adaptor.

Also, the iterator is able to handle an 'empty' range, where there are no
elements (because the beginning and the end are the same). In this case,
movement operations have no effect. Other operations are not affected.

### Ring Buffer

The capacity of memory block is always one less than the number of elements
which can fit. This is required to be able to distinguish the difference between
an empty container (where begin and end are the same), and a full one (where
they are also the same, but the end is on the next 'wrap').
