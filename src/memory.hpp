#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <memory>
#include <type_traits>
#include <sstream>

void* lt_allocate(size_t n);

template <class T>
struct lt_allocator {
    static_assert(std::is_same<T, char>::value);
    
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using propagate_on_container_move_assignment = std::false_type;
    using is_always_equal = std::true_type;

    constexpr lt_allocator() noexcept {};

    inline pointer allocate(size_type n) { return static_cast<pointer>(lt_allocate(n)); }
    constexpr void deallocate(pointer p, size_type n) noexcept {} // noop
    constexpr bool operator==(const lt_allocator&) const noexcept { return true; }
    constexpr bool operator!=(const lt_allocator&) const noexcept { return false; }
};

using lt_stringsteam = std::basic_stringstream<char, std::char_traits<char>, lt_allocator<char>>;
using lt_string = std::basic_string<char, std::char_traits<char>, lt_allocator<char>>;
using lt_char_v = std::vector<char, lt_allocator<char>>;

#endif // MEMORY_HPP