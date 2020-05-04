#pragma once

#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <router/table.h>
#include <utility>
#include <tuple>
#include "stream_traits.h"
#include "connection.h"


namespace tamed {

    /**
     *  Class for initiating an asynchronous
     *  listen operation.
     */
    template <typename body_type, typename protocol_type, typename executor_type, typename router_type, typename... arguments>
    class listen_operation
    {
        public:
            using request_body_type = body_type;
            using acceptor_type     = boost::asio::basic_socket_acceptor<protocol_type, executor_type>;
            using endpoint_type     = typename acceptor_type::endpoint_type;
            using socket_type       = boost::asio::basic_stream_socket<protocol_type, executor_type>;
            using stream_type       = typename async_stream_traits<endpoint_type, arguments...>::stream_type;

            /**
             *  Constructor
             *
             *  @param  router      The routing map to route the requests
             *  @param  executor    The executor for creating the acceptor and socket
             *  @param  parameters  Additional parameters for constructing the stream
             */
            listen_operation(router_type& router, executor_type executor, arguments&&... parameters) :
                _router{ router },
                _acceptor{ std::make_shared<acceptor_type>(executor) },
                _parameters{ std::forward<arguments>(parameters)... }
            {}

            /**
             *  Start listening for incoming connections
             *
             *  @param  endpoint    The endpoint to listen on
             *  @return The error code from the operation
             */
            boost::system::error_code operator()(const endpoint_type& endpoint) noexcept
            {
                // handle system errors
                try {
                    // open the acceptor, bind to the endpoint and start listening
                    _acceptor->open(endpoint.protocol());
                    _acceptor->set_option(boost::asio::socket_base::reuse_address{ true });
                    _acceptor->bind(endpoint);
                    _acceptor->listen(boost::asio::socket_base::max_listen_connections);
                } catch (const boost::system::system_error &error) {
                    // return the error code from the exception
                    return error.code();
                }

                // start listening for connections
                _acceptor->async_wait(boost::asio::socket_base::wait_read, *this);

                // no errors occured
                return {};
            }

            /**
             *  Invoke the handler
             *
             *  @param  ec  The error code from the operation
             */
            void operator()(const boost::system::error_code& ec) noexcept
            {
                // check if the operation was aborted or resulted in an error
                if (ec == boost::asio::error::operation_aborted) {
                    // log that the listening is now aborted
                    std::cerr << "Listen operation aborted" << std::endl;
                } else if (ec) {
                    // log the error that occured
                    std::cerr << "Listening failed: " << ec.message() << std::endl;
                } else {
                    // create a new connection handler
                    connection handler;

                    // accept the incoming connection
                    std::apply(&connection::template accept<body_type, router_type, acceptor_type, arguments...>, std::tuple_cat(
                        std::forward_as_tuple(&handler, _router, *_acceptor),
                        _parameters
                    ));

                    // continue listening for new connections
                    _acceptor->async_wait(boost::asio::socket_base::wait_read, *this);
                }
            }
        private:
            router_type&                    _router;        // the router map to route requests
            std::shared_ptr<acceptor_type>  _acceptor;      // acceptor for incoming connections
            std::tuple<arguments...>        _parameters;    // additional parameters for connections
    };

}
