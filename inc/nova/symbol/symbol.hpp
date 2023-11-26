#pragma once

// Copyright (c) 2023 Tim Blechmann
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// As a non-binding request, please use this code responsibly and ethically.


#include <cstdint>
#include <string_view>
#include <version>

#ifdef __cpp_lib_format
#    include <format>
#endif

#include <boost/hana/string.hpp>

namespace nova {

//----------------------------------------------------------------------------------------------------------------------

namespace detail {
struct symbol_data;
} // namespace detail

//----------------------------------------------------------------------------------------------------------------------

struct string_data_in_persistent_memory_t
{};

constexpr inline string_data_in_persistent_memory_t string_data_in_persistent_memory;

//----------------------------------------------------------------------------------------------------------------------

struct symbol
{
    explicit symbol( const std::string_view& );
    explicit symbol( const std::string_view&, string_data_in_persistent_memory_t );

    symbol( symbol&& )                 = default;
    symbol& operator=( const symbol& ) = default;
    symbol& operator=( symbol&& )      = default;
    symbol( const symbol& )            = default;

    explicit operator std::string_view() const;
    auto     operator<=>( const symbol& ) const = default; // compare by pointer

    std::string_view string_view() const;
    size_t           size() const;
    uint64_t         hash() const;
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
DEFINE_OPERATOR( < )
DEFINE_OPERATOR( <= )
DEFINE_OPERATOR( > )
DEFINE_OPERATOR( >= )
DEFINE_OPERATOR( <=> )

#undef DEFINE_OPERATOR

//----------------------------------------------------------------------------------------------------------------------

namespace symbol_support {

struct lexical_less
{
    using is_transparent = std::true_type;

    bool operator()( const auto& lhs, const auto& rhs ) const
    {
        return std::string_view( lhs ) < std::string_view( rhs );
    }
};

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

namespace detail {

template < char... s >
constexpr inline char const string_storage[ sizeof...( s ) ] = { s... };

template < char... s >
symbol make_symbol()
{
    static const symbol singleton = symbol( std::string_view( detail::string_storage< s... >, sizeof...( s ) ),
                                            string_data_in_persistent_memory );
    return singleton;
}

template < typename BoostHanaString >
symbol make_symbol_from_hana_string( BoostHanaString )
{
    static const symbol singleton = symbol {
        std::string_view { BoostHanaString::c_str() },
        string_data_in_persistent_memory,
    };
    return singleton;
}


} // namespace detail

namespace symbol_literals {

#ifdef __GNUC__

#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"

template < typename Char, Char... s >
inline symbol operator"" _sym()
{
    return nova::detail::make_symbol< s... >();
}

#    pragma clang diagnostic pop

#else

inline symbol operator""_sym( const char* literal, size_t size )
{
    return symbol(
        std::string_view {
            literal,
            size,
        },
        nova::string_data_in_persistent_memory );
}

#endif

} // namespace symbol_literals

//----------------------------------------------------------------------------------------------------------------------


#define NOVA_SYMBOL( s ) nova::detail::make_symbol_from_hana_string( BOOST_HANA_STRING( s ) )

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
struct std::formatter< nova::symbol, char > : std::formatter< std::string_view >
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
