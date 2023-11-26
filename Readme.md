# Nova symbol

Symbols are hashed strings, which are found in many programming languages like lisp, smalltalk (or derived/inspired 
languages):

* immutable (flyweight design pattern)
* small (`sizeof(void*)`)
* fast to compare and copy (pointer operations)
* slow-ish to construct (require lookup/insert into hash table)


## Example

```c++
// signature
struct nova::symbol
{
    explicit symbol( const std::string_view& );
    explicit symbol( const std::string_view&, string_data_in_persistent_memory_t ); // expects string data to be persisten

    symbol( symbol&& )                 = default;
    symbol& operator=( const symbol& ) = default;
    symbol& operator=( symbol&& )      = default;
    symbol( const symbol& )            = default;

    explicit operator std::string_view() const;
    auto     operator<=>( const symbol& ) const = default;

    std::string_view string_view() const;
    size_t           size() const;
    uint64_t         hash() const;
    static uint64_t  s_hash( const std::string_view& );
};

// construction:
using namespace nova::symbol_literals;      // for _sym
nova::symbol sym  = "symbol"_sym;           // clang/gcc implement ""_sym via non-standard extensions, making it a singleton
nova::symbol sym2 = NOVA_SYMBOL("symbol2"); // singleton-based factory (preferred for supporting msvc)
nova::symbol sym3 = nova::symbol{"symbol"}; // expensive, due to hash table lookup/insertion

// (fast) symbol comparision (comparing pointers):
bool equal = sym == sym2;
bool less  = sym < sym2; // ordering is stable, but not persistent across restarting the process

// (slower) string comparison
bool equal = sym == "symbol"; // comparing the string representation of the symbol

// support structs:
struct nova::symbol_support::lexical_less;     // transparent lexical comparison
struct nova::symbol_support::lexical_hash;     // transparent hasher
struct nova::symbol_support::lexical_equal_to; // transparent lexical equality

// formatting
auto string = std::format("{}", sym);
auto string = fmt::format("{}", sym);
```

# Dependencies
* C++20 (ideally with pmr, boost.container can be used for pmr emulation)
* Boost (for instrusive hash table, compile-time strings and pmr emulation)
* Cityhash (for string hashing)
* Catch2 (for unit tests)

# Building

Integrate Conan via
```
cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/conan_provider.cmake [...]
```
