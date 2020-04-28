#pragma once

#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/message.hpp>

#include "data_source.h"


namespace http {

    /**
     *  Message serializer for a
     *  specific message type
     */
    template <typename body_type>
    class message_data_source : public data_source
    {
        public:
            /**
             *  Constructor
             *
             *  @param  message The message we are serializing
             */
            message_data_source(boost::beast::http::response<body_type>&& message) :
                _message{ std::move(message) },
                _serializer{ _message }
            {
                // prepare the payload for sending
                _message.prepare_payload();
            }

            /**
             *  Has all the data been consumed?
             *
             *  @return Whether all data was used up
             */
            bool is_done() noexcept override
            {
                // check whether the serializer is done
                return _serializer.is_done();
            }

            /**
             *  Retrieve bytes to be sent
             *
             *  @param  ec      The error code from getting the data
             *  @return An array of buffers to be sent
             */
            buffers_type next(boost::system::error_code& ec) noexcept override
            {
                // the buffers to fill
                buffers_type result;

                // visit the serializer to get the data
                _serializer.next(ec, [&result](boost::system::error_code&, const auto& buffer_sequence) {
                    // process all buffers
                    for (const auto& buffer : buffer_sequence) {
                        // has the result reached capacity? then we cannot
                        // add more buffers. they will have to be retrieved
                        // later after consuming some of the existing data
                        if (result.size() == result.capacity()) {
                            break;
                        }

                        // add the buffer to the result
                        result.emplace_back(buffer.data(), buffer.size());
                    }
                });

                // return the filled buffer list
                return result;
            }

            /**
             *  Consume bytes
             *
             *  @param  size    The number of bytes to consume
             */
            void consume(std::size_t size) noexcept override
            {
                // the serializer does not appreciate
                // being asked to consume 0 bytes
                if (size != 0) {
                    // consume from the serializer
                    return _serializer.consume(size);
                }
            }
        private:
            boost::beast::http::response<body_type>             _message;       // the message we are serializing
            boost::beast::http::serializer<false, body_type>    _serializer;    // the serializer for the message
    };

}
