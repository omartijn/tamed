#pragma once

#include "listen_operation.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <router/table.h>
#include "enum_map.h"
#include "config.h"


namespace tamed {

    /**
     *  Define the server as a templated type
     *  unimplemented to use with with a config
     */
    template <typename>
    class server;

    /**
     *  Server implementation.
     *
     *  This class can be used with a config specialization, to
     *  customize certain behaviours of the server.
     */
    template <typename body_type, typename executor_type, boost::beast::http::verb... verbs>
    class server<config<body_type, executor_type, verbs...>>
    {
        public:
            using request_body_type = body_type;
            using request_type      = boost::beast::http::request<body_type>;
            using routing_table     = router::table<void(connection, request_type&&)>;
            using map_type          = enum_map<boost::beast::http::verb, routing_table, verbs...>;

            /**
             *  Constructor
             *
             *  @param  executor    The executor to use
             */
            server(executor_type executor) :
                _executor{ executor }
            {}

            /**
             *  Constructor
             *
             *  @param  io_context  The io context to use
             */
            template <typename X = executor_type, typename = std::enable_if_t<std::is_constructible_v<X, boost::asio::io_context::executor_type>>>
            server(boost::asio::io_context& io_context) :
                server{ io_context.get_executor() }
            {}

            /**
             *  Add an endpoint to be handled
             *
             *  @tparam callback    The callback to route to
             *  @param  method      The HTTP method to route
             *  @param  endpoint    The path to add
             */
            template <auto callback>
            std::enable_if_t<!std::is_member_function_pointer_v<decltype(callback)>>
            add(boost::beast::http::verb method, std::string_view endpoint)
            {
                // add the endpoint to the table
                _routers[method].template add<callback>(endpoint);
            }

            /**
             *  Add an endpoint to be handled
             *
             *  @tparam callback    The callback to route to
             *  @param  method      The HTTP method to route
             *  @param  endpoint    The path to add
             *  @param  instance    The instance to invoke the callback on
             */
            template <auto callback>
            std::enable_if_t<std::is_member_function_pointer_v<decltype(callback)>>
            add(boost::beast::http::verb method, std::string_view endpoint, typename router::function_traits<decltype(callback)>::member_type* instance)
            {
                // add the endpoint to the table
                _routers[method].template add<callback>(endpoint, instance);
            }

            /**
             *  Listen at the given endpoint
             *
             *  @param  endpoint    The endpoint to listen to
             *  @return The error code from the operation
             */
            template <typename endpoint_type>
            boost::system::error_code listen(const endpoint_type& endpoint)
            {
                // deduce the protocol type and the listener to create
                using protocol_type = typename endpoint_type::protocol_type;
                using listener_type = listen_operation<request_body_type, protocol_type, executor_type, map_type>;

                // create a listener, initialize it and return the result
                return listener_type{
                    _routers,
                    _executor
                }(endpoint);
            }

            /**
             *  Set a handler for endpoints that are not found
             *
             *  @tparam callback    The callback to route to
             */
            template <auto callback>
            std::enable_if_t<!std::is_member_function_pointer_v<decltype(callback)>>
            set_not_found()
            {
                // all routing tables get the handler
                for (std::size_t index{ 0 }; index < _routers.size(); ++index) {
                    // install handler on the table
                    _routers[index].template set_not_found<callback>();
                }
            }

            /**
             *  Set a handler for endpoints that are not found
             *
             *  @tparam callback    The callback to route to
             *  @param  instance    The instance to invoke the callback on
             */
            template <auto callback>
            std::enable_if_t<std::is_member_function_pointer_v<decltype(callback)>>
            set_not_found(typename router::function_traits<decltype(callback)>::member_type* instance)
            {
                // all routing tables get the handler
                for (std::size_t index{ 0 }; index < _routers.size(); ++index) {
                    // install handler on the table
                    _routers[index].template set_not_found<callback>(instance);
                }
            }

            /**
             *  Listen at the given endpoint
             *
             *  @param  endpoint    The endpoint to listen to
             *  @param  context     The TLS context for transport encryption
             *  @return The error code from the operation
             */
            template <typename endpoint_type>
            boost::system::error_code listen(const endpoint_type& endpoint, boost::asio::ssl::context& context)
            {
                // deduce the protocol type and the listener to create
                using protocol_type = typename endpoint_type::protocol_type;
                using listener_type = listen_operation<request_body_type, protocol_type, executor_type, map_type, boost::asio::ssl::context&>;

                // create a listener, initialize it and return the result
                return listener_type{
                    _routers,
                    _executor,
                    context
                }(endpoint);
            }
        private:
            executor_type   _executor;  // the executor to use
            map_type        _routers;   // the tables to route requests
    };

    /**
     *  Pre-defined server types for REST and WebDAV servers
     */
    using rest_server   = server<rest_config>;
    using webdav_server = server<webdav_config>;

}
