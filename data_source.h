#pragma once

#include <cstddef>
#include <boost/asio/buffer.hpp>
#include <boost/container/static_vector.hpp>


namespace http {

    /**
     *  An abstract data source
     */
    class data_source
    {
        public:
            /**
             *  The container we use for data buffers/
             */
            using buffers_type = boost::container::static_vector<boost::asio::const_buffer, 8>;


            /**
             *  Destructor
             */
            virtual ~data_source() = default;

            /**
             *  Has all the data been consumed?
             *
             *  @return Whether all data was used up
             */
            virtual bool is_done() noexcept = 0;

            /**
             *  Retrieve bytes to be sent
             *
             *  @param  ec      The error code from getting the data
             *  @return An array of buffers to be sent
             */
            virtual buffers_type next(boost::system::error_code& ec) noexcept = 0;

            /**
             *  Consume bytes
             *
             *  @param  size    The number of bytes to consume
             */
            virtual void consume(std::size_t size) noexcept = 0;
    };

}
