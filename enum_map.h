#pragma once

#include <stdexcept>
#include <array>


namespace tamed {

    /**
     *  A compile-time map of enum to value
     */
    template <class Key, class T, Key... Values>
    class enum_map
    {
        public:
            using key_type      = Key;
            using mapped_type   = T;

            /**
             *  Access specified element
             *
             *  @param  key The key to look up
             *  @return Reference to the mapped type
             *  @throws std::out_of_range
             */
            const mapped_type& at(key_type key) const
            {
                return _values[find_index(key)];
            }

            /**
             *  Access specified element
             *
             *  @param  key The key to look up
             *  @return Reference to the mapped type
             *  @throws std::out_of_range
             */
            mapped_type& at(key_type key)
            {
                return _values[find_index(key)];
            }

            /**
             *  Access specified element
             *
             *  @param  key The key to look up
             *  @return Reference to the mapped type
             *  @throws std::out_of_range
             */
            const mapped_type& operator[](key_type key) const
            {
                return _values[find_index(key)];
            }

            /**
             *  Access specified element
             *
             *  @param  key The key to look up
             *  @return Reference to the mapped type
             *  @throws std::out_of_range
             */
            mapped_type& operator[](key_type key)
            {
                return _values[find_index(key)];
            }

            /**
             *  The number of values inside the map
             *
             *  @return Value count
             */
            constexpr static std::size_t size() noexcept
            {
                // we store all given values
                return sizeof...(Values);
            }
        private:
            /**
             *  Match the given value against all
             *  of the valid options.
             *
             *  @tparam I       The current option being tried
             *  @param  value   The value being matched
             *  @return Index of the matched value
             *  @throws std::out_of_range
             */
            template <std::size_t I = 0>
            constexpr static std::size_t find_index(key_type value)
            {
                // is the index in a valid range?
                if constexpr (I >= size()) {
                    // the index is out of range
                    throw std::out_of_range{ "The value is out of range" };
                } else {
                    // did we find a matching value?
                    if (std::get<I>(std::make_tuple(Values...)) == value) {
                        // matching value found
                        return I;
                    } else {
                        // try the next index
                        return find_index<I + 1>(value);
                    }
                }
            }

            std::array<mapped_type, size()> _values;
    };

}
