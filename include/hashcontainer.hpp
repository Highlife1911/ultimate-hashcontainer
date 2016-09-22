
template <typename sizeType, typename hashType>
GenericHashContainer<sizeType, hashType>::GenericHashContainer(size_t entries)
	: m_bucketCount(computeBucketCount(entries))
	, m_nodeCount(static_cast<sizeType>(entries))
	, m_bucketList(std::make_unique<Bucket[]>(m_bucketCount))
	, m_nodeList(std::make_unique<Node[]>(m_nodeCount))
{
	clear();
}

template<typename sizeType, typename hashType>
GenericHashContainer<sizeType, hashType>::GenericHashContainer(const GenericHashContainer &other)
	: m_bucketCount(other.m_bucketCount)
	, m_nodeCount(other.m_nodeCount)
	, m_bucketList(copyArray(other.m_bucketList, m_bucketCount))
	, m_nodeList(copyArray(other.m_nodeList, m_nodeCount))
{
}

template<typename sizeType, typename hashType>
GenericHashContainer<sizeType, hashType>::GenericHashContainer(GenericHashContainer &&other)
	: m_bucketCount(other.m_bucketCount)
	, m_nodeCount(other.m_nodeCount)
	, m_bucketList(std::move(other.m_bucketList))
	, m_nodeList(std::move(other.m_nodeList))
{
}

template<typename sizeType, typename hashType>
GenericHashContainer<sizeType, hashType>& GenericHashContainer<sizeType, hashType>::operator=(GenericHashContainer other)
{
	swap(other);
	return *this;
}

template<typename sizeType, typename hashType>
GenericHashContainer<sizeType, hashType>& GenericHashContainer<sizeType, hashType>::operator=(GenericHashContainer &&other)
{
	swap(other);
	return *this;
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::swap(GenericHashContainer &other)
{
	std::swap(m_bucketCount, other.m_bucketCount);
	std::swap(m_nodeCount, other.m_nodeCount);

	std::swap(m_bucketList, other.m_bucketList);
	std::swap(m_nodeList, other.m_nodeList);
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::insert(size_t hash, sizeType value) const
{
	assert(m_nodeList[value].next == sizeLimits::max());
	assert(m_nodeList[value].hash == hashLimits::max());

	// The low part refers to the bucket and the high part
	// is used to distinct different entries in a single bucket.
	auto bucket = &m_bucketList[low(hash) % m_bucketCount];

	// Let the bucket point to the new inserted element.
	m_nodeList[value].next = bucket->first;
	m_nodeList[value].hash = high(hash);
	bucket->first = value;
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::remove(size_t hash, sizeType value) const
{
	// Do not remove anything when the hashes do not match.
	if (m_nodeList[value].hash != high(hash))
	{
		return;
	}

	// Just remove the entry when it is the first entry.
	sizeType current = m_bucketList[low(hash) % m_bucketCount].first;
	if (current == value)
	{
		m_bucketList[low(hash) % m_bucketCount].first = m_nodeList[value].next;

#ifndef NDEBUG
		// It is necessary to overwrite the memory in debug mode with an
		// invalid value to get the assertion detect invalid operations.
		m_nodeList[value].next = sizeLimits::max();
		m_nodeList[value].hash = hashLimits::max();
#endif

		return;
	}

	// When it is not the first entry we need to find the element
	// that points to the removed element to adjust its next pointer.
	while (current != sizeLimits::max())
	{
		if (m_nodeList[current].next == value)
		{
			m_nodeList[current].next = m_nodeList[value].next;
		}

		current = m_nodeList[current].next;
	}

#ifndef NDEBUG
	// It is necessary to overwrite the memory in debug mode with an
	// invalid value to get the assertion detect invalid operations.
	m_nodeList[value].next = sizeLimits::max();
	m_nodeList[value].hash = hashLimits::max();
#endif
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::clear() const
{
#ifndef NDEBUG
	// We need to initialize the array with an invalid value to detect invalid operations in debug mode.
	// This effectively makes the asserts in insert and remove functional.
	std::memset(m_nodeList.get(), std::numeric_limits<unsigned char>::max(), sizeof(Node) * m_nodeCount);
#endif
	std::memset(m_bucketList.get(), std::numeric_limits<unsigned char>::max(), sizeof(Bucket) * m_bucketCount);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::SearchIterator GenericHashContainer<sizeType, hashType>::find(size_t hash) const
{
	return find(high(hash), low(hash) % m_bucketCount);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::SearchIterator GenericHashContainer<sizeType, hashType>::find(hashType hash, sizeType pos) const
{
	return SearchIterator(*this, findNext(hash, m_bucketList[pos].first));
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::emplace(size_t hash, sizeType value) const
{
	assert(value != sizeLimits::max());
	assert(m_nodeList[value].next == sizeLimits::max());

	// Construct a new node but do not insert it into the bucket structure.
	m_nodeList[value].next = low(hash) % m_bucketCount;
	m_nodeList[value].hash = high(hash);
}

template<typename sizeType, typename hashType>
inline void GenericHashContainer<sizeType, hashType>::insertEmplaced(sizeType value) const
{
	assert(value != sizeLimits::max());
	assert(m_nodeList[value].next != sizeLimits::max());

	// When the element is already emplaced we only need to update the bucket structure.
	auto bucket = &m_bucketList[m_nodeList[value].next];

	m_nodeList[value].next = bucket->first;
	bucket->first = value;
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::SearchIterator GenericHashContainer<sizeType, hashType>::findEmplaced(sizeType pos) const
{
	assert(pos != sizeLimits::max());
	assert(m_nodeList[pos].next != sizeLimits::max());

	return find(m_nodeList[pos].hash, m_nodeList[pos].next);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::Iterator GenericHashContainer<sizeType, hashType>::begin() const
{
	// Find the first bucket that has a valid first pointer.
	sizeType bucket = 0;
	while (m_bucketList[bucket].first == sizeLimits::max())
	{
		++bucket;
		if (bucket == m_bucketCount)
		{
			return end();
		}
	}

	return Iterator(*this, m_bucketList[bucket].first, bucket);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::Iterator GenericHashContainer<sizeType, hashType>::end() const
{
	return Iterator(*this, sizeLimits::max(), 0);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::LocalIterator GenericHashContainer<sizeType, hashType>::localBegin(sizeType index) const
{
	return LocalIterator(*this, m_bucketList[index].first, index);
}

template<typename sizeType, typename hashType>
inline typename GenericHashContainer<sizeType, hashType>::LocalIterator GenericHashContainer<sizeType, hashType>::localEnd() const
{
	return LocalIterator(*this, sizeLimits::max(), 0);
}

template<class sizeType, class hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::findNext(sizeType current) const
{
	return findNext(m_nodeList[current].hash, m_nodeList[current].next);
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::nodes() const
{
	return m_nodeCount;
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::buckets() const
{
	return m_bucketCount;
}

template<typename sizeType, typename hashType>
inline hashType GenericHashContainer<sizeType, hashType>::hash(sizeType index)
{
	return m_nodeList[index].hash;
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::findNext(hashType hash, sizeType current) const
{
	while (current != sizeLimits::max())
	{
		if (m_nodeList[current].hash == hash)
			return current;
		current = m_nodeList[current].next;
	}

	return sizeLimits::max();
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::nextElement(sizeType current, sizeType &bucket) const
{
	// Iterate over a bucket.
	if (m_nodeList[current].next != sizeLimits::max())
	{
		return m_nodeList[current].next;
	}

	// The end of the bucket is reached. We need to find the next bucket with a valid first pointer.
	while (++bucket != m_bucketCount)
	{
		if (m_bucketList[bucket].first != sizeLimits::max())
		{
			return m_bucketList[bucket].first;
		}
	}

	return std::numeric_limits<sizeType>::max();
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::computeBucketCount(size_t entries)
{
	// It is possible to adjust the container performance by modifying this factor.
	// Increasing it beyond 2 only results in minor performance gains and reducing it
	// below 1 results in severe performance penalties.
	const size_t bucketFactor = 2;
	if (entries >= sizeLimits::max() / bucketFactor)
	{
		throw std::runtime_error("HashContainer: Size is too large.");
	}
	return static_cast<sizeType>(bucketFactor * entries);
}

template<typename sizeType, typename hashType>
inline hashType GenericHashContainer<sizeType, hashType>::high(size_t hash)
{
	// Return the highest part of hash that fits into hashType.
	static const int bits = (sizeof(size_t) - sizeof(hashType)) * 8;
	return static_cast<hashType>(hash >> bits);
}

template<typename sizeType, typename hashType>
inline sizeType GenericHashContainer<sizeType, hashType>::low(size_t hash)
{
	return static_cast<sizeType>(hash);
}

template<typename sizeType, typename hashType>
template<class T>
inline std::unique_ptr<T[]> GenericHashContainer<sizeType, hashType>::copyArray(const std::unique_ptr<T[]> &reference, sizeType size)
{
	std::unique_ptr<T[]> result = std::make_unique<T[]>(size);
	std::copy_n(reference.get(), size, result.get());
	return std::move(result);
}
