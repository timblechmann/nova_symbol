#include <nova/symbol/symbol.hpp>

#include <array>
#include <cstdint>
#include <mutex>

#ifdef __cpp_lib_memory_resource
#    include <memory_resource>
#else
#    include <boost/container/pmr/monotonic_buffer_resource.hpp>
#    include <boost/container/pmr/polymorphic_allocator.hpp>
#endif

#include <boost/intrusive/unordered_set.hpp>

#include <city.h>

namespace nova {
namespace detail {

//----------------------------------------------------------------------------------------------------------------------

struct symbol_data : boost::intrusive::unordered_set_base_hook<>
{
    symbol_data( const symbol_data& )            = delete;
    symbol_data( symbol_data&& )                 = delete;
    symbol_data& operator=( const symbol_data& ) = delete;
    symbol_data& operator=( symbol_data&& )      = delete;

    static constexpr bool pack_ownership_in_string_pointer = false; // allows better memory efficiency

    symbol_data( const std::string_view& v, uint64_t hash )
    {
        if constexpr ( pack_ownership_in_string_pointer ) {
            m_ptr  = reinterpret_cast< const char* >( std::intptr_t( v.data() ) | ownership_bitmask );
            m_size = v.size();
        } else {
            m_ptr  = v.data();
            m_size = v.size() | size_ownership_bitmask;
        }
        m_hash = hash;
    }

    symbol_data( const std::string_view& v, uint64_t hash, nova::string_data_in_persistent_memory_t )
    {
        m_ptr  = v.data();
        m_size = v.size();
        m_hash = hash;
    }

    explicit operator std::string_view() const
    {
        return std::string_view {
            str(),
            size(),
        };
    }

    bool owns_memory()
    {
        if constexpr ( pack_ownership_in_string_pointer ) {
            return std::intptr_t( m_ptr ) & ownership_bitmask;
        } else {
            return bool( m_size ^ size_ownership_bitmask );
        }
    }

    const char* str() const
    {
        if constexpr ( pack_ownership_in_string_pointer ) {
            return reinterpret_cast< const char* >( std::intptr_t( m_ptr ) & content_bitmask );
        } else {
            return m_ptr;
        }
    }

    uint64_t hash() const
    {
        return m_hash;
    }

    size_t size() const
    {
        if constexpr ( pack_ownership_in_string_pointer ) {
            return m_size;
        } else {
            return m_size & ~size_ownership_bitmask;
        }
    }

private:
    static constexpr auto ownership_bitmask = std::intptr_t( 1 );
    static constexpr auto content_bitmask   = std::intptr_t( -1 ) ^ ownership_bitmask;
    static constexpr size_t size_ownership_bitmask = sizeof( size_t ) == 8 ? ( size_t( 1 ) << 63 )
                                                                           : ( size_t( 1 ) << 31 );

    const char* m_ptr;
    size_t      m_size;
    uint64_t    m_hash;
};

//----------------------------------------------------------------------------------------------------------------------

namespace {

struct hasher
{
    uint64_t operator()( const std::string_view& sv )
    {
        return CityHash64( sv.data(), sv.size() );
    }

    uint64_t operator()( const nova::detail::symbol_data& data )
    {
        return data.hash();
    }
};

struct hasher_with_context
{
    uint64_t operator()( const std::string_view& sv ) const
    {
        return hash;
    }

    uint64_t operator()( const nova::detail::symbol_data& data ) const
    {
        return data.hash();
    }

    uint64_t hash;
};

struct compare
{
    bool operator()( const auto& lhs, const auto& rhs ) const
    {
        return std::string_view( lhs ) == std::string_view( rhs );
    }
};


struct symbol_table
{
#ifdef __cpp_lib_memory_resource
    using allocator_t               = std::pmr::polymorphic_allocator<>;
    using monotonic_buffer_resource = std::pmr::monotonic_buffer_resource;
#else
    using allocator_t               = boost::container::pmr::polymorphic_allocator< char >;
    using monotonic_buffer_resource = boost::container::pmr::monotonic_buffer_resource;
#endif

    static constexpr size_t string_alignment = symbol_data::pack_ownership_in_string_pointer ? 2 : 1;

public:
    ~symbol_table()
    {
        allocator_t allocator {
            &memory_resource,
        };

#ifdef __cpp_lib_memory_resource
        table.clear_and_dispose( [ & ]( symbol_data* d ) {
            if ( d->owns_memory() )
                allocator.deallocate_bytes( const_cast< void* >( reinterpret_cast< const void* >( d->str() ) ),
                                            d->size(),
                                            string_alignment );
            allocator.delete_object( d );
        } );
#else
        table.clear_and_dispose( [ & ]( symbol_data* d ) {
            if ( d->owns_memory() )
                allocator.deallocate( const_cast< char* >( d->str() ), d->size() );
            d->~symbol_data();
            allocator.deallocate( reinterpret_cast< char* >( d ), sizeof( symbol_data ) );
        } );
#endif
    }

    const symbol_data* gensym( const std::string_view& sv )
    {
        const uint64_t hash = hasher()( sv );

        std::lock_guard lock { mutex };

        auto found = table.find( sv, hasher_with_context { hash }, compare {} );
        if ( found != table.end() )
            return &*found;

        allocator_t allocator {
            &memory_resource,
        };

#ifdef __cpp_lib_memory_resource
        char* string_data = reinterpret_cast< char* >( allocator.allocate_bytes( sv.size(), string_alignment ) );
        std::memcpy( string_data, sv.data(), sv.size() );

        auto obj = allocator.new_object< symbol_data >(
            std::string_view {
                string_data,
                sv.size(),
            },
            hash );
#else
        char* string_data = allocator.allocate( sv.size() );
        std::memcpy( string_data, sv.data(), sv.size() );

        void* chunk = allocator.allocate( sizeof( symbol_data ) );
        auto  obj   = new ( chunk ) symbol_data(
            std::string_view {
                string_data,
                sv.size(),
            },
            hash );
#endif

        table.insert( *obj );
        return obj;
    }

    const symbol_data* gensym( const std::string_view& sv, nova::string_data_in_persistent_memory_t pm )
    {
        const uint64_t hash = hasher()( sv );

        std::lock_guard lock { mutex };

        auto found = table.find( sv, hasher_with_context { hash }, compare {} );
        if ( found != table.end() )
            return &*found;

        allocator_t allocator {
            &memory_resource,
        };

#ifdef __cpp_lib_memory_resource
        auto obj = allocator.new_object< symbol_data >( sv, hash, pm );
#else
        void* chunk = allocator.allocate( sizeof( symbol_data ) );
        auto  obj   = new ( chunk ) symbol_data( sv, hash, pm );
#endif

        table.insert( *obj );
        return obj;
    }

    static symbol_table& instance()
    {
        static symbol_table s_symboltable;
        return s_symboltable;
    }

private:
    std::mutex mutex;

    // hash table
    using hash_table = boost::intrusive::unordered_set< nova::detail::symbol_data,

                                                        boost::intrusive::hash< nova::detail::hasher >,
                                                        boost::intrusive::equal< nova::detail::compare >,

                                                        boost::intrusive::constant_time_size< false >,
                                                        boost::intrusive::power_2_buckets< true > >;

    static constexpr size_t number_of_buckets = 2048;

    hash_table::bucket_type buckets[ number_of_buckets ];
    hash_table              table {
        hash_table::bucket_traits { buckets, number_of_buckets },
    };

    // memory resource
    static constexpr size_t average_symbol_size            = 8;
    static constexpr size_t preallocated_number_of_symbols = 1024;
    static constexpr size_t allocator_overhead             = 1024;
    static constexpr size_t preallocated_memory
        = ( sizeof( symbol_data ) + average_symbol_size ) * preallocated_number_of_symbols + allocator_overhead;

    alignas( 128 ) std::array< const char*, preallocated_memory > buffer;
    monotonic_buffer_resource memory_resource {
        buffer.data(),
        buffer.size(),
    };
};

} // namespace

} // namespace detail

//----------------------------------------------------------------------------------------------------------------------

symbol::symbol( const std::string_view& sv )
{
    data = detail::symbol_table::instance().gensym( sv );
}

symbol::symbol( const std::string_view& sv, nova::string_data_in_persistent_memory_t pm )
{
    data = detail::symbol_table::instance().gensym( sv, pm );
}

symbol::operator std::string_view() const
{
    return std::string_view( *data );
}

std::string_view symbol::string_view() const
{
    return std::string_view( *data );
}

size_t symbol::size() const
{
    return data->size();
}

uint64_t symbol::hash() const
{
    return data->hash();
}

uint64_t symbol::s_hash( const std::string_view& sv )
{
    return detail::hasher()( sv );
}


//----------------------------------------------------------------------------------------------------------------------

} // namespace nova
