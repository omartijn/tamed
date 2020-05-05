#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include "derived_optional.h"
#include "message_data_source.h"
#include "send_data.h"
#include "stream_traits.h"
#include "connection_data.h"
#include "handshake_operation.h"


namespace tamed {

    /**
     *  Class to handle the requests from a single connection
     */
    class connection
    {
        public:
            /**
             *  Constructor
             *
             *  @param  data    The existing state to work with
             */
            connection(std::shared_ptr<connection_data> data) :
                _data{ std::move(data) }
            {}

            /**
             *  Send a response message
             *
             *  @param  message     The message to send
             */
            template <typename response_body_type>
            void send(boost::beast::http::response<response_body_type> message) noexcept
            {
                // start writing the response message
                _data->write_response(std::move(message));
            }
        private:
            std::shared_ptr<connection_data>    _data;  // connection state;
    };

}

#include "connection_data.inl"
