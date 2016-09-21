#include <gtest/gtest.h>

#include <hashcontainer.h>

const std::vector<size_t> sizes = {1, 4, 7, 12, 41, 99, 120};

template<typename container_t>
struct HashContainer_test : testing::Test
{
};

using all_container_ts = ::testing::Types<
	HashContainer,
	SparseHashContainer,
	GenericHashContainer<uint8_t, uint8_t>,
	GenericHashContainer<uint8_t, uint16_t>,
	GenericHashContainer<uint8_t, uint32_t>,
	GenericHashContainer<uint16_t, uint8_t>,
	GenericHashContainer<uint16_t, uint16_t>,
	GenericHashContainer<uint16_t, uint32_t>,
	GenericHashContainer<uint32_t, uint8_t>,
	GenericHashContainer<uint64_t, uint8_t>,
	GenericHashContainer<uint64_t, uint16_t>,
	GenericHashContainer<uint64_t, uint32_t>>;
TYPED_TEST_CASE(HashContainer_test, all_container_ts);

TYPED_TEST(HashContainer_test, initialize_different_sizes_no_throw)
{
	for (auto size : sizes)
	{
		EXPECT_NO_THROW(TypeParam container(size));
	}
}

TYPED_TEST(HashContainer_test, initialize_zero_size)
{
	EXPECT_NO_THROW(TypeParam container(0));
}

TYPED_TEST(HashContainer_test, initialize_large_sizes_throw)
{
	EXPECT_THROW(TypeParam container(static_cast<size_t>(TypeParam::sizeLimits::max() * 0.75f)), std::runtime_error);
}

TYPED_TEST(HashContainer_test, initialize_very_large_sizes_throw)
{
	EXPECT_THROW(TypeParam container(TypeParam::sizeLimits::max()), std::runtime_error);
}

TYPED_TEST(HashContainer_test, clear_content)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.insert(i, i);
		}

		for (uint32_t i = 0; i < size; ++i)
		{
			auto it = container.find(i);

			ASSERT_EQ(*it, i);
			ASSERT_FALSE(++it);
		}

		container.clear();
		ASSERT_FALSE(container.begin());
	}
}

TYPED_TEST(HashContainer_test, insert_n_elements_into_n_buckets)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.insert(i, i);
		}

		for (uint32_t i = 0; i < size; ++i)
		{
			auto it = container.find(i);

			ASSERT_EQ(*it, i);
			ASSERT_FALSE(++it);
		}
	}
}

TYPED_TEST(HashContainer_test, find_emplaced_unique)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.emplace(i, i);
		}

		for (uint32_t i = 0; i < size; ++i)
		{
			ASSERT_FALSE(container.findEmplaced(i));
		}

		for (uint32_t i = 0; i < size; ++i)
		{
			container.insertEmplaced(i);

			auto it = container.find(i);
			ASSERT_TRUE(it);
			ASSERT_FALSE(++it);
		}
	}
}

TYPED_TEST(HashContainer_test, find_emplaced_common)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.emplace(0, i);
		}
		container.insertEmplaced(0);

		for (uint32_t i = 1; i < size; ++i)
		{
			auto it = container.findEmplaced(i);
			ASSERT_TRUE(it);
			ASSERT_FALSE(++it);
		}
	}
}

TYPED_TEST(HashContainer_test, insert_n_elements_in_1_buckets)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.insert(0, i);
		}

		auto it = container.find(0);
		for (uint32_t i = 0; i < size; ++i)
		{
			ASSERT_EQ(*it, size - i - 1);
			++it;
		}
		ASSERT_FALSE(it);
	}
}

TYPED_TEST(HashContainer_test, remove_all_elements_iterativ)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (int repeat = 0; repeat < 5; ++repeat)
		{
			for (uint32_t i = 0; i < size; ++i)
			{
				ASSERT_FALSE(container.find(i));
			}

			for (uint32_t i = 0; i < size; ++i)
			{
				container.insert(i, i);
			}

			for (uint32_t i = 0; i < size; ++i)
			{
				container.remove(i, i);
			}
		}
	}
}

TYPED_TEST(HashContainer_test, remove_one_element)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; i++)
		{
			container.insert(i, i);
		}

		container.remove(0, 0);
		ASSERT_FALSE(container.find(0));
		for (uint32_t i = 1; i < size; ++i)
		{
			ASSERT_TRUE(container.find(i));
		}
	}
}

TYPED_TEST(HashContainer_test, find_all_elements_with_same_hash)
{
	for (auto size : sizes)
	{
		TypeParam container(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			container.insert(i / 2, i);
		}

		for (uint32_t i = 0; i < size / 2; ++i)
		{
			auto it = container.find(i);
			ASSERT_TRUE(it);
			ASSERT_TRUE(++it);
			ASSERT_FALSE(++it);
		}

		ASSERT_FALSE(container.find(size));
	}
}

TYPED_TEST(HashContainer_test, iterateor_invalid_when_container_empty)
{
	for (auto size : sizes)
	{
		TypeParam container(size);

		ASSERT_FALSE(container.begin());
		ASSERT_FALSE(container.end());
	}
}

TYPED_TEST(HashContainer_test, iterate_over_filled_container)
{
	for (auto size : sizes)
	{
		for (auto fill : sizes)
		{
			if (fill > size)
			{
				continue;
			}

			TypeParam container(size);
			for (uint32_t i = 0; i < fill; ++i)
			{
				container.insert(i, i);
			}

			auto it = container.begin();
			for (uint32_t i = 0; i < fill; ++i)
			{
				ASSERT_TRUE(it);
				++it;
			}
			ASSERT_FALSE(it);
		}
	}
}

TYPED_TEST(HashContainer_test, iterator_operator_bool)
{
	TypeParam container(1);
	container.insert(0, 0);

	auto it = container.find(0);
	ASSERT_TRUE(it);
	ASSERT_FALSE(++it);

	ASSERT_FALSE(container.find(1));
}
