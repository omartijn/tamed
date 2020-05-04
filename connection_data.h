#pragma once


namespace tamed {

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
    class connection_data
    {
        public:
            /**
             *  Destructor
             */
            virtual ~connection_data() = default;

            /**
             *  Read request data
             */
            virtual void read_request() noexcept = 0;

            /**
             *  Route the request to registered
             *  callbacks
             */
            virtual void route_request() noexcept = 0;

            /**
             *  Write response data
             */
            virtual void write_response() noexcept = 0;

            state                               state   { state::idle   };  // the current state
            derived_optional<data_source, 512>  response{               };  // the response to send
            bool                                close   { false         };  // do we need to close the connection
    };

    /**
     *  The data implementation, templated on
     *  the specific stream- and executor type
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    class connection_data_impl :
        public connection_data,
        public std::enable_shared_from_this<connection_data_impl<router_type, body_type, stream_type, executor_type>>
    {
        public:
            using request_body_type = body_type;
            using request_type      = boost::beast::http::request<request_body_type>;


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
             *  Read request data
             */
            void read_request() noexcept final;

            /**
             *  Route the request to registered
             *  callbacks
             */
            void route_request() noexcept final;

            /**
             *  Write response data
             */
            void write_response() noexcept final;

            stream_type                 socket;     // the socket to handle
            boost::beast::flat_buffer   buffer;     // buffer to use for reading request data
            router_type&                router;     // the table for routing requests
            request_type                request;    // the incoming request to read
    };

}
