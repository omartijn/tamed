#pragma once


namespace tamed {

    /**
     *  The data members to keep
     *  between handler callbacks
     */
    class connection_data
    {
        public:
            /**
             *  Write the given response
             *
             *  @param  response    The response message to write
             */
            template <typename response_body_type>
            void write_response(boost::beast::http::response<response_body_type> response) noexcept
            {
                // store the message inside the serializer and start writing
                _response.template emplace<message_data_source<response_body_type>>(std::move(response));
                write_response(*_response);
            }
        protected:
            /**
             *  Destructor
             *
             *  @note   The destructor is protected to avoid the data
             *          being deleted through the base pointer
             */
            ~connection_data() = default;
        private:
            /**
             *  Write response data
             *
             *  @param  response    The response message to write
             */
            virtual void write_response(data_source& response) noexcept = 0;

            derived_optional<data_source, 512>  _response;  // the response to send
    };

    /**
     *  The data implementation, templated on
     *  the specific stream- and executor type
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    class connection_data_impl final :
        public connection_data,
        public std::enable_shared_from_this<connection_data_impl<router_type, body_type, stream_type, executor_type>>
    {
        public:
            using request_type = boost::beast::http::request<body_type>;


            /**
             *  Constructor
             *
             *  @param  router      The routing table to route requests
             *  @param  executor    The executor to bind to
             *  @param  parameters  Optional additional arguments for constructing the stream
             */
            template <typename... arguments>
            connection_data_impl(router_type& router, executor_type executor, arguments&&... parameters) noexcept :
                socket{ executor, std::forward<arguments>(parameters)... },
                router{ router }
            {}

            /**
             *  Accept an incoming connection
             *
             *  @param  acceptor    The acceptor to that has an incoming connection waiting
             */
            template <typename acceptor_type>
            void accept(acceptor_type& acceptor) noexcept;

            /**
             *  Read request data
             */
            void read_request() noexcept;

            /**
             *  Route the request to registered
             *  callbacks
             */
            void route_request() noexcept;

            /**
             *  Write response data
             *
             *  @param  response    The response message to write
             */
            void write_response(data_source& response) noexcept override;

            stream_type                 socket;     // the socket to handle
            boost::beast::flat_buffer   buffer;     // buffer to use for reading request data
            router_type&                router;     // the table for routing requests
            request_type                request;    // the incoming request to read
            bool                        close;      // do we need to close the connection
    };

}
