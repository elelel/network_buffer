#include "../buffer.hpp"

#include <cstring>
#include <exception>

#include <iostream>

elelel::network_buffer::network_buffer::~network_buffer() {
  if (buf_ != nullptr) {
    free(buf_);
    buf_ = nullptr;
  }
  capacity_ = 0;
  count_ = 0;
}

elelel::network_buffer::network_buffer() :
  buf_(nullptr),
  capacity_{4096},
  autogrow_{false},
  autogrow_limit_{0},
  begin_pos_{0},
  count_{0},
  size_n_{0},
  size_E_{0.0},
  size_M2_{0.0} {
  reset();
}

elelel::network_buffer::network_buffer(const size_t capacity) :
  network_buffer() {
  reset(capacity);
}

elelel::network_buffer::network_buffer(const type& other) :
  capacity_{other.capacity_},
  autogrow_{other.autogrow_},
  autogrow_limit_{other.autogrow_limit_},
  begin_pos_{other.begin_pos_},
  count_{other.count_},
  size_n_{other.size_n_},
  size_E_{other.size_E_},
  size_M2_{other.size_M2_} {
    // Since it we are copying anyway, take advantage of this to defragment the data
    buf_ = malloc(capacity_);
    if (buf_ == nullptr) throw std::bad_alloc();
    memcpy(buf_, (void*)((uintptr_t)other.buf_ + other.begin_pos_), other.head_size());
    memcpy((void*)((uintptr_t)buf_ + other.head_size()), other.buf_, other.tail_size());
  }

elelel::network_buffer::network_buffer(type&& other) :
  buf_(std::move(other.buf_)),
  capacity_(std::move(other.capacity_)),
  autogrow_(std::move(other.autogrow_)),
  autogrow_limit_(std::move(other.autogrow_limit_)),
  begin_pos_(std::move(other.begin_pos_)),
  count_(std::move(other.count_)),
  size_n_(std::move(other.size_n_)),
  size_E_(std::move(other.size_E_)),
  size_M2_(std::move(other.size_M2_)) {
  }

void elelel::network_buffer::swap(type& other) {
  std::swap(buf_, other.buf_);
  std::swap(capacity_, other.capacity_);
  std::swap(autogrow_, other.autogrow_);
  std::swap(autogrow_limit_, other.autogrow_limit_);
  std::swap(begin_pos_, other.begin_pos_);
  std::swap(count_, other.count_);
  std::swap(size_n_, other.size_n_);
  std::swap(size_E_, other.size_E_);
  std::swap(size_M2_, other.size_M2_);
}

auto elelel::network_buffer::operator=(const type& other) -> type& {
  type tmp(other);
  swap(tmp);
  return *this;
}

auto elelel::network_buffer::operator=(type&& other) -> type& {
  buf_ = std::move(other.buf_);
  capacity_ = std::move(other.capacity_);
  autogrow_ = std::move(other.autogrow_);
  autogrow_limit_ = std::move(other.autogrow_limit_);
  begin_pos_ = std::move(other.begin_pos_);
  count_ = std::move(other.count_);
  size_n_ = std::move(other.size_n_);
  size_E_ = std::move(other.size_E_);
  size_M2_ = std::move(other.size_M2_);
  return *this;
}

void elelel::network_buffer::reset() {
  reset(capacity_);
}

void elelel::network_buffer::reset(const size_t capacity) {
  if (buf_ != nullptr) {
    auto new_buf = realloc(buf_, capacity_);
    if (new_buf == nullptr) throw std::bad_alloc();
    buf_ = new_buf;
  } else {
    buf_ = malloc(capacity_);
    if (buf_ == nullptr) throw std::bad_alloc();
  }
  capacity_ = capacity;
  count_ = 0;
  begin_pos_ = 0;
  size_n_ = 0;
  size_E_ = 0;
  size_M2_ = 0;
}

void elelel::network_buffer::pop_front(const size_t sz) {
  // Caller is responsible to ensure sz <= size()

  // Update statistics on used buffer size
  ++size_n_;
  const auto delta = count_ - size_E_;
  size_E_ += delta / double(size_n_);
  size_M2_ += delta * (count_ - size_E_);

  if (is_fragmented()) {
    const size_t head_sz = capacity_ - begin_pos_;
    const size_t tail_sz = count_ - head_sz;
    if (sz < head_sz) {
      begin_pos_ += sz;
    } else {
      begin_pos_ = sz - (capacity_ - begin_pos_);
    }
  } else {
    if (sz == count_) {
      begin_pos_ = 0;
    } else {
      begin_pos_ += sz;
    }
  }
  count_ -= sz;
}

void elelel::network_buffer::resize(const size_t sz) {
  defragment();
  capacity_ = sz;
  if (count_ > sz) count_ = sz;
  auto new_buf = realloc(buf_, sz);
  if (new_buf == nullptr) throw std::bad_alloc();
  buf_ = new_buf;
}

void elelel::network_buffer::adjust_size_by(const size_t delta) {
  count_ += delta;
}

void elelel::network_buffer::defragment() {
  if (begin_pos_ != 0) {
    const auto begin_p = (void*)((uintptr_t)buf_ + begin_pos_);
    if (is_fragmented()) {
      void* p = malloc(capacity_);
      if (buf_ == nullptr) {
        throw std::bad_alloc();
      }
      const size_t head_sz = capacity_ - begin_pos_;
      const size_t tail_sz = count_ - head_sz;
      memcpy(p, begin_p, head_sz);
      memcpy((void*)((uintptr_t)p + head_sz), buf_, tail_sz);
      free(buf_);
      buf_ = p;
    } else {
      memmove(buf_, begin_p, count_);
    }
  }
  begin_pos_ = 0;
}

void elelel::network_buffer::push_back(const void* data, const size_t sz) {
  const size_t free_space = capacity_ - count_;
  if (sz > free_space) {
    if (autogrow_ && (autogrow_limit - count_ >= sz)) {
      resize(sz);
    } else {
      throw std::runtime_error("elelel::network_buffer out of buffer space");
    }
  }
  void* p;
  if (is_fragmented()) {
    const size_t head_sz = capacity_ - begin_pos_;
    const size_t tail_sz = count_ - head_sz;
    memcpy((void*)((uintptr_t)buf_ + tail_sz), data, sz);
  } else {
    const size_t head_sz = count_ ? capacity_ - begin_pos_ : 0;
    const size_t after_head_sz = count_ ? capacity_ - (begin_pos_ + count_) : capacity_;
    if (after_head_sz >= sz) {
      memcpy((void*)((uintptr_t)buf_ + begin_pos_ + head_sz), data, sz);
    } else {
      memcpy((void*)((uintptr_t)buf_ + begin_pos_ + head_sz), data, after_head_sz);
      memcpy(buf_, (void*)((uintptr_t)data + after_head_sz), sz - after_head_sz);
    }
  }
  count_ += sz;
}

const void* elelel::network_buffer::begin() const {
  return (void*)((uintptr_t)buf_ + begin_pos_);
}

void* elelel::network_buffer::end() const {
  if (is_fragmented()) {
    const size_t head_sz = capacity_ - begin_pos_;
    const size_t tail_sz = count_ - head_sz;
    return (void*)((uintptr_t)buf_ + tail_sz);
  } 
  return (void*)((uintptr_t)buf_ + begin_pos_ + count_);
}

bool elelel::network_buffer::is_fragmented() const {
  return begin_pos_ + count_ > capacity_;
}

bool elelel::network_buffer::is_fragmented(const size_t sz) const {
  auto clamp = sz;
  if (clamp > count_) clamp = count_;
  return begin_pos_ + clamp > capacity_;
}

const size_t& elelel::network_buffer::size() const {
  return count_;
}

size_t elelel::network_buffer::capacity() const {
  return capacity_;
}

size_t elelel::network_buffer::head_size() const {
  if (is_fragmented()) {
    return capacity_ - begin_pos_;
  }
  return count_;
}

size_t elelel::network_buffer::tail_size() const {
  if (is_fragmented()) {
    const size_t head_sz = capacity_ - begin_pos_;
    return count_ - head_sz;
  }
  return 0;
}

size_t elelel::network_buffer::free_size() const {
  return capacity_ - count_;
}

size_t elelel::network_buffer::free_size_unfragmented() const {
  if (is_fragmented()) {
    const size_t head_sz = capacity_ - begin_pos_;
    const size_t tail_sz = count_ - (capacity_ - begin_pos_);
    return begin_pos_ - tail_sz;
  }
  return count_ ? capacity_ - (begin_pos_ + count_) : capacity_;
}

bool& elelel::network_buffer::autogrow() {
  return autogrow_;
}

size_t& elelel::network_buffer::autogrow_limit() {
  return autogrow_limit_;
}

double elelel::network_buffer::size_expectation() const {
  if (size_n_ > 2) {
    return size_E_;
  }
  return double{};
}

double elelel::network_buffer::size_variance() const {
  if (size_n_ > 2) {
    return size_M2_ / double(size_n_ - 1);
  }
  return double{};
}

const void* elelel::network_buffer::raw_buf() const {
  return buf_;
}
