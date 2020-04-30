#pragma once

#include <type_traits>
#include <utility>


namespace tamed {

    /**
     *  An optional value, containing any class
     *  that is derived from the templated class.
     *
     *  If the buffer size is sufficient, the class
     *  is constructed in-place, inside the buffer,
     *  otherwise it is allocated.
     */
    template <typename T, std::size_t S>
    class derived_optional
    {
        public:
            /**
             *  The type we are storing inside this optional.
             *  Derived classes are also allowed.
             */
            using value_type = T;

            /**
             *  The in-place buffer type. If the class
             *  fits within this buffer, no allocations
             *  are done by derived_optional.
             */
            constexpr const static std::size_t buffer_size = S;

            /**
             *  If the base class does not fit within the buffer,
             *  there is really no point to using this class, as
             *  it will always allocate memory (since derived classes
             *  will never be _smaller_).
             */
            static_assert(sizeof(value_type) <= buffer_size);

            /**
             *  Constructor
             *
             *  Does not initialize anything, the optional
             *  will not have a value.
             */
            derived_optional() = default;

            /**
             *  Constructor
             *
             *  @param  parameters  The arguments used for constructing the instance
             */
            template <typename instance, typename... arguments>
            derived_optional(std::in_place_type_t<instance>, arguments&&... parameters)
            {
                // emplace the instance in the storage
                emplace<instance>(std::forward<arguments>(parameters)...);
            }

            /**
             *  Copying and moving is not allowed
             *  because it would result in slicing
             */
            derived_optional(const derived_optional&) = delete;
            derived_optional(derived_optional&&) = delete;

            /**
             *  Destructor
             */
            ~derived_optional()
            {
                // ensure any instance is freed
                reset();
            }

            /**
             *  Retrieve a pointer to the contained value
             *
             *  @return The pointer to the contained value, or a
             *          nullptr if no value is contained
             */
            const value_type* operator->() const
            {
                // what state are we in?
                switch (_state) {
                    // are we uninitialized?
                    case state::uninitialized:
                        // no pointer to return
                        return nullptr;
                    case state::initialized_in_place:
                        // cast the buffer and launder it
                        // to avoid optimizations resulting
                        // in undefined behaviour
                        return std::launder(reinterpret_cast<const value_type*>(&_buffer));
                    case state::initialized_allocated:
                    {
                        // the result pointer to fill
                        const value_type* result;

                        // copy the data from the buffer
                        std::memcpy(&result, &_buffer, sizeof(result));

                        // return the filled result
                        return result;
                    }
                }
            }

            /**
             *  Retrieve a pointer to the contained value
             *
             *  @return The pointer to the contained value, or a
             *          nullptr if no value is contained
             */
            value_type* operator->()
            {
                // cast away the const, this is allowed, since our
                // this pointer is known to be not-const at this pointer
                return const_cast<value_type*>(std::as_const(*this).operator->());
            }

            /**
             *  Retrieve a reference to the contained value
             *
             *  @return The reference to the contained value
             *  @note   If no value is contained, this function results
             *          in undefined behaviour
             */
            const value_type& operator*() const &
            {
                return *operator->();
            }

            /**
             *  Retrieve a reference to the contained value
             *
             *  @return The reference to the contained value
             *  @note   If no value is contained, this function results
             *          in undefined behaviour
             */
            value_type& operator*() &
            {
                return *operator->();
            }

            /**
             *  Check whether we contain a value
             *
             *  @return Do we have an initialized instance
             */
            bool has_value() const noexcept
            {
                // check whether we are initialized
                return _state != state::uninitialized;
            }

            /**
             *  Check whether we contain a value
             *
             *  @return Do we have an initialized instance
             */
            operator bool() const noexcept
            {
                // check whether we have a value
                return has_value();
            }

            /**
             *  Retrieve the stored value, or throw
             *  an exception if no value is available
             *
             *  @return The stored value
             *  @throws std::bad_optional_access
             */
            value_type& value() &
            {
                // cast away the const, this is allowed, since our
                // this pointer is known to be not-const at this pointer
                return const_cast<value_type&>(std::as_const(this)->value());
            }

            /**
             *  Retrieve the stored value, or throw
             *  an exception if no value is available
             *
             *  @return The stored value
             *  @throws std::bad_optional_access
             */
            const value_type& value() const &
            {
                // check if we have a value
                if (has_value()) {
                    // return the stored value
                    return *operator->();
                }

                // no value available
                throw std::bad_optional_access{};
            }

            /**
             *  Destroy any instance contained. If no instance
             *  is contained, no action is performed
             */
            void reset() noexcept
            {
                // do we have an in-buffer instance?
                if (_state == state::initialized_in_place) {
                    // call the destructor on the object
                    // but do not free it (not allocated)
                    operator->()->~value_type();
                } else if (_state == state::initialized_allocated) {
                    // the instance was allocated, delete it
                    delete operator->();
                }

                // we no longer have an instance
                _state = state::uninitialized;
            }

            /**
             *  Emplace an instance in the storage
             *
             *  @param  parameters  The parameters for creating the instance
             *  @return Reference to the created instance
             */
            template <typename instance, typename... arguments>
            std::enable_if_t<std::is_base_of_v<value_type, instance>>
            emplace(arguments&&... parameters)
            {
                // can we create the instance inside the buffer
                if constexpr (sizeof(instance) <= buffer_size) {
                    // create the new instance
                    new (&_buffer) instance{ std::forward<arguments>(parameters)... };

                    // we are now initialized in-place
                    _state = state::initialized_in_place;
                } else {
                    // create the new instance and store the deleter
                    auto pointer = new instance{ std::forward<arguments>(parameters)... };

                    // store the pointer inside the buffer
                    std::memcpy(&_buffer, &pointer, sizeof(pointer));

                    // we are now initialized in allocated memory
                    _state = state::initialized_allocated;
                }
            }
        private:
            /**
             *  The type to use for storing the instance
             */
            using buffer_type   = std::aligned_storage_t<buffer_size>;

            /**
             *  The current state of our instance
             */
            enum class state
            {
                uninitialized,
                initialized_in_place,
                initialized_allocated
            };

            buffer_type     _buffer;                            // the data buffer for storing the instance
            state           _state  { state::uninitialized  };  // default constructor does not initialize anything
    };

}
