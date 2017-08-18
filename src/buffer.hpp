#pragma once

namespace elelel {
  struct network_buffer {
    using type = network_buffer;
    ~network_buffer();
    network_buffer();
    network_buffer(const size_t capacity);
    network_buffer(const type& other);
    network_buffer(type&& other);
    void swap(type& other);
    type& operator=(const type& other);
    type& operator=(type&& other);

    void reset(); // Resets counters, allocs or reallocs memory
    void reset(const size_t capacity); // Resets counters, allocs or reallocs memory for the capacity
    void pop_front(const size_t sz);  // Release data from buf start
    void push_back(const void* data, const size_t sz); // Copy data into buf
    void clear(); // Clears data from buffer
    void resize(const size_t capacity); // Resizes the buffer, behaves like realloc in respect to sizes, runs defragmentation inside
    void adjust_size_by(const size_t delta); // Adds delta to current size (for adding data in raw mode)
    void defragment(); // If the data is not aligned with start or fragmented, aligns it with buf_ start and defragments

    const void* begin() const;  // Pointer to data start
    void* end() const;          // Pointer to data end (past the last byte)

    bool is_fragmented() const; // Returns true if data is fragmented
    bool is_fragmented(const size_t sz) const; // Returns true if first sz bytes of data are fragmented
    size_t capacity() const; // Return the current capacity
    const size_t& size() const; // Total bytes occupied by data
    size_t head_size() const; // Size of head data fragment
    size_t tail_size() const; // Size of tail data fragment
    size_t free_size() const; // Total free bytes
    size_t free_size_unfragmented() const; // Num of free bytes after end()

    bool& autogrow();
    size_t& autogrow_limit();
    
    double size_expectation() const; // Math expectation of size
    double size_variance() const;    // Variance estimate for size

    const void* raw_buf() const;  // Returns underlying raw buf, for tests
  private:
    void* buf_;  // Memory chunk to hold the data
    size_t capacity_;  // Maximum size of the data that can be held
    bool autogrow_;  // Allow autogrow if size is insufficient for store request?
    size_t autogrow_limit_; // If autogrow, limit on size to grow
    size_t begin_pos_; // Offset in memory chunk where the stored data starts
    size_t count_; // Number of bytes occupied by data

    // Statistics on size, updated on each pop_front operation
    unsigned long size_n_;  // Number of samples
    double size_E_;       // Current expectation
    double size_M2_;      // Sum of squares of differences

  };
}
