#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <string>

class ConcurrentQueueTest : public ::testing::Test
{
};

TEST_F(ConcurrentQueueTest, StartsEmpty)
{
  geoqik::ConcurrentQueue<int> queue(2);

  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(queue.peek(), nullptr);
}

TEST_F(ConcurrentQueueTest, DequeueReturnsItemsInFifoOrder)
{
  geoqik::ConcurrentQueue<int> queue(3);

  queue.enqueue(1);
  queue.enqueue(2);

  ASSERT_EQ(queue.size(), 2);
  ASSERT_NE(queue.peek(), nullptr);
  EXPECT_EQ(*queue.peek(), 1);

  const auto first = queue.dequeue();
  const auto second = queue.dequeue();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(*first, 1);
  EXPECT_EQ(*second, 2);
  EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, SupportsMoveOnlyMessages)
{
  geoqik::ConcurrentQueue<std::unique_ptr<int>> queue(1);

  queue.enqueue(std::make_unique<int>(42));
  const auto message = queue.dequeue();

  ASSERT_TRUE(message.has_value());
  ASSERT_NE(message->get(), nullptr);
  EXPECT_EQ(**message, 42);
  EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, ClearRemovesQueuedMessages)
{
  geoqik::ConcurrentQueue<std::string> queue(3);
  queue.enqueue(std::string("first"));
  queue.enqueue(std::string("second"));

  queue.clear();

  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(queue.peek(), nullptr);

  queue.enqueue(std::string("after clear"));
  const auto message = queue.dequeue();

  ASSERT_TRUE(message.has_value());
  EXPECT_EQ(*message, "after clear");
}

TEST_F(ConcurrentQueueTest, TryEnqueueReturnsFalseWhenFull)
{
  geoqik::ConcurrentQueue<int> queue(1);
  ASSERT_TRUE(queue.try_enqueue(1));
  EXPECT_FALSE(queue.try_enqueue(2));

  const auto first = queue.dequeue();
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(*first, 1);

  ASSERT_TRUE(queue.try_enqueue(2));

  const auto second = queue.dequeue();
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(*second, 2);
  EXPECT_TRUE(queue.empty());
}
