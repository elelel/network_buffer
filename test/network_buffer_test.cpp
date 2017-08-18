#include "../contrib/Catch/include/catch.hpp"

#include "../include/elelel/network_buffer"

#include <iostream>

using namespace elelel;

bool operator==(const network_buffer& buf, const std::string& string) {
  std::string s;
  for (const auto& c : string) {
    if ((c != '[') && (c != ']') && (c != ' ')) s.push_back(c);
  }
  if (s.size() != buf.capacity()) return false;
  auto b = (char*)buf.raw_buf();
  const size_t head_begin_idx = (uintptr_t)buf.begin() - (uintptr_t)buf.raw_buf();
  const size_t head_end_idx = head_begin_idx + buf.head_size() - 1;
  bool do_tail{false};
  const size_t tail_begin_idx{0};
  size_t tail_end_idx{0};  
  if (buf.is_fragmented()) {
    do_tail = true;
    tail_end_idx = buf.tail_size() - 1;
  }
    
  auto within_head = [head_begin_idx, head_end_idx] (size_t i) {
    return (i >= head_begin_idx) && (i <= head_end_idx);
  };
  auto within_tail = [tail_begin_idx, tail_end_idx, do_tail] (size_t i) {
    return do_tail && (i >= tail_begin_idx) && (i <= tail_end_idx);
  };

  // std::cout << "Start compare cycle\n";
  for (auto i = 0; i < s.size(); ++i) {
    // std::cout << " Comparing s[" << i << "] = " << s[i] << " and b[" << i << "] = " << b[i] << std::endl;
    if ((within_tail(i) || within_head(i))) {
      if (((s[i] != '_') && (b[i] != s[i])) || (s[i] == '_')) return false;
    }
  }
  return true;
}

bool operator==(const std::string& string, const network_buffer& buf) {
  return buf == string;
}

bool operator!=(const network_buffer& buf, const std::string& string) {
  return !(buf == string);
}

bool operator!=(const std::string& string, const network_buffer& buf) {
  return !(buf == string);
}

network_buffer& operator<<(network_buffer& buf, const std::string& string) {
  buf.push_back(string.c_str(), string.size());
  return buf;
}

SCENARIO("Test the tester") {
  GIVEN("Buffer [A B C _ _]") {
    network_buffer b(5);
    std::string s{"ABC"};
    b << s;
    THEN("It has to be equal string 'ABC__'") {
      REQUIRE(b == "ABC__");
      REQUIRE(b == "[ABC__]");
      REQUIRE(b == "[ A B C _ _]");
      REQUIRE(b != "[CBA__]");
      REQUIRE(b != "[_ABC__]");
      REQUIRE(b != "ABC_");
    }
  }
}

SCENARIO("Constructors, pushing, swap, assign") {
  WHEN("Constructing buffers with () constructor") {
    network_buffer a;
    network_buffer b;
    THEN("They should be of equal (default) capacity and empty") {
      REQUIRE(a.capacity() == b.capacity());
      REQUIRE(a.size() == 0);
      REQUIRE(b.size() == 0);
    }
  }
  WHEN("Constructing buffers with (capacity) constructor") {
    network_buffer a(5);
    network_buffer b{6};
    THEN("Check the capacity and sizes") {
      REQUIRE(a.capacity() == 5);
      REQUIRE(b.capacity() == 6);
      REQUIRE(a.size() == 0);
      REQUIRE(b.size() == 0);
      WHEN("Pushing some data") {
        a << "123";
        b << "12345";
        THEN("The contents should match") {
          REQUIRE(a.size() == 3);
          REQUIRE(b.size() == 5);
          REQUIRE(a == "123__");
          REQUIRE(b == "12345_");
          THEN("Swap the values and check contents") {
            a.swap(b);
            REQUIRE(a == "12345_");
            REQUIRE(b == "123__");
          }
          THEN("Assign a to b") {
            b = a;
            REQUIRE(b.capacity() == 5);
            REQUIRE(a == "123__");
            REQUIRE(b == "123__");
          }
          THEN("Assign b to a") {
            a = b;
            REQUIRE(a.capacity() == 6);
            REQUIRE(a == "12345_");
            REQUIRE(b == "12345_");
          }
          THEN("Test popping") {
            a.pop_front(1);
            REQUIRE(a.size() == 2);
            REQUIRE(a.capacity() == 5);
            REQUIRE(a == "_23__");
          }
        }
      }
    }
  }
}

SCENARIO("Fragmented buffer case") {
  GIVEN("A buffer [1 2 3 4 5]") {
    network_buffer a(5);
    a << "12345";
    WHEN("Making it fragmented as [6 _ _ 4 5]") {
      a.pop_front(1);
      REQUIRE(a.head_size() == 4);
      REQUIRE(a.tail_size() == 0);
      REQUIRE(a == "_2345");
      a << "6";
      REQUIRE(a == "62345");
      REQUIRE(a.is_fragmented());
      a.pop_front(2);
      REQUIRE(a.size() == 3);
      REQUIRE(a.head_size() == 2);
      REQUIRE(a.tail_size() == 1);
      REQUIRE(a == "6 _ _ 4 5");
      REQUIRE(a.is_fragmented());
      THEN("Pop part of head to get [6 _ _ _ 5]") {
        a.pop_front(1);
        REQUIRE(a.size() == 2);
        REQUIRE(a.head_size() == 1);
        REQUIRE(a.tail_size() == 1);
        REQUIRE(a.is_fragmented());
        REQUIRE(a == "6 _ _ _ 5");
        
        THEN("Push to tail's end") {
          a << "7";
          REQUIRE(a == "6 7 _ _ 5");
          THEN("Defragment") {
            a.defragment();
            REQUIRE(a == "5 6 7 _ _");
          }
          THEN("Resize") {
            a.resize(4);
            REQUIRE(a == "5 6 7 _");
          }
          THEN("Pop past head end") {
            a.pop_front(2);
            REQUIRE(a.size() == 1);
            REQUIRE(a.head_size() == 1);
            REQUIRE(a.tail_size() == 0);
            REQUIRE(a == "_ 7 _ _ _");
          }
        }
        THEN("Pop single element that is at the end of raw buf") {
          a.pop_front(1);
          REQUIRE(a.size() == 1);
          REQUIRE(a.head_size() == 1);
          REQUIRE(a.tail_size() == 0);
          REQUIRE(a == "6 _ _ _ _");
        }
      }
      THEN("Pop exactly the size of the head to get [6 _ _ _ _]") {
        a.pop_front(2);
        REQUIRE(a.size() == 1);
        REQUIRE(a.is_fragmented() == false);
        REQUIRE(a.head_size() == 1);
        REQUIRE(a.tail_size() == 0);
      }
      THEN("Pop the whole buffer size") {
        a.pop_front(3);
        REQUIRE(a.size() == 0);
        REQUIRE(a.is_fragmented() == false);
        REQUIRE(a.head_size() == 0);
        REQUIRE(a.tail_size() == 0);
      }
    }
  }
}
