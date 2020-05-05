#pragma once

#include "connection_data.h"


namespace tamed {

    /**
     *  Read an incoming request
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    class handshake_operation
    {
        public:
            /**
             *  The connection data type
             */
            using data_type = connection_data_impl<router_type, body_type, stream_type, executor_type>;

            /**
             *  Constructor
             *
             *  @param  data    The connection data to use for handshaking
             */
            handshake_operation(std::shared_ptr<data_type> data) :
                _data{ std::move(data) }
            {}

            /**
             *  Handle the completion of reading the request
             *
             *  @param  ec      The error code from the operation
             */
            void operator()(const boost::system::error_code& ec, std::size_t) noexcept
            {
                // did an error occur?
                if (ec != boost::system::error_code{}) {
                    // log the error and abort
                    std::cerr << "Error occurred during TLS handshake: " << ec.message() << std::endl;
                    return;
                }

                // route the request
                _data->read_request();
            }
        private:
            std::shared_ptr<data_type>  _data;  // the connection data
    };

}
