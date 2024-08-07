#include <cxxtest/TestSuite.h>

#include "common/array.h"
#include "common/noncopyable.h"
#include "common/str.h"

class ArrayTestSuite : public CxxTest::TestSuite
{
	public:
	void test_empty_clear() {
		Common::Array<int> array;
		TS_ASSERT(array.empty());
		array.push_back(17);
		array.push_back(33);
		TS_ASSERT(!array.empty());
		array.clear();
		TS_ASSERT(array.empty());
	}

	void test_iterator() {
		Common::Array<int> array;
		Common::Array<int>::iterator iter;

		// Fill the array with some random data
		array.push_back(17);
		array.push_back(33);
		array.push_back(-11);

		// Iterate over the array and verify that we encounter the elements in
		// the order we expect them to be.

		iter = array.begin();

		TS_ASSERT_EQUALS(*iter, 17);
		++iter;
		TS_ASSERT_DIFFERS(iter, array.end());

		TS_ASSERT_EQUALS(*iter, 33);
		++iter;
		TS_ASSERT_DIFFERS(iter, array.end());

		// Also test the postinc
		TS_ASSERT_EQUALS(*iter, -11);
		iter++;
		TS_ASSERT_EQUALS(iter, array.end());
	}

	void test_erase_iterator() {
		Common::Array<int> array;
		Common::Array<int>::iterator iter;

		// Fill the array with some random data
		array.push_back(17);
		array.push_back(33);
		array.push_back(-11);

		iter = array.begin();
		++iter;

		iter = array.erase(iter);
		TS_ASSERT_DIFFERS(iter, array.end());
		TS_ASSERT_EQUALS(*iter, -11);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)2);
		TS_ASSERT_EQUALS(array[0], 17);
		TS_ASSERT_EQUALS(array[1], -11);
	}

	void test_erase_iterator_range() {
		Common::Array<int> array;
		Common::Array<int>::iterator first, last;

		// Fill the array with some random data
		array.push_back(17);
		array.push_back(33);
		array.push_back(66);
		array.push_back(99);
		array.push_back(-11);

		first = array.begin();
		++first;

		last = array.end();
		--last;

		first = array.erase(first, last);
		TS_ASSERT_DIFFERS(first, array.end());
		TS_ASSERT_EQUALS(*first, -11);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)2);
		TS_ASSERT_EQUALS(array[0], 17);
		TS_ASSERT_EQUALS(array[1], -11);
	}

	void test_insert_iterator() {
		Common::Array<int> array;
		Common::Array<int>::iterator iter;

		// Fill the array with some random data
		array.push_back(17);
		array.push_back(33);
		array.push_back(-11);

		iter = array.begin();
		++iter;

		array.insert(iter, 99);

		TS_ASSERT_EQUALS(*iter, 99);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)4);
		TS_ASSERT_EQUALS(array[0], 17);
		TS_ASSERT_EQUALS(array[1], 99);
		TS_ASSERT_EQUALS(array[2], 33);
		TS_ASSERT_EQUALS(array[3], -11);
	}

	void test_direct_access() {
		Common::Array<int> array;

		// Fill the array with some random data
		array.push_back(17);
		array.push_back(33);
		array.push_back(-11);

		TS_ASSERT_EQUALS(array[0], 17);
		TS_ASSERT_EQUALS(array[1], 33);
		TS_ASSERT_EQUALS(array[2], -11);
	}

	void test_insert_at() {
		Common::Array<int> array;

		// First of all some data
		array.push_back(-12);
		array.push_back(17);
		array.push_back(25);
		array.push_back(-11);

		// Insert some data
		array.insert_at(2, 33);

		TS_ASSERT_EQUALS(array[0], -12);
		TS_ASSERT_EQUALS(array[1], 17);
		TS_ASSERT_EQUALS(array[2], 33);
		TS_ASSERT_EQUALS(array[3], 25);
		TS_ASSERT_EQUALS(array[4], -11);

		TS_ASSERT_EQUALS(array.size(), (unsigned int)5);
	}

	void test_insert_at_array() {
		Common::Array<int> array;
		Common::Array<int> array2;

		// First of all some data
		array.push_back(-12);
		array.push_back(17);
		array.push_back(25);
		array.push_back(-11);

		array2.push_back(42);
		array2.push_back(105);
		array2.push_back(-1);

		// Insert some data
		array.insert_at(2, array2);

		TS_ASSERT_EQUALS(array.size(), (unsigned int)7);

		TS_ASSERT_EQUALS(array[0], -12);
		TS_ASSERT_EQUALS(array[1], 17);
		TS_ASSERT_EQUALS(array[2], 42);
		TS_ASSERT_EQUALS(array[3], 105);
		TS_ASSERT_EQUALS(array[4], -1);
		TS_ASSERT_EQUALS(array[5], 25);
		TS_ASSERT_EQUALS(array[6], -11);

	}

	void test_self_insert() {
		Common::Array<int> array;
		int i;

		// Insert some data -- and make sure we have enough space for
		// *twice* as much data. This way, there is no need to allocate
		// new storage, so if the insert() operation is "clever", it
		// will try to reuse the existing storage.
		// This in turn may uncover bugs if the insertion code does not
		// expect self-insertions.
		array.reserve(128);
		for (i = 0; i < 64; ++i)
			array.push_back(i);

		// Now insert the array into the middle of itself
		array.insert_at(12, array);

		// Verify integrity
		TS_ASSERT_EQUALS(array.size(), 128UL);

		for (i = 0; i < 12; ++i)
			TS_ASSERT_EQUALS(array[i], i);
		for (i = 0; i < 64; ++i)
			TS_ASSERT_EQUALS(array[i+12], i);
		for (i = 12; i < 64; ++i)
			TS_ASSERT_EQUALS(array[i+64], i);
	}


	void test_remove_at() {
		Common::Array<int> array;

		// First of all some data
		array.push_back(-12);
		array.push_back(17);
		array.push_back(33);
		array.push_back(25);
		array.push_back(-11);

		// Remove some data
		array.remove_at(1);

		TS_ASSERT_EQUALS(array[0], -12);
		TS_ASSERT_EQUALS(array[1], 33);
		TS_ASSERT_EQUALS(array[2], 25);
		TS_ASSERT_EQUALS(array[3], -11);

		TS_ASSERT_EQUALS(array.size(), (unsigned int)4);
	}

	void test_push_back() {
		Common::Array<int> array1, array2;

		// Some data for both
		array1.push_back(-3);
		array1.push_back(5);
		array1.push_back(9);

		array2.push_back(3);
		array2.push_back(-2);
		array2.push_back(-131);

		array1.push_back(array2);

		TS_ASSERT_EQUALS(array1[0], -3);
		TS_ASSERT_EQUALS(array1[1], 5);
		TS_ASSERT_EQUALS(array1[2], 9);
		TS_ASSERT_EQUALS(array1[3], 3);
		TS_ASSERT_EQUALS(array1[4], -2);
		TS_ASSERT_EQUALS(array1[5], -131);

		TS_ASSERT_EQUALS(array1.size(), (unsigned int)6);
		TS_ASSERT_EQUALS(array2.size(), (unsigned int)3);
	}

	struct SafeInt {
		int val;
		SafeInt() : val(0) {}
		SafeInt(int v) : val(v) {}
		~SafeInt() { val = -1; }
		bool operator==(int v) {
			return val == v;
		}
	};

	void test_push_back_ex() {
		// This test makes sure that inserting an element invalidates
		// references/iterators/pointers to elements in the array itself
		// only *after* their value has been copied.
		Common::Array<SafeInt> array;

		array.push_back(42);
		for (int i = 0; i < 40; ++i) {
			array.push_back(array[0]);
			TS_ASSERT_EQUALS(array[i], 42);
		}
	}

	void test_copy_constructor() {
		Common::Array<int> array1;

		// Some data for both
		array1.push_back(-3);
		array1.push_back(5);
		array1.push_back(9);

		Common::Array<int> array2(array1);

		// Alter the original array
		array1[0] = 7;
		array1[1] = -5;
		array1[2] = 2;

		TS_ASSERT_EQUALS(array2[0], -3);
		TS_ASSERT_EQUALS(array2[1], 5);
		TS_ASSERT_EQUALS(array2[2], 9);

		TS_ASSERT_EQUALS(array2.size(), (unsigned int)3);
	}

	void test_equals() {
		Common::Array<int> array1;

		// Some data for both
		array1.push_back(-3);
		array1.push_back(5);
		array1.push_back(9);

		Common::Array<int> array2(array1);

		TS_ASSERT(array1 == array2);
		array1.push_back(42);
		TS_ASSERT(array1 != array2);
		array2.push_back(42);
		TS_ASSERT(array1 == array2);
	}

	void test_array_constructor() {
		const int array1[] = { -3, 5, 9 };

		Common::Array<int> array2(array1, 3);

		TS_ASSERT_EQUALS(array2[0], -3);
		TS_ASSERT_EQUALS(array2[1], 5);
		TS_ASSERT_EQUALS(array2[2], 9);

		TS_ASSERT_EQUALS(array2.size(), (unsigned int)3);
	}

	class Copyable {
		bool _copied;
		int _value;
		Copyable &operator=(Copyable &);
	public:
		Copyable() : _copied(false), _value(1) {}
		explicit Copyable(const int v) : _copied(false), _value(v) {}
		Copyable(const Copyable &other) : _copied(true), _value(other._value) {}
		bool copied() const { return _copied; }
		int value() const { return _value; }
	};

	void test_array_constructor_count() {
		Common::Array<int> array(10);
		TS_ASSERT_EQUALS(array.size(), 10U);
		TS_ASSERT_EQUALS(array[0], 0);
		TS_ASSERT_EQUALS(array[9], 0);

		// This will fail at compile time if it is not possible to construct an
		// array without copy-construction
		Common::Array<Common::NonCopyable> nonCopyable(1);
	}

	void test_array_constructor_list() {
		Common::Array<int> array = {1, 42, 255};
		TS_ASSERT_EQUALS(array.size(), 3U);
		TS_ASSERT_EQUALS(array[0], 1);
		TS_ASSERT_EQUALS(array[1], 42);
		TS_ASSERT_EQUALS(array[2], 255);
	}

	void test_array_constructor_count_copy_value() {
		Common::Array<int> trivial(5, 1);
		TS_ASSERT_EQUALS(trivial.size(), 5U);
		TS_ASSERT_EQUALS(trivial[0], 1);
		TS_ASSERT_EQUALS(trivial[4], 1);

		Copyable c(123);
		typedef Common::Array<Copyable> NonTrivialArray;

		NonTrivialArray nonTrivialCopy(3, c);
		TS_ASSERT_EQUALS(nonTrivialCopy.size(), 3U);
		for (NonTrivialArray::size_type i = 0; i < nonTrivialCopy.size(); ++i) {
			TS_ASSERT_EQUALS(nonTrivialCopy[0].value(), 123);
			TS_ASSERT(nonTrivialCopy[0].copied());
		}

		NonTrivialArray nonTrivialDefault(3);
		TS_ASSERT_EQUALS(nonTrivialDefault.size(), 3U);
		for (NonTrivialArray::size_type i = 0; i < nonTrivialDefault.size(); ++i) {
			TS_ASSERT_EQUALS(nonTrivialDefault[0].value(), 1);
			TS_ASSERT(!nonTrivialDefault[0].copied());
		}
	}

	void test_array_constructor_str() {
		const char *array1[] = { "a", "b", "c" };

		Common::Array<Common::String> array2(array1, 3);

		TS_ASSERT_EQUALS(array2[0], "a");
		TS_ASSERT_EQUALS(array2[1], "b");
		TS_ASSERT_EQUALS(array2[2], "c");

		TS_ASSERT_EQUALS(array2.size(), (unsigned int)3);
	}

	void test_data() {
		Common::Array<int> array;
		TS_ASSERT(array.data() == nullptr);
		array.resize(2);
		TS_ASSERT(array.data() != nullptr);
		TS_ASSERT_EQUALS(array.data(), &array.front());
		TS_ASSERT_EQUALS(array.data() + array.size() - 1, &array.back());
	}

	void test_front_back_push_pop() {
		Common::Array<int> container;

		container.push_back( 42);
		container.push_back(-23);

		TS_ASSERT_EQUALS(container.front(), 42);
		TS_ASSERT_EQUALS(container.back(), -23);

		container.front() = -17;
		container.back() = 163;
		TS_ASSERT_EQUALS(container.front(), -17);
		TS_ASSERT_EQUALS(container.back(),  163);

		container.pop_back();
		TS_ASSERT_EQUALS(container.front(), -17);
		TS_ASSERT_EQUALS(container.back(),  -17);
	}

	void test_resize() {
		Common::Array<int> array;

		array.resize(3);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)3);

		array[0] = -3;
		array[1] = 163;
		array[2] = 17;

		array.resize(100);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)100);
		TS_ASSERT_EQUALS(array[0], -3);
		TS_ASSERT_EQUALS(array[1], 163);
		TS_ASSERT_EQUALS(array[2], 17);

		TS_ASSERT_EQUALS(array[99], 0);

		array.resize(2);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)2);
		TS_ASSERT_EQUALS(array[0], -3);
		TS_ASSERT_EQUALS(array[1], 163);

		array.resize(4, 42);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)4);
		TS_ASSERT_EQUALS(array[0], -3);
		TS_ASSERT_EQUALS(array[1], 163);
		TS_ASSERT_EQUALS(array[2], 42);
		TS_ASSERT_EQUALS(array[3], 42);

		array.resize(2, 42);
		TS_ASSERT_EQUALS(array.size(), (unsigned int)2);
		TS_ASSERT_EQUALS(array[0], -3);
		TS_ASSERT_EQUALS(array[1], 163);
	}

	void test_swap() {
		Common::Array<int> array1, array2;

		array1.push_back(-3);
		array1.push_back(163);
		array1.push_back(17);
		array2.push_back(5);
		array2.push_back(9);

		TS_ASSERT_EQUALS(array1.size(), 3u);
		TS_ASSERT_EQUALS(array1[0], -3);
		TS_ASSERT_EQUALS(array1[1], 163);
		TS_ASSERT_EQUALS(array1[2], 17);
		TS_ASSERT_EQUALS(array2.size(), 2u);
		TS_ASSERT_EQUALS(array2[0], 5);
		TS_ASSERT_EQUALS(array2[1], 9);

		array1.swap(array2);

		TS_ASSERT_EQUALS(array1.size(), 2u);
		TS_ASSERT_EQUALS(array1[0], 5);
		TS_ASSERT_EQUALS(array1[1], 9);
		TS_ASSERT_EQUALS(array2.size(), 3u);
		TS_ASSERT_EQUALS(array2[0], -3);
		TS_ASSERT_EQUALS(array2[1], 163);
		TS_ASSERT_EQUALS(array2[2], 17);
	}

};

struct ListElement {
	int value;
	int tag;

	ListElement(int v, int t = 0) : value(v), tag(t) {}
};

static int compareInts(const void *a, const void *b) {
	return ((const ListElement *)a)->value - ((const ListElement *)b)->value;
}

class SortedArrayTestSuite : public CxxTest::TestSuite {
public:
	void test_insert() {
		Common::SortedArray<ListElement *> container(compareInts);
		Common::SortedArray<ListElement *>::iterator iter;

		// Fill the container with some random data
		container.insert(new ListElement(1));
		container.insert(new ListElement(7));
		container.insert(new ListElement(8));
		container.insert(new ListElement(3));
		container.insert(new ListElement(5));
		container.insert(new ListElement(4));
		container.insert(new ListElement(9));
		container.insert(new ListElement(2));
		container.insert(new ListElement(6));

		// Verify contents are correct
		iter = container.begin();

		for (int i = 1; i < 10; i++) {
			TS_ASSERT_EQUALS((*iter)->value, i);
			++iter;
		}

		TS_ASSERT_EQUALS(iter, container.end());

		// Cleanup
		for(iter = container.begin(); iter != container.end(); iter++) {
			delete *iter;
		}
	}

	void test_stability() {
		Common::SortedArray<ListElement *> container(compareInts);
		Common::SortedArray<ListElement *>::iterator iter;

		// Check stability, using duplicate keys and sequential tags.
		container.insert(new ListElement(1, 3));
		container.insert(new ListElement(0, 1));
		container.insert(new ListElement(4, 8));
		container.insert(new ListElement(1, 4));
		container.insert(new ListElement(0, 2));
		container.insert(new ListElement(2, 6));
		container.insert(new ListElement(1, 5));
		container.insert(new ListElement(3, 7));
		container.insert(new ListElement(4, 9));

		// Verify contents are correct
		iter = container.begin();

		for (int i = 1; i < 10; i++) {
			TS_ASSERT_EQUALS((*iter)->tag, i);
			++iter;
		}

		TS_ASSERT_EQUALS(iter, container.end());

		// Cleanup
		for(iter = container.begin(); iter != container.end(); iter++) {
			delete *iter;
		}
	}

};
