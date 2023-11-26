#include <catch2/catch_all.hpp>

#include <nova/symbol/symbol.hpp>

#include <unordered_set>

TEST_CASE( "symbol" )
{
    using namespace nova::symbol_literals;

    nova::symbol a( "a" );
    nova::symbol a_2( std::string_view { "a" } );
    nova::symbol a_3 = "a"_sym;
    nova::symbol a_4 = NOVA_SYMBOL( "a" );

    CHECK( a == a_2 );
    CHECK( a == a_3 );
    CHECK( a == a_4 );

    nova::symbol b( "b" );

    CHECK( b != a_2 );
    CHECK( b != a_3 );
    CHECK( b != a_4 );

    CHECK( a == "a" );
    CHECK( a == std::string_view( "a" ) );
    CHECK( a == std::string( "a" ) );

    SECTION( "containers" )
    {
        std::unordered_set< nova::symbol > unordered_set;
        unordered_set.insert( a );
        CHECK( unordered_set.contains( a ) );
        CHECK( unordered_set.contains( a_2 ) );
        CHECK( !unordered_set.contains( b ) );

        std::set< nova::symbol > set;
        set.insert( a );
        CHECK( set.contains( a ) );
        CHECK( set.contains( a_2 ) );
        CHECK( !set.contains( b ) );
    }

    SECTION( "containers with lexical comparison" )
    {
        std::set< nova::symbol, nova::symbol_support::lexical_less > set;
        set.insert( a );
        CHECK( set.contains( a ) );
        CHECK( set.contains( a_2 ) );
        CHECK( !set.contains( b ) );
        CHECK( set.contains( "a" ) );
        CHECK( !set.contains( "b" ) );

        std::unordered_set< nova::symbol, nova::symbol_support::lexical_hash, nova::symbol_support::lexical_equal_to >
            unordered_set;
        unordered_set.insert( a );
        CHECK( unordered_set.contains( a ) );
        CHECK( unordered_set.contains( a_2 ) );
        CHECK( !unordered_set.contains( b ) );
        CHECK( unordered_set.contains( "a" ) );
        CHECK( !unordered_set.contains( "b" ) );
    }
}


#ifdef __cpp_lib_format

TEST_CASE( "format" )
{
    nova::symbol a( "a" );
    CHECK( std::format( "{}", a ) == "a" );
}

#endif
