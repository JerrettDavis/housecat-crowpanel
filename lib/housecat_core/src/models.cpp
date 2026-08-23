#include "housecat/domain/models.h"

#include <utility>

namespace housecat {

bool NotificationQueue::push(const Notification& notification) {
    // Remove an existing item with the same id before re-inserting it. This
    // deduplicates retries while still re-sorting if its priority changed.
    if (!notification.id.empty()) {
        for (std::size_t index = 0; index < count_; ++index) {
            if (items_[index].id == notification.id) {
                for (std::size_t move = index + 1; move < count_; ++move) {
                    items_[move - 1] = std::move(items_[move]);
                }
                --count_;
                break;
            }
        }
    }

    std::size_t insertAt = count_;
    for (std::size_t index = 0; index < count_; ++index) {
        if (static_cast<int>(notification.priority) > static_cast<int>(items_[index].priority)) {
            insertAt = index;
            break;
        }
    }

    if (count_ == kCapacity && insertAt == count_) {
        return false;
    }

    const std::size_t newCount = count_ < kCapacity ? count_ + 1 : count_;
    for (std::size_t index = newCount - 1; index > insertAt; --index) {
        items_[index] = std::move(items_[index - 1]);
    }
    items_[insertAt] = notification;
    count_ = newCount;
    return true;
}

std::optional<Notification> NotificationQueue::pop() noexcept {
    if (count_ == 0) {
        return std::nullopt;
    }
    Notification result = std::move(items_[0]);
    for (std::size_t index = 1; index < count_; ++index) {
        items_[index - 1] = std::move(items_[index]);
    }
    --count_;
    return result;
}

void NotificationQueue::removeExpired(const std::uint64_t nowMs) noexcept {
    std::size_t write = 0;
    for (std::size_t read = 0; read < count_; ++read) {
        if (!items_[read].isExpired(nowMs)) {
            if (write != read) {
                items_[write] = std::move(items_[read]);
            }
            ++write;
        }
    }
    count_ = write;
}

}  // namespace housecat
