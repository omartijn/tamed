#pragma once

#include "connection.h"
#include "read_operation.h"
#include "write_operation.h"


namespace tamed {

    /**
     *  Retrieve the executor
     *
     *  @return The executor associated with the connection
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    executor_type connection_data_impl<router_type, body_type, stream_type, executor_type>::get_executor() noexcept
    {
        return socket.get_executor();
    }

    /**
     *  Accept an incoming connection
     *
     *  @param  acceptor    The acceptor to that has an incoming connection waiting
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    template <typename acceptor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::accept(acceptor_type& acceptor) noexcept
    {
        // the error code from the operation
        boost::system::error_code ec;

        // accept the incoming connection
        acceptor.accept(socket.lowest_layer(), ec);

        // check whether the socket was accepted successfully
        if (ec != boost::system::error_code{}) {
            // cannot continue without an open socket
            std::cerr << "Error during socket accept: " << ec.message() << std::endl;
            return;
        }

        // do we have a stream with support for asynchronous handshakes?
        if constexpr (is_async_tls_stream_v<stream_type>) {
            // initiate the SSL handshake
            socket.async_handshake(boost::asio::ssl::stream_base::server, handshake_operation{ this->shared_from_this() });
        } else {
            // no handshake required, proceed directly to reading the request
            read_request();
        }
    }

    /**
     *  Read request data
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::read_request() noexcept
    {
        // read from the socket into the request
        boost::beast::http::async_read(socket, buffer, this->request, read_operation{ this->shared_from_this() });
    }

    /**
     *  Route the request to registered
     *  callbacks
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::route_request() noexcept
    {
        // do we need to close the connection after writing
        close = request.need_eof();

        try {
            // extract the target the request goes to
            std::string_view target{ request.target().data(), request.target().size() };

            // handle the processed request
            router[request.method()].route(target, connection{ this->shared_from_this() }, std::move(request));
        } catch (const std::out_of_range&) {
            // the connection to send over and the response to send
            connection                                                      connection  { this->shared_from_this()                  };
            boost::beast::http::response<boost::beast::http::string_body>   response    { boost::beast::http::status::not_found, 11 };

            // no handler was installed for this path
            response.body().assign("The requested resource was not found on this server");
            connection.send(std::move(response));
        }

        // reset the request
        request = request_type{};
    }

    /**
     *  Write response data
     *
     *  @param  response    The response message to write
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::write_response(data_source& response) noexcept
    {
        // start sending the response over the stream
        async_send_data(socket, response, write_operation{ this->shared_from_this() });
    }

}
