#pragma once
#include <cpslib/string.h>
#include <cpslib/types.hpp>
#include <cpslib/system.h>
#include <cpslib/memory.hpp>
#include <cpslib/print.hpp>

namespace cpsio {
  /**
   * @brief Count the length of null terminated string (excluding the null terminated symbol)
   * Non-null terminated string need to be passed here, 
   * Otherwise it will not give the right length
   * @param cstr - null terminated string
   */
  inline size_t cstrlen(const char* cstr) {
    size_t len = 0;
    while(*cstr != '\0') {
      len++;
      cstr++;
    }
    return len;
  }
    
  class string {

  private:
    size_t size; // size of the string
    char* data; // underlying data
    bool own_memory; // true if the object is responsible to clean the memory
    uint32_t* refcount; // shared reference count to the underlying data

    // Release data if no more string reference to it
    void release_data_if_needed() {
      if (own_memory && refcount != nullptr) {
        (*refcount)--;
        if (*refcount == 0) {
          free(data);
        }
      }
    }

  public:
    /**
     * Default constructor
     */
    string() : size(0), data(nullptr), own_memory(false), refcount(nullptr) {
    }

    /**
     * Constructor to create string with reserved space
     * @param s size to be reserved (in number o)
     */
    string(size_t s) : size(s) {
      if (s == 0) {
        data = nullptr;
        own_memory = false;
        refcount = nullptr;
      } else {
        data = (char *)malloc(s * sizeof(char));
        own_memory = true;
        refcount = (uint32_t*)malloc(sizeof(uint32_t));
        *refcount = 1;
      }
    }

    /**
     * Constructor to create string with given data and size
     * @param d    data
     * @param s    size of the string (in number of bytes)
     * @param copy true to have the data copied and owned by the object
     */
    string(char* d, size_t s, bool copy) {
      assign(d, s, copy);
    }

    // Copy constructor
    string(const string& obj) {
      if (this != &obj) {
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
    }

    /**
     * @brief Constructor for string literal
     * Non-null terminated string need to be passed here, 
     * Otherwise it will have extraneous data
     * @param cstr - null terminated string
     */
    string(const char* cstr) {
      size = cstrlen(cstr) + 1;
      data = (char *)malloc(size * sizeof(char));
      memcpy(data, cstr, size * sizeof(char));
      own_memory = true;
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;
    }

    // Destructor
    ~string() {
      release_data_if_needed();
    }

    // Get size of the string (in number of bytes)
    const size_t get_size() const {
      return size;
    }

    // Get the underlying data of the string
    const char* get_data() const {
      return data;
    }

    // Check if it owns memory
    const bool is_own_memory() const {
      return own_memory;
    }

    // Get the ref count
    const uint32_t get_refcount() const {
      return *refcount;
    }

    /**
     * Assign string with new data and size
     * @param  d    data
     * @param  s    size (in number of bytes)
     * @param  copy true to have the data copied and owned by the object
     * @return      the current string
     */
    string& assign(char* d, size_t s, bool copy) {
      if (s == 0) {
        clear();
      } else {
        release_data_if_needed();
        if (copy) {
          data = (char *)malloc(s * sizeof(char));
          memcpy(data, d, s * sizeof(char));
          own_memory = true;
          refcount = (uint32_t*)malloc(sizeof(uint32_t));
          *refcount = 1;
        } else {
          data = d;
          own_memory = false;
          refcount = nullptr;
        }
        size = s;
      }

      return *this;
    }
    
    /**
     * Clear the content of the string
     */
    void clear() {
      release_data_if_needed();
      data = nullptr;
      size = 0;
      own_memory = false;
      refcount = nullptr;
    }

    /**
     * Create substring from current string
     * @param  offset      offset from the current string's data
     * @param  substr_size size of the substring
     * @param  copy        true to have the data copied and owned by the object
     * @return             substring of the current string
     */
    string substr(size_t offset, size_t substr_size, bool copy) {
      assert((offset < size) && (offset + substr_size < size), "out of bound");
      return string(data + offset, substr_size, copy);
    }

    char operator [] (const size_t index) {
      assert(index < size, "index out of bound");
      return *(data + index);
    }

    // Assignment operator
    string& operator = (const string& obj) {
      if (this != &obj) {
        release_data_if_needed();
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
      return *this;
    }

    /**
     * @brief Assignment operator for string literal
     * Non-null terminated string need to be passed here, 
     * Otherwise it will have extraneous data
     * @param cstr - null terminated string
     */
    string& operator = (const char* cstr) {
        release_data_if_needed();
        size = cstrlen(cstr) + 1;
        data = (char *)malloc(size * sizeof(char));
        memcpy(data, cstr, size * sizeof(char));
        own_memory = true;
        refcount = (uint32_t*)malloc(sizeof(uint32_t));
        *refcount = 1;
        return *this;
    }

    string& operator += (const string& str){
      assert((size + str.size > size) && (size + str.size > str.size), "overflow");

      char* new_data;
      size_t new_size;
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string, remove the \0 when concatenates
        new_size = size - 1 + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, (size - 1) * sizeof(char));
        memcpy(new_data + size - 1, str.data, str.size * sizeof(char));
      } else {
        new_size = size + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, size * sizeof(char));
        memcpy(new_data + size, str.data, str.size * sizeof(char));
      }

      // Release old data
      release_data_if_needed();
      // Assign new data
      data = new_data;

      size = new_size;
      own_memory = true;
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;

      return *this;
    }

    // Compare two strings
    // Return an integral value indicating the relationship between strings
    //   >0 if the first string is greater than the second string
    //   0 if both strings are equal
    //   <0 if the first string is smaller than the second string
    // The return value also represents the difference between the first character that doesn't match of the two strings
    int32_t compare(const string& str) const {
      int32_t result;
      if (size == str.size) {
        result = memcmp(data, str.data, size);
      } else if (size < str.size) {
        result = memcmp(data, str.data, size);
        if (result == 0) {
          // String is equal up to size of the shorter string, return the difference in byte of the next character
          result = 0 - (unsigned char)str.data[size];
        }
      } else if (size > str.size) {
        result = memcmp(data, str.data, str.size);
        if (result == 0) {
          // String is equal up to size of the shorter string, return the difference in byte of the next character
          result = (unsigned char)data[str.size];
        }
      }
      return result;
    }

    friend bool operator < (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) < 0;
    }

    friend bool operator > (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) > 0;
    }

    friend bool operator == (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) == 0;
    }

    friend bool operator != (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) != 0;
    }

    friend string operator + (string lhs, const string& rhs) {
      return lhs += rhs;
    }

    void print() const {
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string
        prints(data);
      } else {
        // Non null terminated string
        // We need to specify the size of string so it knows where to stop
        prints_l(data, size);
      }
   }
  };

}
