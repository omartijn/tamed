#pragma once

#include "data_source.h"


namespace http {

    /**
     *  Asynchronous operation to send data
     *  to a stream
     */
    template <class stream_type, class handler_type>
    class send_data_operation
    {
        public:
            /**
             *  Constructor
             *
             *  @param  stream  The stream to send data to
             *  @param  data    The data to send over the stream
             *  @param  handler The completion handler to invoke
             */
            send_data_operation(stream_type& stream, data_source& data, handler_type&& handler) noexcept :
                _stream{ stream },
                _data{ data },
                _handler{ std::move(handler) }
            {}

            /**
             *  Callback handler
             *
             *  @param  ec          The error code from the operation
             *  @param  transferred The number of bytes that were transferred
             */
            void operator()(const boost::system::error_code& ec, std::size_t transferred) noexcept
            {
                // check whether the operation resulted in an error
                if (ec != boost::system::error_code{}) {
                    // report the error and stop
                    // processing further data
                    return _handler(ec);
                }

                // did we transfer any data?
                if (transferred != 0) {
                    // consume the data from the source
                    // so we don't send it again
                    _data.consume(transferred);
                }

                // is the data source exhausted?
                if (!_data.is_done()) {
                    // the data to write to the socket
                    const void* data;
                    std::size_t size;

                    // retrieve data to write and write it to the connection
                    _data.next(data, size);
                    _stream.async_write_some(boost::asio::const_buffer(data, size), *this);
                } else {
                    // all data was sent, invoke the final handler
                    _handler(ec);
                }
            }
        private:
            stream_type&    _stream;    // the stream to send over
            data_source&    _data;      // the data to send
            handler_type    _handler;   // the completion handler to invoke
    };

    /**
     *  Send data to a stream asynchronously
     *
     *  @param  stream  The stream to send over
     *  @param  data    The data to send
     *  @param  handler The completion handler
     */
    template <class stream_type, class handler_type>
    decltype(auto) async_send_data(stream_type& stream, data_source& data, handler_type&& handler)
    {
        using handler_signature     = void(boost::system::error_code);
        using completion_token_type = boost::asio::async_completion<handler_type, handler_signature>;
        using operation_type        = send_data_operation<stream_type, BOOST_ASIO_HANDLER_TYPE(handler_type, handler_signature)>;

        // construct the asynchronous completion token
        completion_token_type   init{ handler };

        // create and execute the request
        operation_type{
            stream,
            data,
            std::move(init.completion_handler)
        }(boost::system::error_code{}, 0);

        // return the result of the operation
        return init.result.get();
    }

}
