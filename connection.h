#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include "derived_optional.h"
#include "message_data_source.h"
#include "send_data.h"
#include "stream_traits.h"
#include "connection_data.h"


namespace tamed {

    /**
     *  Class to handle the requests from a single connection
     */
    class connection
    {
        public:
            /**
             *  Default constructor
             */
            connection() = default;

            /**
             *  Constructor
             *
             *  @param  data    The existing state to work with
             */
            connection(std::shared_ptr<connection_data> data) :
                _data{ std::move(data) }
            {}

            /**
             *  Begin processing requests
             *
             *  @param  router      The table to route requests
             *  @param  acceptor    The acceptor for accepting the incoming connection
             *  @return The error code from accepting the connection
             */
            template <typename body_type, typename router_type, typename acceptor_type, typename... arguments>
            boost::system::error_code accept(router_type& router, acceptor_type& acceptor, arguments&&... parameters) noexcept
            {
                using executor_type = typename acceptor_type::executor_type;
                using endpoint_type = typename acceptor_type::endpoint_type;
                using stream_type   = typename async_stream_traits<endpoint_type, arguments...>::stream_type;
                using data_type     = connection_data_impl<router_type, body_type, stream_type, executor_type>;

                // create the connection data
                auto impl   = std::make_shared<data_type>(router, acceptor.get_executor(), std::forward<arguments>(parameters)...);
                _data       = impl;

                // the error code from the operation
                boost::system::error_code ec;

                // accept the incoming connection
                acceptor.accept(impl->socket.lowest_layer(), ec);

                // check whether the socket was accepted successfully
                if (ec != boost::system::error_code{}) {
                    // cannot continue without an open socket
                    return ec;
                }

                // do we have a stream with support for asynchronous handshakes?
                if constexpr (is_async_tls_stream_v<stream_type>) {
                    // initiate the SSL handshake
                    impl->socket.async_handshake(boost::asio::ssl::stream_base::server, *this);
                } else {
                    // no handshake required, proceed directly to reading the request
                    operator()(boost::system::error_code{});
                }

                // no error occured
                return {};
            }

            /**
             *  Send a response message
             *
             *  @param  message     The message to send
             */
            template <typename response_body_type>
            void send(boost::beast::http::response<response_body_type> message) noexcept
            {
                // store the message inside the serializer and start writing
                _data->response.template emplace<message_data_source<response_body_type>>(std::move(message));
                _data->write_response();
            }

            /**
             *  Read or write request data
             *
             *  @param  ec          The error code from the operation
             */
            void operator()(const boost::system::error_code& ec, std::size_t = 0) noexcept
            {
                // did an error occur?
                if (ec != boost::system::error_code{}) {
                    // log the error and abort
                    std::cerr << ec.message() << std::endl;
                    return;
                }

                // check current state
                switch (_data->state)
                {
                    case state::idle:
                        // read in the request
                        return _data->read_request();
                    case state::reading:
                        _data->route_request();
                        break;
                    case state::writing:
                        // do we need to continue on to the next request
                        if (!_data->close) {
                            return _data->read_request();
                        }
                        break;
                }
            }
        private:
            std::shared_ptr<connection_data>    _data;  // connection state;
    };

}

#include "connection_data.inl"
