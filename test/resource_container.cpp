#include <gtest/gtest.h>
#include "ref_counting.hpp"

/**
 * Tests whether the resources get destroyed when the container is destroyed.
 */
TEST(resource_container_test, container_dies_before_resources)
{
	struct my_resource
	{
		int* m_created_counter;
		int* m_del_counter;

		explicit my_resource(int* created_counter, int* del_counter) :
			m_created_counter(created_counter),
			m_del_counter(del_counter)
		{
			(*m_created_counter)++;
		}

		~my_resource()
		{
			(*m_del_counter)++;
		}
	};

	int created_counter = 0, del_counter = 0;
	const int n = 10;

	std::vector<vren::rc<my_resource>> resources;

	{
		vren::resource_container container;
		for (int i = 0; i < n; i++) {
			resources.push_back(
				container.make_rc<my_resource>(&created_counter, &del_counter)
			);
		}
	}

	ASSERT_EQ(created_counter, n) << "The resources weren't created correctly within the container";
	ASSERT_EQ(del_counter, n) << "The container couldn't free its resources";

	for (auto& res : resources)
	{
		ASSERT_FALSE(res.is_valid());
	}
}

/**
 * Tests when the resources are freed before the container is freed.
 */
TEST(resource_container_test, rc_freed_before_container)
{
	vren::resource_container container;

	{
		vren::rc<int> res1 = container.make_rc<int>();
		vren::rc<int> res2 = container.make_rc<int>();
		vren::rc<int> res3 = container.make_rc<int>();
		vren::rc<int> res4 = container.make_rc<int>();

		ASSERT_EQ(container.get_resources_count(), 4);
	}

	ASSERT_EQ(container.get_resources_count(), 0);
}

/**
 * Tests resource empty construction.
 */
TEST(resource_container_test, rc_empty_constructor)
{
	vren::rc<int> res1;
	ASSERT_FALSE(res1.is_valid());
}

/**
 * Tests resource move construction.
 */
TEST(resource_container_test, rc_move_constructor)
{
	vren::resource_container container;

	vren::rc<int> res1 = container.make_rc<int>();

	{
		vren::rc<int> res2 = std::move(res1);

		ASSERT_FALSE(res1.is_valid());

		ASSERT_TRUE(res2.is_valid());
		ASSERT_EQ(res2.use_count(), 1);
	}

	ASSERT_EQ(container.get_resources_count(), 0);
}

/**
 * Tests resource copy construction.
 */
TEST(resource_container_test, rc_copy_constructor)
{
	vren::resource_container container;

	vren::rc<int> res1 = container.make_rc<int>();
	vren::rc<int> res2 = res1;

	ASSERT_EQ(res1.use_count(), 2);
	ASSERT_EQ(res2.use_count(), 2);

	{
		vren::rc<int> res3 = res2;

		ASSERT_EQ(res1.use_count(), 3);
		ASSERT_EQ(res2.use_count(), 3);
		ASSERT_EQ(res3.use_count(), 3);
	}

	ASSERT_EQ(res1.use_count(), 2);
	ASSERT_EQ(res2.use_count(), 2);
}


/**
 * Tests the behavior of `vren::rc::reset()`
 */
TEST(resource_container_test, rc_reset)
{
	vren::resource_container container;

	vren::rc<int> res1 = container.make_rc<int>();

	ASSERT_EQ(res1.use_count(), 1);
	ASSERT_EQ(container.get_resources_count(), 1);

	res1.reset();

	ASSERT_FALSE(res1.is_valid());
	ASSERT_EQ(container.get_resources_count(), 0);
}
