#pragma once

#include <type_traits>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>


namespace tamed {

    /**
     *  Fallback struct for a stream that does not
     *  support asynchronous handshaking
     */
    template <typename T, typename = void>
    struct is_async_tls_stream : std::false_type {};

    /**
     *  Structure matching on streams that do
     *  support asynchronous handshaking
     */
    template <typename T>
    struct is_async_tls_stream<T, std::void_t<
        // execute asynchronous handshake
        decltype(std::declval<T>().async_handshake(
            std::declval<boost::asio::ssl::stream_base::handshake_type>(),
            std::declval<void(*)(const boost::system::error_code&)>()
        )),

        // asynchronously terminate the tls envelope
        decltype(std::declval<T>().async_shutdown(
            std::declval<void(*)(const boost::system::error_code&)>()
        ))
    >> : std::true_type {};

    /**
     *  Value alias for the different traits
     */
    template <typename T>
    constexpr bool is_async_tls_stream_v = is_async_tls_stream<T>::value;

    /**
     *  Forward-declare the stream deducer
     *  without implementation (cannot be used)
     */
    template <typename... T>
    struct async_stream_traits;

    /**
     *  Deduce traits for a non-tls endpoint
     */
    template <typename endpoint_type>
    struct async_stream_traits<endpoint_type>
    {
        using protocol_type = typename endpoint_type::protocol_type;
        using socket_type   = typename protocol_type::socket;
        using stream_type   = typename protocol_type::socket;
        using acceptor_type = boost::asio::basic_socket_acceptor<endpoint_type>;
    };

    /**
     *  Deduce traits for a tls endpoint
     */
    template <typename endpoint_type>
    struct async_stream_traits<endpoint_type, boost::asio::ssl::context>
    {
        using protocol_type = typename endpoint_type::protocol_type;
        using socket_type   = typename protocol_type::socket;
        using stream_type   = boost::beast::ssl_stream<socket_type>;
        using acceptor_type = boost::asio::basic_socket_acceptor<endpoint_type>;
    };

}
