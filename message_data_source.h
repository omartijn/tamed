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
             *  Retrieve bytes to be send
             *
             *  @param  data    Reference to data pointer to send
             *  @param  size    Reference to size of data to send
             *  @return Whether the data and size were set
             */
            bool next(const void*& data, std::size_t& size) noexcept override
            {
                // did we get the data, and the error from the operation
                bool                        data_set{ false };
                boost::system::error_code   ec      {       };

                // visit the serializer to get the data
                _serializer.next(ec, [&data, &size, &data_set](boost::system::error_code&, const auto& buffer_sequence) {
                    // is the buffer sequence empty?
                    if (buffer_sequence.begin() == buffer_sequence.end()) {
                        // an empty sequence has no data
                        return;
                    }

                    // set the data and size
                    data = (*buffer_sequence.begin()).data();
                    size = (*buffer_sequence.begin()).size();

                    // we have the data
                    data_set = true;
                });

                // check whether the data was set
                return data_set;
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
