#pragma once

#include "FixedUninitVec.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace dnsge {

namespace detail {

// NOLINTBEGIN(readability-magic-numbers)

using metadata_t = uint8_t;

enum Metadata : metadata_t {
    Empty = 0b10000000,
    Deleted = 0b11111111,
};

constexpr bool IsFree(metadata_t metadata) {
    // Check if the high bit of metadata is set
    return metadata >= 0b10000000;
}

constexpr size_t H1(size_t hash) {
    // Remove the lower 7 bits of hash
    return hash >> 7;
}

constexpr metadata_t H2(size_t hash) {
    // Get the lower 7 bits of hash, i.e. last byte with top bit 0
    return hash & 0x7F;
}

static_assert(IsFree(Metadata::Empty), "Empty should be considered free");
static_assert(IsFree(Metadata::Deleted), "Deleted should be considered free");
static_assert(!IsFree(H2(0xFFFF)), "H2 of 0xFFFF should be not be considered free");

// NOLINTEND(readability-magic-numbers)

}; // namespace detail

template <typename K, typename V, typename Hash = std::hash<K>, typename Eq = std::equal_to<K>>
class HashMap {
public:
    static constexpr size_t DefaultInitialCapacity = 16;
    static constexpr float MaxLoadFactor = 0.875;
    static constexpr float MaxDeletedLoadFactor = 0.875;
    static constexpr float GrowthFactor = 2;

    using Slot = std::pair<const K, V>;

    static_assert(std::is_copy_assignable_v<K>, "Key must be copy assignable");
    static_assert(std::is_move_constructible_v<Slot>, "Slot must be move constructable");

    template <typename SlotType>
    class MapIterator {
    private:
        MapIterator(size_t idex, SlotType* slotPtr)
            : idex_(idex)
            , slotPtr_(slotPtr) {}

    public:
        SlotType &operator*() const {
            return *this->slotPtr_;
        }

        SlotType* operator->() const {
            return this->slotPtr_;
        }

        bool operator==(const MapIterator &other) const {
            return this->idex_ == other.idex_;
        }

        bool operator!=(const MapIterator &other) const {
            return this->idex_ != other.idex_;
        }

    private:
        friend class HashMap<K, V, Hash, Eq>;
        size_t idex_;
        SlotType* slotPtr_;
    };

    using iterator = MapIterator<Slot>;
    using const_iterator = MapIterator<const Slot>;

    /**
     * @brief Construct a new HashMap with a default capacity.
     */
    HashMap()
        : HashMap(DefaultInitialCapacity) {}

    /**
     * @brief Construct a new HashMap with a specified capacity.
     * 
     * @param initialCapacity Initial slot capacity.
     */
    HashMap(size_t initialCapacity)
        : capacity_(initialCapacity)
        , size_(0)
        , slots_(initialCapacity)
        , deletedCount_(0) {
        this->metadata_.resize(initialCapacity, detail::Metadata::Empty);
    }

    ~HashMap() {
        if (this->empty()) {
            return;
        }
        for (size_t i = 0; i < this->capacity_; ++i) {
            if (!detail::IsFree(this->metadata_[i])) {
                this->slots_[i].~Slot(); // Destroy slot entry
            }
        }
    }

    HashMap(const HashMap &other) = default;
    HashMap &operator=(const HashMap &other) = default;

    HashMap(HashMap &&other) noexcept
        : capacity_(other.capacity_)
        , size_(other.size_)
        , metadata_(std::move(other.metadata_))
        , slots_(std::move(other.slots_))
        , deletedCount_(other.deletedCount_) {
        other.capacity_ = 0;
        other.size_ = 0;
        other.deletedCount_ = 0;
    }

    HashMap &operator=(HashMap &&other) noexcept {
        this->capacity_ = other.capacity_;
        this->size_ = other.size_;
        this->metadata_ = std::move(other.metadata_);
        this->slots_ = std::move(other.slots_);
        this->deletedCount_ = other.deletedCount_;
        other.capacity_ = 0;
        other.size_ = 0;
        other.deletedCount_ = 0;
        return *this;
    }

    /**
     * @brief Find a key-value pair in the HashMap.
     * 
     * @param key Key to look up.
     * @return Iterator to the element, or end() if not found.
     */
    iterator find(const K &key) {
        return this->iteratorAt(this->doFind(key));
    }

    /**
     * @brief Find a key-value pair in the HashMap.
     * 
     * @param key Key to look up.
     * @return Iterator to the element, or end() if not found.
     */
    const_iterator find(const K &key) const {
        return this->iteratorAt(this->doFind(key));
    }

    /**
     * @brief Check whether a key is present in the HashMap.
     * 
     * @param key Key to look for.
     */
    bool contains(const K &key) const {
        return this->doFind(key).has_value();
    }

    /**
     * @brief Get the value of a key in the HashMap. Throws std::out_of_range if
     * the key is not in the HashMap.
     *
     * @param key Key to look up.
     * @return Reference to found value.
     */
    V &at(const K &key) {
        auto res = this->doFind(key);
        if (res) {
            return this->slots_[res.value()].second;
        }
        throw std::out_of_range("key not found");
    }

    /**
     * @brief Insert a key-value pair into the HashMap. Returns the iterator to
     * the inserted key-value pair, or std::nullopt if the key already has a value.
     * 
     * @param value Key-value pair to insert.
     * @return Iterator or std::nullopt if already exists.
     */
    std::optional<iterator> insert(const std::pair<K, V> &value) {
        // Check if we need to grow
        if (this->needRehashBeforeInsertion()) {
            this->growOrRehash();
        }
        // Perform the insert
        return this->insertUnchecked(value);
    }

    /**
     * @brief Insert a key-value pair into the HashMap. Returns the iterator to
     * the inserted key-value pair, or std::nullopt if the key already has a value.
     * 
     * @param value Key-value pair to insert.
     * @return Iterator or std::nullopt if already exists.
     */
    std::optional<iterator> insert(std::pair<K, V> &&value) {
        // Check if we need to grow
        if (this->needRehashBeforeInsertion()) {
            this->growOrRehash();
        }
        // Perform the insert
        return this->insertUnchecked(std::move(value));
    }

    /**
     * @brief Get the value of a key in the HashMap. If the key is not present,
     * a default-constructed value is first inserted.
     * 
     * @param key Key to look up.
     * @return Reference to the value.
     */
    template <typename U = V>
    typename std::enable_if_t<std::is_default_constructible_v<U>, V &> operator[](const K &key) {
        auto res = this->doFind(key);
        if (res) {
            return this->slots_[res.value()].second;
        }
        // Need to insert and then return
        auto it = this->insert(std::make_pair<K, V>(K{key}, V{}));
        assert(it.has_value());
        return this->slots_[it->idex_].second;
    }

    /**
     * @brief Erase the key-value pair at an iterator. Returns whether the
     * key-value pair was successfully removed.
     * 
     * @param it Iterator to delete
     * @return Whether the key-value pair was found and removed. 
     */
    bool erase(iterator it) {
        if (it == this->end()) {
            return false;
        }
        assert(it.idex_ < this->capacity_);
        // Delete data associated with key-value pair
        this->destroySlot(it.idex_);
        // Decrement size
        --this->size_;

        // Check if we have an elevated number of deleted slots.
        // If so, we want to rehash everything to prevent fragmentation.
        if (this->needRehash()) {
            this->rehashEverything();
        }

        return true;
    }

    /**
     * @brief Erase the key-value pair of a key. Returns whether the
     * key-value pair was successfully removed.
     * 
     * @param key Key to erase.
     * @return Whether the key-value pair was found and removed. 
     */
    bool erase(const K &key) {
        return this->erase(this->find(key));
    }

    /**
     * @brief Clear all elements from the HashMap.
     */
    void clear() {
        if (this->empty()) {
            return;
        }
        for (size_t i = 0; i < this->capacity_; ++i) {
            if (!detail::IsFree(this->metadata_[i])) {
                this->slots_[i].~Slot(); // Destroy slot entry
            }
            this->metadata_[i] = detail::Metadata::Empty;
        }
        this->size_ = 0;
        this->deletedCount_ = 0;
    }

    /**
     * @brief Reserve at least enough empty slots. Does not consider the load factor.
     * Causes a rehash if growing is required.
     * @param n Number of slots to reserve.
     */
    void reserve(size_t n) {
        auto target = static_cast<size_t>(n / MaxLoadFactor);
        if (this->capacity_ >= target) {
            return;
        }
        this->growAndRehash(target);
    }

    /**
     * @brief Get the number of elements in the HashMap
     */
    size_t size() const {
        return this->size_;
    }

    /**
     * @brief Get the slot capacity of the HashMap.
     */
    size_t capacity() const {
        return this->capacity_;
    }

    /**
     * @brief Check whether the HashMap is empty.
     */
    bool empty() const {
        return this->size_ == 0;
    }

    /**
     * @brief Get an "end" iterator. "end" points to an invalid element and is
     * returned from HashMap operations as a sentinel value.
     */
    inline iterator end() {
        return iteratorAt(this->slots_.size());
    }

    /**
     * @brief Get an "end" iterator. "end" points to an invalid element and is
     * returned from HashMap operations as a sentinel value.
     */
    inline const_iterator end() const {
        return iteratorAt(this->slots_.size());
    }

private:
    struct InsertionLoc {
        // The internal HashMap index
        size_t idex;
        // The level 2 hash of the key (for metadata)
        size_t h2;
    };

    /**
     * @brief Compute the internal location for inserting a key.
     * 
     * @param key Key for insertion.
     * @return The insertion location, or std::nullopt if the key already exists.
     */
    std::optional<InsertionLoc> locationForInsertion(const K &key) const {
        Hash hasher;
        Eq eq;
        size_t hash = hasher(key);
        auto h1 = detail::H1(hash);
        auto h2 = detail::H2(hash);

        size_t idex = h1 % this->capacity_;
        while (true) {
            auto metadata = this->metadata_[idex];
            if (detail::IsFree(metadata)) {
                // Found free spot for insertion
                return InsertionLoc{idex, h2};
            } else if (metadata == h2 && eq(key, this->slots_[idex].first)) {
                // Key already exists
                return std::nullopt;
            }
            idex = (idex + 1) % this->capacity_;
        }
    }

    /**
     * @brief Insert a key-value pair without checking for capacity.
     * 
     * @param value Key-value pair to insert.
     * @return Iterator to the inserted key-value pair, or std::nullopt if key already exists.
     */
    std::optional<iterator> insertUnchecked(const std::pair<K, V> &value) {
        if (auto loc = this->locationForInsertion(value.first)) {
            // Insert value into slot. Update the metadata with the hash data.
            this->metadata_[loc->idex] = loc->h2;
            // Construct new slot entry in place
            new (&this->slots_[loc->idex]) Slot{value.first, value.second};
            // Increment size
            ++this->size_;
            return this->iteratorAt(loc->idex);
        }
        return std::nullopt;
    }

    /**
     * @brief Insert a key-value pair without checking for capacity.
     * 
     * @param value Key-value pair to insert.
     * @return Iterator to the inserted key-value pair, or std::nullopt if key already exists.
     */
    std::optional<iterator> insertUnchecked(std::pair<K, V> &&value) {
        if (auto loc = this->locationForInsertion(value.first)) {
            // Insert value into slot. Update the metadata with the hash data.
            this->metadata_[loc->idex] = loc->h2;
            // Construct new slot entry in place, moving the values.
            new (&this->slots_[loc->idex]) Slot{std::move(value.first), std::move(value.second)};
            // Increment size
            ++this->size_;
            return this->iteratorAt(loc->idex);
        }
        return std::nullopt;
    }

    /**
     * @brief Insert a slot rvalue.
     * 
     * @param slot Slot to insert.
     * @return Iterator to the inserted slot, or std::nullopt if key already exists.
     */
    std::optional<iterator> insertUnchecked(Slot &&slot) {
        if (auto loc = this->locationForInsertion(slot.first)) {
            // Insert value into slot. Update the metadata with the hash data.
            this->metadata_[loc->idex] = loc->h2;
            // Move old slot into new slot.
            new (&this->slots_[loc->idex]) Slot{std::move(slot)};
            // Increment size
            ++this->size_;
            return this->iteratorAt(loc->idex);
        }
        return std::nullopt;
    }

    /**
     * @brief Destroy the slot at an index. Update the metadata and call the slot destructor.
     * 
     * @param idex Index to destroy at.
     */
    void destroySlot(size_t idex) {
        // Mark slot as deleted
        this->metadata_[idex] = detail::Metadata::Deleted;
        ++this->deletedCount_;
        // Call destructor on slot entry
        this->slots_[idex].~Slot();
    }

    /**
     * @brief Get an iterator to an internal HashTable index.
     */
    inline iterator iteratorAt(size_t idex) {
        return iterator(idex, this->slots_.data() + idex);
    }

    /**
     * @brief Get an iterator to an internal HashTable index.
     */
    inline const_iterator iteratorAt(size_t idex) const {
        return const_iterator(idex, this->slots_.data() + idex);
    }

    /**
     * @brief Get an iterator to an internal HashTable index. Returns iterator to end
     * if value is not set.
     */
    inline iterator iteratorAt(std::optional<size_t> maybeIdex) {
        if (maybeIdex) {
            return iteratorAt(*maybeIdex);
        }
        return this->end();
    }

    /**
     * @brief Get an iterator to an internal HashTable index. Returns iterator to end
     * if value is not set.
     */
    inline const_iterator iteratorAt(std::optional<size_t> maybeIdex) const {
        if (maybeIdex) {
            return iteratorAt(*maybeIdex);
        }
        return this->end();
    }

    /**
     * @brief Find a key in the HashTable.
     * 
     * @param key Key to find.
     * @return Internal HashTable index of key-value, or std::nullopt if not found.
     */
    std::optional<size_t> doFind(const K &key) const {
        if (this->empty()) {
            return std::nullopt;
        }
        Hash hasher;
        Eq eq;
        size_t hash = hasher(key);
        auto h1 = detail::H1(hash);
        auto h2 = detail::H2(hash);

        size_t idex = h1 % this->capacity_;
        while (true) {
            auto metadata = this->metadata_[idex];
            if (metadata == h2 && eq(key, this->slots_[idex].first)) {
                // Found key
                return idex;
            } else if (metadata == detail::Metadata::Empty) {
                // Found empty slot, must not be in hash table
                return std::nullopt;
            }
            idex = (idex + 1) % this->capacity_;
        }
    }

    void growOrRehash() {
        size_t effectiveSize = this->effectiveSize();
        if ((float)this->deletedCount_ / effectiveSize > MaxDeletedLoadFactor) {
            // A lot of capacity is being used by deleted slots, rehash everything
            this->rehashEverything();
        } else {
            // Capacity is limited due to elements, grow AND rehash everything
            this->growAndRehash();
        }
    }

    /**
     * @brief Double the capacity and rehash the table.
     */
    void growAndRehash() {
        size_t newCapacity = static_cast<size_t>(this->capacity_ * GrowthFactor);
        if (this->capacity_ == 0) {
            newCapacity = DefaultInitialCapacity;
        }
        this->growAndRehash(newCapacity);
    }

    /**
     * @brief Increase the capacity and rehash the table.
     * 
     * @param newCapacity 
     */
    void growAndRehash(size_t newCapacity) {
        if (newCapacity <= this->capacity_) {
            return;
        }

        // Temporary new table to move existing elements into
        HashMap<K, V, Hash, Eq> newTable(newCapacity);

        if (!this->empty()) {
            for (size_t i = 0; i < this->capacity_; ++i) {
                if (!detail::IsFree(this->metadata_[i])) {
                    // Move slot data into new table
                    newTable.insertUnchecked(std::move(this->slots_[i]));
                    // Call destructor on residual slot
                    this->slots_[i].~Slot();
                    // Decrement size to avoid additional cleanup when *this is dropped
                    --this->size_;
                }
            }
        }

        // Swap internals with temporary table
        *this = std::move(newTable);
    }

    /**
     * @brief Rehash the HashTable without changing capacity.
     */
    void rehashEverything() {
        if (this->empty()) {
            return;
        }

        // Temporary new table to move existing elements into
        HashMap<K, V, Hash, Eq> newTable(this->capacity_);
        for (size_t i = 0; i < this->capacity_; ++i) {
            if (!detail::IsFree(this->metadata_[i])) {
                // Move slot data into new table
                newTable.insertUnchecked(std::move(this->slots_[i]));
                // Call destructor on residual slot
                this->slots_[i].~Slot();
                // Decrement size to avoid additional cleanup when *this is dropped
                --this->size_;
            }
        }

        assert(this->size_ == 0);
        // Swap internals with temporary table
        *this = std::move(newTable);
    }

    size_t effectiveSize() const {
        return this->size_ + this->deletedCount_;
    }

    /**
     * @brief Compute the current effective load factor.
     */
    float effectiveLoadFactor() const {
        return ((float)this->effectiveSize()) / this->capacity_;
    }

    /**
     * @brief Compute the load factor after an insertion.
     */
    float loadFactorAfterInsertion() const {
        return ((float)(this->size_ + 1)) / this->capacity_;
    }

    /**
     * @brief Check whether the load factor is too high and we need to rehash.
     */
    bool needRehash() const {
        static_assert(MaxLoadFactor <= 1);
        return this->effectiveLoadFactor() >= MaxLoadFactor || this->capacity_ == 0;
    }

    /**
     * @brief Check whether the load factor will be too high after an insertion.
     */
    bool needRehashBeforeInsertion() const {
        return this->loadFactorAfterInsertion() >= MaxLoadFactor || this->capacity_ == 0;
    }

    size_t capacity_;
    size_t size_;

    std::vector<detail::metadata_t> metadata_;
    FixedUninitVec<Slot> slots_;

    size_t deletedCount_;
};

} // namespace dnsge
