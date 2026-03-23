#pragma once

// SPDX-FileCopyrightText: 2023 Tim Blechmann
// SPDX-License-Identifier: MIT
//
// As a non-binding request, please use this code responsibly and ethically.

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <version>

#ifdef __cpp_lib_format
#    include <format>
#endif

namespace nova {

//----------------------------------------------------------------------------------------------------------------------

namespace detail {
struct symbol_data;
} // namespace detail

//----------------------------------------------------------------------------------------------------------------------

/// @brief Tag type to indicate the string data passed to `symbol` has persistent lifetime.
struct string_data_in_persistent_memory_t
{};

/// @brief Tag value for `string_data_in_persistent_memory_t`.
constexpr inline string_data_in_persistent_memory_t string_data_in_persistent_memory;

//----------------------------------------------------------------------------------------------------------------------

/// @brief Interned string with O(1) copy and comparison.
///
/// Symbols are flyweight immutable strings: equal strings share the same backing storage,
/// so comparison and copying are pointer operations. Construction requires a hash-table
/// lookup and is therefore slower.
struct symbol
{
    /// @brief Interns `sv`, copying the string data into internal storage.
    explicit symbol( const std::string_view& sv );
    /// @brief Interns `sv` without copying; the caller guarantees the data outlives the process.
    explicit symbol( const std::string_view& sv, string_data_in_persistent_memory_t );

    symbol( symbol&& )                 = default;
    symbol& operator=( const symbol& ) = default;
    symbol& operator=( symbol&& )      = default;
    symbol( const symbol& )            = default;

    /// @brief Returns the interned string as a `string_view`.
    explicit operator std::string_view() const;
    /// @brief Pointer-based comparison; ordering is stable within a process run.
    auto     operator<=>( const symbol& ) const = default; // compare by pointer

    /// @brief Returns the interned string as a `string_view`.
    std::string_view string_view() const;
    /// @brief Returns the length of the string in bytes.
    size_t           size() const;
    /// @brief Returns the precomputed hash of this symbol.
    uint64_t         hash() const;
    /// @brief Computes the hash of an arbitrary string view (same algorithm as symbol internment).
    /// @note The hash function is an implementation detail and not guaranteed to be stable across library versions or platforms.
    static uint64_t  s_hash( const std::string_view& );

private:
    const detail::symbol_data* data;
};

//----------------------------------------------------------------------------------------------------------------------

#define DEFINE_OPERATOR( OP )                                   \
                                                                \
    inline auto operator OP( std::string_view lhs, symbol rhs ) \
    {                                                           \
        return lhs OP std::string_view { rhs };                 \
    }                                                           \
                                                                \
    inline auto operator OP( symbol lhs, std::string_view rhs ) \
    {                                                           \
        return std::string_view { lhs } OP rhs;                 \
    }

DEFINE_OPERATOR( == )
DEFINE_OPERATOR( != )

#undef DEFINE_OPERATOR

//----------------------------------------------------------------------------------------------------------------------

namespace symbol_support {

/// @brief Transparent comparator ordering by hash value; suitable for `std::set`/`std::map`.
struct hash_less
{
    using is_transparent = std::true_type;

    static uint64_t hash( const std::string_view& arg )
    {
        return symbol::s_hash( arg );
    }
    static uint64_t hash( const symbol& arg )
    {
        return arg.hash();
    }

    bool operator()( const auto& lhs, const auto& rhs ) const
    {
        auto hash_compare = hash( lhs ) <=> hash( rhs );

        if ( hash_compare == std::strong_ordering::less )
            return true;
        if ( hash_compare == std::strong_ordering::greater )
            return false;

        return std::string_view( lhs ) < std::string_view( rhs );
    }
};

/// @brief Transparent comparator using lexical (string) ordering; suitable for `std::set`/`std::map`.
struct lexical_less
{
    using is_transparent = std::true_type;

    bool operator()( const auto& lhs, const auto& rhs ) const
    {
        return std::string_view( lhs ) < std::string_view( rhs );
    }
    uint64_t operator()( nova::symbol lhs, nova::symbol rhs ) const
    {
        if ( lhs == rhs )
            return false;
        return std::string_view( lhs ) < std::string_view( rhs );
    }
};

/// @brief Transparent hasher using the same algorithm as symbol internment; for use with `std::unordered_*`.
struct lexical_hash
{
    using is_transparent = std::true_type;

    uint64_t operator()( const auto& arg ) const
    {
        return symbol::s_hash( std::string_view( arg ) );
    }
    uint64_t operator()( nova::symbol sym ) const
    {
        return sym.hash();
    }
};

/// @brief Transparent equality predicate using lexical (string) comparison; for use with `std::unordered_*`.
struct lexical_equal_to
{
    using is_transparent = std::true_type;

    uint64_t operator()( const auto& lhs, const auto& rhs ) const
    {
        return std::string_view( lhs ) == std::string_view( rhs );
    }

    uint64_t operator()( nova::symbol lhs, nova::symbol rhs ) const
    {
        return lhs == rhs;
    }
};

} // namespace symbol_support

//----------------------------------------------------------------------------------------------------------------------

namespace symbol_literals {

namespace detail {

template < size_t N >
struct literal_storage
{
    // NOLINTNEXTLINE(google-explicit-constructor)
    consteval literal_storage( const char ( &str )[ N ] )
    {
        std::copy_n( str, N, data.begin() );
    }
    std::array< char, N > data;
};

} // namespace detail

/// @brief User-defined literal; returns a `symbol` for a compile-time string constant.
/// @code
///   using namespace nova::symbol_literals;
///   nova::symbol s = "foo"_sym;
/// @endcode
template < detail::literal_storage S >
inline symbol operator""_sym() noexcept
{
    static const symbol singleton = symbol {
        std::string_view {
            S.data.data(),
            S.data.size() - 1,
        },
        string_data_in_persistent_memory,
    };
    return singleton;
}

} // namespace symbol_literals

//----------------------------------------------------------------------------------------------------------------------

} // namespace nova

//----------------------------------------------------------------------------------------------------------------------

#ifdef __cpp_lib_format

template <>
struct std::formatter< nova::symbol, char > : std::formatter< std::string_view >
{
    template < typename FormatContext >
    auto format( const nova::symbol& symbol, FormatContext& ctx ) const -> decltype( ctx.out() )
    {
        return std::formatter< std::string_view >::format( std::string_view( symbol ), ctx );
    }
};

#endif

#if __has_include( <fmt/format.h>)

#    include <fmt/format.h>

template <>
struct fmt::formatter< nova::symbol, char > : fmt::formatter< std::string_view >
{
    template < typename FormatContext >
    auto format( const nova::symbol& symbol, FormatContext& ctx ) const -> decltype( ctx.out() )
    {
        return fmt::formatter< std::string_view >::format( std::string_view( symbol ), ctx );
    }
};

#endif

//----------------------------------------------------------------------------------------------------------------------

template <>
struct std::hash< nova::symbol >
{
    size_t operator()( nova::symbol sym ) const
    {
        return size_t( sym.hash() );
    }
};
