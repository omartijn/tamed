#pragma once

#include "connection_data.h"


namespace tamed {

    /**
     *  Read an incoming request
     */
    template <class router_type, class body_type, typename stream_type, typename executor_type>
    class write_operation
    {
        public:
            /**
             *  The connection data type
             */
            using data_type = connection_data_impl<router_type, body_type, stream_type, executor_type>;

            /**
             *  Constructor
             *
             *  @param  data    The connection data to write to
             */
            write_operation(std::shared_ptr<data_type> data) :
                _data{ std::move(data) }
            {}

            /**
             *  Retrieve the executor
             *
             *  @return The executor associated with the connection
             */
            executor_type get_executor() noexcept
            {
                return _data->get_executor();
            }

            /**
             *  Handle the completion of reading the request
             *
             *  @param  ec      The error code from the operation
             */
            void operator()(const boost::system::error_code& ec) noexcept
            {
                // did an error occur?
                if (ec != boost::system::error_code{}) {
                    // log the error and abort
                    std::cerr << "Error occurred during request reading: " << ec.message() << std::endl;
                    return;
                }

                // do we need to continue reading
                if (!_data->close) {
                    // read the next request
                    _data->read_request();
                }
            }
        private:
            std::shared_ptr<data_type>  _data;  // the connection data
    };

}
