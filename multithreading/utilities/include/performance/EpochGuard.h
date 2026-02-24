#pragma once

#include "./EpochReclamation.h"


namespace multithreading::utilities::performance {

    template <typename T>
    class EpochGuard {
    private:
        EpochReclamation<T>* reclamation;
    public:
        explicit EpochGuard(EpochReclamation<T>* reclamation)
            : reclamation(reclamation)
        {
            reclamation->engage_epoch_user();
        }

        EpochGuard(const EpochGuard&) = delete;
        EpochGuard& operator=(const EpochGuard&) = delete;
        EpochGuard(EpochGuard&&) = delete;
        EpochGuard& operator=(EpochGuard&&) = delete;

        ~EpochGuard() {
            reclamation->free_epoch_user();
        }
    };
} // namespace multithreading::utilities::performance