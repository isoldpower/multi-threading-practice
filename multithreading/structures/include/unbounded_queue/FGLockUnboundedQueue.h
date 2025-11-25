#pragma once

#include <map>

#include "./UnboundedQueue.h"

namespace multithreading::structures::unbounded_queue {

    template <typename T>
    struct FGNode {
    private:
        T value;
        FGNode* nextNode;
    public:
        explicit FGNode(const T& value)
            : value(value)
            , nextNode(nullptr)
        {}

        explicit FGNode(T&& value)
            : value(std::move(value))
            , nextNode(nullptr)
        {}

        [[nodiscard]] T& get() {
            return value;
        }

        [[nodiscard]] FGNode* next() const {
            return nextNode;
        }

        void setNext(FGNode* next) {
            this->nextNode = next;
        }
    };

    template <typename T>
    class FGLockUnboundedQueue : public UnboundedQueue<T> {
    private:
        mutable std::mutex head_mu;
        mutable std::mutex tail_mu;

        std::condition_variable enqueue_condition;

        FGNode<T>* head;
        FGNode<T>* tail;

        void unsafe_enqueue(FGNode<T>* node) {
            tail->setNext(node);
            tail = node;

            enqueue_condition.notify_one();
        }

        std::optional<T> unsafe_dequeue() {
            FGNode<T>* dummyNode = head;
            FGNode<T>* firstValuableNode = dummyNode->next();

            if (firstValuableNode == nullptr) {
                return std::nullopt;
            }

            std::optional<T> result = std::move(firstValuableNode->get());
            head = firstValuableNode;
            delete dummyNode;

            return result;
        }
    public:
        FGLockUnboundedQueue() {
            head = tail = new FGNode<T>(T{});
        }

        ~FGLockUnboundedQueue() {
            while (FGLockUnboundedQueue<T>::try_dequeue().has_value()) {}

            delete tail;
        }

        void enqueue(const T& value) override {
            auto* node = new FGNode<T>(value);
            std::lock_guard lock(tail_mu);

            unsafe_enqueue(node);
        }

        void enqueue(T&& value) override {
            auto* node = new FGNode<T>(std::move(value));
            std::lock_guard lock(tail_mu);

            unsafe_enqueue(node);
        }

        std::optional<T> try_dequeue() override {
            std::lock_guard lock(head_mu);

            return unsafe_dequeue();
        }

        std::optional<T> wait_dequeue(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            std::unique_lock lock(head_mu);

            if (!enqueue_condition.wait_for(lock, timeout, [this]() {
                return head->next() != nullptr;
            })) {
                return std::nullopt;
            }

            return unsafe_dequeue();
        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return std::async(std::launch::async, [this, timeout]() -> std::optional<T> {
                return this->wait_dequeue(timeout);
            });
        }

        [[nodiscard]] bool is_empty() const override {
            std::lock_guard lock(head_mu);
            return head->next() == nullptr;
        }
    };
} // namespace multithreading::structures::unbounded_queue