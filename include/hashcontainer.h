#pragma once

#include <algorithm>
#include <memory>

//! @short The HashContainer template defines a fixed size container to store hashes.
//! This class acts as a replacement for unordered containers provided by the STL.
//! It contains several optimizations regarding container size and insertion time.
//! Use this container instead of the STL when you:
//! * Only need to store hashes.
//! * Determine a maximum number of entries.
//! * Know the approximate number of entries and its value is almost the maximum.
//! * Don't need the full interface provided by the STL.
//! * Can enumerate hashes from 0 to container size - 1.
//! The last point is important because this number is internally used as an address. With this
//! number the HashContainer can behave as an unordered_map with a value type of an unsigned int.
template<typename sizeType_t, typename hashType_t>
class GenericHashContainer
{
public:
	using sizeType = sizeType_t;
	using hashType = hashType_t;
	using sizeLimits = std::numeric_limits<sizeType>;
	using hashLimits = std::numeric_limits<hashType>;

	//! @short The Bucket class is used as an index to access all Nodes that share a part of their hash.
	struct Bucket
	{
		sizeType first;
	};

	//! @short All entries are stored inside Nodes. Therefore the number of Nodes define the container size.
	struct Node
	{
		hashType hash;
		sizeType next;
	};

	//! @short Construct a HashContainer with a fixed size.
	//! @param entries : Maximum number of entries the HashContainer can hold.
	explicit GenericHashContainer(size_t entries);

	//! @short Construct a copy of HashContainer instance.
	//! @param other : The container to copy.
	GenericHashContainer(const GenericHashContainer &other);

	//! @short Construct a HashContainer invalidating the other instance.
	//! @param other : The container to move from.
	GenericHashContainer(GenericHashContainer &&other);

	//! @short Assigns this instance with another HashContainer.
	//! @param other : The container to copy.
	GenericHashContainer& operator=(GenericHashContainer other);

	//! @short Moves another HashContainer to this instance.
	//! @param other : The container to move from.
	GenericHashContainer& operator=(GenericHashContainer &&other);

	//! @short Swaps this instance with another.
	//! @param other : The container to swap.
	void swap(GenericHashContainer &other);

protected:
	class AbstractIterator
	{
	public:
		AbstractIterator(const GenericHashContainer &ptr, sizeType pos) : m_container(ptr), m_position(pos) {}

		//! @short Accessor for the value this Iterator points to.
		sizeType operator*() const { return m_position; }

		//! @short Operator to check validness of the Iterator.
		//! @return __True__ when Iterator is valid.
		//! @return __False__ when Iterator is invalid.
		operator bool() const { return m_position != sizeLimits::max(); }

		bool operator==(const AbstractIterator &other)
		{
			return m_position == other.m_position;
		}

		bool operator!=(const AbstractIterator &other)
		{
			return !operator==(other);
		}

	protected:
		const GenericHashContainer<sizeType, hashType> &m_container;
		sizeType m_position;
	};

public:

	//! @short This Iterator class is used to access All Element with the same hash.
	//! This Iterator iterates therefore only over Nodes that are in the same Bucket.
	class SearchIterator : public AbstractIterator
	{
	public:
		//! @short Construct an Iterator. Only used inside of the HashContainer.
		//! @param ptr : The HashContainer this iterator points to.
		//! @param pos : The position of the current Node the Iterator is pointing at.
		SearchIterator(const GenericHashContainer &ptr, sizeType pos) : AbstractIterator(ptr, pos) {}

		//! @short Pre-increment to access the next value with the same hash as the current.
		SearchIterator& operator++()
		{
			AbstractIterator::m_position = AbstractIterator::m_container.findNext(AbstractIterator::m_position);
			return *this;
		}
	};

	//! @short Iterator that is used to access every entry in an order of the associated hash.
	class Iterator : public AbstractIterator
	{
	public:
		Iterator(const GenericHashContainer &ptr, sizeType pos, sizeType bucket) : AbstractIterator(ptr, pos), m_bucket(bucket) {}

		Iterator& operator++()
		{
			AbstractIterator::m_position = AbstractIterator::m_container.nextElement(AbstractIterator::m_position, m_bucket);
			return *this;
		}

	protected:
		sizeType m_bucket;
	};

	//! @short LocalIterator that is used to access every entry in an order of the associated hash. It can only iterate over one bucket.
	class LocalIterator : public Iterator
	{
	public:
		LocalIterator(const GenericHashContainer &ptr, sizeType pos, sizeType bucket) : Iterator(ptr, pos, bucket) {}

		LocalIterator& operator++()
		{
			sizeType currentBucket = Iterator::m_bucket;
			Iterator::operator++();
			// We make this iterator invalid when we reach the next bucket.
			if (Iterator::m_bucket != currentBucket)
			{
				AbstractIterator::m_position = sizeLimits::max();
			}
			return *this;
		}
	};

	//! @short Inserts a hash value pair into this container. This might invalidate every Iterator.
	//! @param hash : The hash to insert into this container. Not necessary unique.
	//! @param value : The value associated with the hash. Must be unique for every entry and smaller than the container size.
	//! Calling insert with a value already in use will invalidate the container.
	void insert(size_t hash, sizeType value) const;

	//! @short Removes a hash value pair from this container. This might invalidate every Iterator.
	//! When the hash value pair can not be found nothing will happen.
	//! @param hash : The hash to insert into this container.
	//! @param value : The value associated with the hash. 
	void remove(size_t hash, sizeType value) const;

	//! @short Removes the content but does not change its size.
	void clear() const;

	//! @short Searches for a specific hash and returns an Iterator.
	//! @return __valid Iterator__ when the hash is found.
	//! @return __invalid Iterator__ when the hash wasn't found.
	SearchIterator find(size_t hash) const;

	//! @short Returns a (global) Iterator that can be used to iterate
	//! over all nodes in an order defined by the associated hash.
	Iterator begin() const;

	//! @short Returns an iterator to the end (i.e. the element after the last element) of the given HashContainer.
	//! @remark Do not dereference this Iterator.
	Iterator end() const;

	//! @short Returns a (global) Iterator that can be used to iterate
	//! over all nodes of a bucket in an order defined by the associated hash.
	//! @param index : The bucket the iterator should point at.
	LocalIterator localBegin(sizeType index) const;

	//! @short Returns an iterator to the end (i.e. the element after the last element) of the given HashContainer.
	//! @remark Do not dereference this Iterator.
	LocalIterator localEnd() const;

	//! @short Constructs a node with the given parameter but does not insert it into the bucket structure.
	//! @remark This function is intended to be used with insertEmplaced and findEmplaced but does not interact with find.
	//! @param hash The hash to emplace.
	//! @param value The position the hash should be stored inside the nodeList.
	void emplace(size_t hash, sizeType value) const;

	//! @short Inserts an already emplaced node into the bucket structure.
	//! @param pos The position inside the nodeList the element to insert is located.
	void insertEmplaced(sizeType pos) const;

	//! @short Searches for a node that has the same hash than an already emplaced node.
	//! @param pos The position inside the nodeList the emplaced hash can be found.
	//! @remark This function is only useful when a node was emplaced before at position pos.
	SearchIterator findEmplaced(sizeType pos) const;

	//! @short Returns the number of nodes of this instance.
	sizeType nodes() const;

	//! @short Returns the number of buckets of this instance.
	sizeType buckets() const;

	//! @short Returns the internal hash of an entry.
	hashType hash(sizeType index);

protected:

	//! @short Internal find used by public find functions.
	SearchIterator find(hashType hash, sizeType pos) const;

	//! @short Internal find used by Iterator.
	sizeType findNext(sizeType current) const;

	//! @short Internal find to retrieve the next hash.
	sizeType findNext(hashType hash, sizeType current) const;

	//! @short Internal function to access the next Element.
	sizeType nextElement(sizeType current, sizeType &bucket) const;

	//! @short Returns the highest part of hash that fits into hashType.
	static hashType high(size_t hash);

	//! @short Returns the lowest part of hash that fits into sizeType
	static sizeType low(size_t hash);

	static sizeType computeBucketCount(size_t entries);

	template<class T>
	std::unique_ptr<T[]> copyArray(const std::unique_ptr<T[]> &reference, sizeType size);

	sizeType m_bucketCount;
	sizeType m_nodeCount;

	std::unique_ptr<Bucket[]> m_bucketList;
	std::unique_ptr<Node[]> m_nodeList;

	static_assert(sizeof(size_t) == 8, "Hash data type must be 64 bit.");
	static_assert(sizeof(sizeType) <= sizeof(size_t), "sizeType must not be larger than size_t.");
	static_assert(sizeof(hashType) < sizeof(size_t), "hashType must not be larger than size_t.");
	static_assert(std::is_unsigned<sizeType>::value, "sizeType must be an unsigned integral.");
	static_assert(std::is_unsigned<hashType>::value, "hashType must be an unsigned integral.");
};

using HashContainer = GenericHashContainer<uint32_t, uint32_t>;
using SparseHashContainer = GenericHashContainer<uint32_t, uint16_t>;

#include "hashcontainer.hpp"
