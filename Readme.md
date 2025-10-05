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
    explicit symbol( const std::string_view&, string_data_in_persistent_memory_t ); // expects string data to be persistent

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
nova::symbol sym  = "symbol"_sym;           // user-defined literal
nova::symbol sym2 = nova::symbol{"symbol"}; // expensive, due to hash table lookup/insertion. user-defined literal is preferred

// (fast) symbol comparision (comparing pointers):
bool equal = sym == sym2;
bool less  = sym < sym2; // ordering is stable, but not persistent across restarting the process

// (slower) string comparison
bool equal = sym == "symbol"; // comparing the hash value

// support structs:
struct nova::symbol_support::lexical_less;     // transparent lexical comparison
struct nova::symbol_support::lexical_hash;     // transparent hasher
struct nova::symbol_support::lexical_equal_to; // transparent lexical equality
struct nova::symbol_support::hash_less;        // transparent comparison using hash value

// formatting
auto string = std::format("{}", sym);
auto string = fmt::format("{}", sym);
```

# Dependencies
* C++20 (ideally with pmr, boost.container can be used for pmr emulation)
* Boost (for instrusive hash table and pmr emulation)
* Cityhash (for string hashing)
* Catch2 (for unit tests)

# Building

Integrate Conan via
```
cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/conan_provider.cmake [...] -DNOVA_SYMBOL_USE_CPM=OFF
```

Alternatively, use CPM.cmake (preferred) via
```
cmake -DNOVA_SYMBOL_USE_CPM=ON [...]
```
