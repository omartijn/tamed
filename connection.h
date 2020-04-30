#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include "derived_optional.h"
#include "message_data_source.h"
#include "send_data.h"
#include "stream_traits.h"


namespace tamed {

    /**
     *  Class to handle the requests from a single connection
     */
    template <class body_type>
    class connection
    {
        public:
            using request_body_type = body_type;
            using request_type      = boost::beast::http::request<request_body_type>;
            using connection_type   = connection<body_type>;
            using routing_table     = router::table<void(connection_type, request_type&&)>;

            /**
             *  Constructor
             *
             *  @param  router      The table to route requests
             */
            connection(routing_table& router) :
                _router{ router }
            {}


            /**
             *  Begin processing requests
             *
             *  @param  acceptor    The acceptor for accepting the incoming connection
             *  @return The error code from accepting the connection
             */
            template <typename acceptor_type, typename... arguments>
            boost::system::error_code accept(acceptor_type& acceptor, arguments&&... parameters) noexcept
            {
                using executor_type = typename acceptor_type::executor_type;
                using endpoint_type = typename acceptor_type::endpoint_type;
                using stream_type   = typename async_stream_traits<endpoint_type, arguments...>::stream_type;

                // create the connection data
                auto impl   = std::make_shared<data_impl<stream_type, executor_type>>(acceptor.get_executor(), std::forward<arguments>(parameters)...);
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
                    _data->socket.async_handshake(boost::asio::ssl::stream_base::server, *this);
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
            template <typename response_body_type, typename fields_type>
            void send(boost::beast::http::response<response_body_type, fields_type> message) noexcept
            {
                // store the message inside the serializer and start writing
                _data->response.template emplace<message_data_source<body_type>>(std::move(message));
                _data->write_response(*this);
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
                        return _data->read_request(*this);
                    case state::reading:
                        // do we need to close the connection after writing
                        _data->close = _data->request.need_eof();

                        try {
                            // the request target is stored in a boost::string_view, while we need
                            // to use an std::string_view for the routing table
                            std::string_view target{ _data->request.target().data(), _data->request.target().size() };

                            // handle the processed request
                            _router.route(target, *this, std::move(_data->request));
                        } catch (const std::out_of_range&) {
                            // no handler was installed for this path
                            boost::beast::http::response<boost::beast::http::string_body>   response{ boost::beast::http::status::not_found, 11 };
                            response.body().assign("The requested resource was not found on this server");
                            send(std::move(response));
                        }
                        break;
                    case state::writing:
                        // do we need to continue on to the next request
                        if (!_data->close) {
                            return _data->read_request(*this);
                        }
                        break;
                }
            }
        private:
            /**
             *  The current connection state
             */
            enum class state
            {
                idle,
                reading,
                writing
            };

            /**
             *  The data members to keep
             *  between handler callbacks
             */
            class data
            {
                public:
                    /**
                     *  Destructor
                     */
                    virtual ~data() = default;

                    /**
                     *  Read request data
                     *
                     *  @param  connection  The connection to read into
                     */
                    virtual void read_request(connection& connection) noexcept = 0;

                    /**
                     *  Write response data
                     *
                     *  @param  connection  The connection to write to
                     */
                    virtual void write_response(connection& connection) noexcept = 0;

                    state                               state;      // the current state
                    request_type                        request;    // the incoming request to read
                    derived_optional<data_source, 512>  response;   // the response to send
                    bool                                close;      // do we need to close the connection
            };

            /**
             *  The data implementation, templated on
             *  the specific stream- and executor type
             */
            template <typename stream_type, typename executor_type>
            class data_impl : public data
            {
                public:
                    /**
                     *  Constructor
                     *
                     *  @param  executor    The executor to bind to
                     *  @param  parameters  Optional additional arguments for constructing the stream
                     */
                    template <typename... arguments>
                    data_impl(executor_type executor, arguments&&... parameters) noexcept :
                        socket{ executor, std::forward<arguments>(parameters)... }
                    {}

                    /**
                     *  Read request data
                     *
                     *  @param  connection  The connection to read into
                     */
                    void read_request(connection& connection) noexcept override
                    {
                        // read from the socket into the request
                        boost::beast::http::async_read(socket, buffer, this->request, connection);
                        this->state = state::reading;
                    }

                    /**
                     *  Write response data
                     *
                     *  @param  connection  The connection to write to
                     */
                    void write_response(connection& connection) noexcept override
                    {
                        // start sending the response over the stream
                        async_send_data(socket, *this->response, connection);
                        this->state = state::writing;
                    }

                    stream_type                 socket; // the socket to handle
                    boost::beast::flat_buffer   buffer; // buffer to use for reading request data
            };


            routing_table&          _router;    // the table for routing requests
            std::shared_ptr<data>   _data;      // connection state;
    };

}
