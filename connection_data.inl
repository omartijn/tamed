#pragma once

#include "connection.h"


namespace tamed {

    /**
     *  Read request data
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::read_request() noexcept
    {
        // read from the socket into the request
        boost::beast::http::async_read(socket, buffer, this->request, connection{ this->shared_from_this() });
        this->state = state::reading;
    }

    /**
     *  Route the request to registered
     *  callbacks
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::route_request() noexcept
    {
        // do we need to close the connection after writing
        this->close = request.need_eof();

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
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    void connection_data_impl<router_type, body_type, stream_type, executor_type>::write_response() noexcept
    {
        // start sending the response over the stream
        async_send_data(socket, *this->response, connection{ this->shared_from_this() });
        this->state = state::writing;
    }

}
