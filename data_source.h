#pragma once

#include <cstddef>


namespace http {

    /**
     *  An abstract data source
     */
    class data_source
    {
        public:
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
             *  Retrieve bytes to be send
             *
             *  @param  data    Reference to data pointer to send
             *  @param  size    Reference to size of data to send
             *  @return Whether the data and size were set
             */
            virtual bool next(const void*& data, std::size_t& size) noexcept = 0;

            /**
             *  Consume bytes
             *
             *  @param  size    The number of bytes to consume
             */
            virtual void consume(std::size_t size) noexcept = 0;
    };

}
