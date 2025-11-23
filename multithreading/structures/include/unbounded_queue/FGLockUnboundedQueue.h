#pragma once

#include <map>

#include "./UnboundedQueue.h"

namespace multithreading::structures::unbounded_queue {

    template <typename T>
    struct Node {
    private:
        T value;
        Node* nextNode;
    public:
        explicit Node(const T& value)
            : value(value)
            , nextNode(nullptr)
        {}

        explicit Node(T&& value)
            : value(std::move(value))
            , nextNode(nullptr)
        {}

        [[nodiscard]] T& get() {
            return value;
        }

        [[nodiscard]] Node* next() const {
            return nextNode;
        }

        void setNext(Node* next) {
            this->nextNode = next;
        }
    };

    template <typename T>
    class FGLockUnboundedQueue : public UnboundedQueue<T> {
    private:
        mutable std::mutex head_mu;
        mutable std::mutex tail_mu;

        std::condition_variable enqueue_condition;

        Node<T>* head;
        Node<T>* tail;

        std::optional<T> unsafe_dequeue() {
            Node<T>* dummyNode = head;
            Node<T>* firstValuableNode = dummyNode->next();

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
            head = tail = new Node<T>(T{});
        }

        ~FGLockUnboundedQueue() {
            while (FGLockUnboundedQueue<T>::try_dequeue().has_value()) {}

            delete tail;
        }

        void enqueue(const T& value) override {
            auto* node = new Node<T>(value);
            std::lock_guard lock(tail_mu);

            tail->setNext(node);
            tail = node;

            enqueue_condition.notify_one();
        }

        void enqueue(T&& value) override {
            auto* node = new Node<T>(std::move(value));
            std::lock_guard lock(tail_mu);

            tail->setNext(node);
            tail = node;

            enqueue_condition.notify_one();
        }

        std::optional<T> try_dequeue() override {
            std::lock_guard lock(head_mu);

            return unsafe_dequeue();
        }

        std::future<std::optional<T>> wait_dequeue(const std::chrono::milliseconds& timeout) override {
            return std::async(std::launch::async, [this, timeout]() -> std::optional<T> {
                std::unique_lock lock(head_mu);

                if (!enqueue_condition.wait_for(lock, timeout, [this]() {
                    return head->next() != nullptr;
                })) {
                    return std::nullopt;
                }

                return unsafe_dequeue();
            });
        }

        [[nodiscard]] bool is_empty() const override {
            std::lock_guard lock(head_mu);
            return head->next() == nullptr;
        }
    };
} // namespace multithreading::structures::unbounded_queue