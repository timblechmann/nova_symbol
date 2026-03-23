# Nova symbol

Symbols are interned, hashed strings, commonly found in programming languages like Lisp, SmallTalk (or derived/inspired
languages such as Ruby or SuperCollider):

* Immutable (flyweight design pattern)
* Compact (`sizeof(void*)`)
* Fast to copy and compare (pointer operations)
* Pre-hashed (hash stored alongside the symbol)
* Costly to construct from `std::string_view` (requires hashing and interning)


## Example

```c++
// signature
namespace nova {

struct symbol
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

} // namespace nova

// construction:
using namespace nova::symbol_literals;      // for _sym
nova::symbol sym  = "symbol"_sym;           // user-defined literal
nova::symbol sym2 = nova::symbol{"symbol"}; // expensive, due to hash table lookup/insertion. user-defined literal is preferred

// (fast) symbol comparision (comparing pointers):
bool equal = sym == sym2;
bool less  = sym < sym2; // ordering is stable, but not persistent across restarting the process

// (slower) string comparison
bool equal = sym == "symbol"; // comparing the string value

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
* rapidhash (for string hashing)
* Catch2 (for unit tests)

# Building

```
cmake -B build
cmake --build build
ctest --test-dir build
```

## License

MIT — see [LICENSE](LICENSE). As a non-binding request, please use this code responsibly and ethically.
