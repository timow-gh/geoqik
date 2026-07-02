#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class ConcurrentQueueTest : public ::testing::Test {};

TEST_F(ConcurrentQueueTest, StartsEmpty) {
    geoqik::ConcurrentQueue<int> queue(2);

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.peek(), nullptr);
}

TEST_F(ConcurrentQueueTest, DequeueReturnsItemsInFifoOrder) {
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

TEST_F(ConcurrentQueueTest, SupportsMoveOnlyMessages) {
    geoqik::ConcurrentQueue<std::unique_ptr<int>> queue(1);

    queue.enqueue(std::make_unique<int>(42));
    const auto message = queue.dequeue();

    ASSERT_TRUE(message.has_value());
    ASSERT_NE(message->get(), nullptr);
    EXPECT_EQ(**message, 42);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, ClearRemovesQueuedMessages) {
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

TEST_F(ConcurrentQueueTest, TryEnqueueReturnsFalseWhenFull) {
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

TEST_F(ConcurrentQueueTest, MultipleProducersSingleConsumer) {
    constexpr int producerCount = 4;
    constexpr int itemsPerProducer = 1000;
    constexpr int totalItems = producerCount * itemsPerProducer;

    geoqik::ConcurrentQueue<int> queue(totalItems);

    std::atomic<int> consumed{0};

    std::thread consumer([&]() {
        int count = 0;
        while (count < totalItems) {
            auto item = queue.dequeue();
            if (item.has_value()) {
                ++count;
            }
        }
        consumed.store(count, std::memory_order_release);
    });

    std::vector<std::thread> producers;
    producers.reserve(producerCount);
    for (int p = 0; p < producerCount; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < itemsPerProducer; ++i) {
                queue.enqueue(p * itemsPerProducer + i);
            }
        });
    }

    for (auto& t: producers) {
        t.join();
    }
    consumer.join();

    EXPECT_EQ(consumed.load(), totalItems);
}

TEST_F(ConcurrentQueueTest, ProducerBlocksOnFullQueue) {
    geoqik::ConcurrentQueue<int> queue(2);
    queue.enqueue(1);
    queue.enqueue(2); // queue now full

    std::atomic<bool> enqueueCompleted{false};

    std::thread producer([&]() {
        queue.enqueue(3); // must block until consumer drains one slot
        enqueueCompleted.store(true, std::memory_order_release);
    });

    // Give the producer thread time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(enqueueCompleted.load(std::memory_order_acquire));

    // Consume one item ΓÇö should unblock the producer
    auto item = queue.dequeue();
    ASSERT_TRUE(item.has_value());

    producer.join();
    EXPECT_TRUE(enqueueCompleted.load(std::memory_order_acquire));

    // Drain remaining two items
    ASSERT_TRUE(queue.dequeue().has_value());
    ASSERT_TRUE(queue.dequeue().has_value());
    EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, TryEnqueueUnderContention) {
    constexpr int threadCount = 8;
    geoqik::ConcurrentQueue<int> queue(1);

    std::atomic<int> successCount{0};
    std::atomic<bool> startFlag{false};

    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&, i]() {
            while (!startFlag.load(std::memory_order_acquire)) {}
            auto val = i;
            if (queue.try_enqueue(std::move(val))) {
                successCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    startFlag.store(true, std::memory_order_release);
    for (auto& t: threads) {
        t.join();
    }

    // Exactly one item should be in the queue
    EXPECT_EQ(successCount.load(), 1);
    EXPECT_EQ(queue.size(), 1);
    auto item = queue.dequeue();
    ASSERT_TRUE(item.has_value());
    EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, ConcurrentProducerConsumerNoDeadlock) {
    constexpr int itemCount = 10000;
    geoqik::ConcurrentQueue<int> queue(100);

    std::atomic<int> consumedSum{0};

    std::thread consumer([&]() {
        int count = 0;
        while (count < itemCount) {
            auto item = queue.dequeue();
            if (item.has_value()) {
                consumedSum.fetch_add(*item, std::memory_order_relaxed);
                ++count;
            }
        }
    });

    std::thread producer([&]() {
        for (int i = 0; i < itemCount; ++i) {
            queue.enqueue(std::move(i));
        }
    });

    producer.join();
    consumer.join();

    // Sum of 0..9999
    const int expectedSum = itemCount * (itemCount - 1) / 2;
    EXPECT_EQ(consumedSum.load(), expectedSum);
}
