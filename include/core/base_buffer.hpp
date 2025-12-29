//
//  base_buffer.hpp
//  toybox
//
//  Created by Fredrik on 2025-12-28.
//

#pragma once

namespace toybox {
    
    namespace detail {

        /**
         Static storage base class for vector_c and map_c when Count > 0.
         Provides fixed-capacity storage using a statically allocated array.
         */
        template<class Type, int Count>
        class base_buffer_static_c {
        protected:
            // Storage interface for vector_c
            __forceinline aligned_membuf_s<Type>* __buffer() __pure {
                return _buffer;
            }
            __forceinline const aligned_membuf_s<Type>* __buffer() const __pure {
                return _buffer;
            }
            __forceinline constexpr int __capacity() const __pure {
                return Count;
            }
            __forceinline void __ensure_capacity(int needed, int current_size) const {
                (void)current_size;  // Unused for static storage
                assert(needed <= Count && "Vector capacity exceeded");
            }
        private:
            aligned_membuf_s<Type> _buffer[Count];
        };

        /**
         Dynamic storage base class for vector_c and map_c when Count == 0.
         Provides growable heap-allocated storage with automatic reallocation.
         */
        template<class Type>
        class base_buffer_dynamic_c {
        protected:
            ~base_buffer_dynamic_c() {
                if (_buffer) delete[] _buffer;
            }

            // Storage interface for vector_c
            __forceinline aligned_membuf_s<Type>* __buffer() __pure {
                return _buffer;
            }
            __forceinline const aligned_membuf_s<Type>* __buffer() const __pure {
                return _buffer;
            }
            __forceinline int __capacity() const __pure {
                return _capacity;
            }

            void __ensure_capacity(int needed, int current_size) {
                if (needed <= _capacity) return;
                int new_cap = needed;
                if (_capacity > 0) {
                    new_cap = _capacity * 2;
                    if (new_cap < needed) new_cap = needed;
                } else {
                    if (new_cap < 8) new_cap = 8;
                }
                auto* new_buffer = new aligned_membuf_s<Type>[new_cap];

                // Move existing constructed elements to new buffer
                if (current_size > 0) {
                    Type* src_first = _buffer[0].template ptr<0>();
                    Type* src_last = _buffer[current_size].template ptr<0>();
                    Type* dst_first = new_buffer[0].template ptr<0>();
                    uninitialized_move(src_first, src_last, dst_first);
                    destroy(src_first, src_last);
                }

                if (_buffer) delete[] _buffer;
                _buffer = new_buffer;
                _capacity = new_cap;
            }

            /// Transfer ownership from another base_vector_dynamic_c (for move operations)
            __forceinline void __take_ownership(base_buffer_dynamic_c& o) {
                _buffer = o._buffer;
                _capacity = o._capacity;
                o._buffer = nullptr;
                o._capacity = 0;
            }

            /// Release ownership without destroying (for move operations)
            __forceinline void __release_ownership() {
                if (_buffer) delete[] _buffer;
                _buffer = nullptr;
                _capacity = 0;
            }

        private:
            aligned_membuf_s<Type>* _buffer = nullptr;
            int _capacity = 0;
        };

    } // namespace detail
    
}
