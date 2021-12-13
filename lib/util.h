#pragma once

#include <boost/iostreams/stream.hpp>

namespace Embag {

// std::make_unique is not available in c++11 :(
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

using message_stream = boost::iostreams::stream<boost::iostreams::array_source>;

template<class PointerType, class ValueType>
class PyBindPointerWrapper;

}

namespace pybind11 { namespace detail {
  template<class Holder>
  struct holder_helper;
  
  template <class PointerType, class ValueType>
  struct holder_helper<Embag::PyBindPointerWrapper<PointerType, ValueType>> {
    static const ValueType* get(const Embag::PyBindPointerWrapper<PointerType, ValueType> &pointer_wrapper) {
      return pointer_wrapper.get();
    }
  };
}}

namespace Embag {

template<class PointerType, class ValueType>
class PyBindPointerWrapper {
 public:
  PyBindPointerWrapper()
  {
  }

  PyBindPointerWrapper(ValueType*& ref)
  {
    throw std::runtime_error("This should never be called");
  }

  PyBindPointerWrapper(const PointerType& pointer_to_wrap)
    : wrapped_pointer_(pointer_to_wrap)
  {
  }

  const ValueType* operator->() const {
    return get();
  }

 private:
  PointerType wrapped_pointer_;

  // We don't want any public interface that exposes the underlying item's pointer as it may be improperly used.
  friend const ValueType* pybind11::detail::holder_helper<PyBindPointerWrapper<PointerType, ValueType>>::get(const PyBindPointerWrapper<PointerType, ValueType>& pointer_wrapper);

  const ValueType* get() const {
    return &**this;
  }

  const ValueType& operator*() const {
    return *wrapped_pointer_;
  }
};

}
