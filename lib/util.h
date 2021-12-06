#pragma once

#include <boost/iostreams/stream.hpp>

namespace Embag {

// std::make_unique is not available in c++11 :(
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

using message_stream = boost::iostreams::stream<boost::iostreams::array_source>;

template<class T>
class VectorItemPointer;

}

namespace pybind11 { namespace detail {
  template<class Holder>
  struct holder_helper;
  
  template <class T>
  struct holder_helper<Embag::VectorItemPointer<T>> {
    static const T* get(const Embag::VectorItemPointer<T> &vip) {
      return vip.get();
    }
  };
}}

namespace Embag {

template<class T>
class VectorItemPointer {
  std::shared_ptr<std::vector<T>> base;
  size_t index;

 public:
  VectorItemPointer(const std::shared_ptr<std::vector<T>>& base, size_t index)
  : base(base)
  , index(index)
  {
  }

  VectorItemPointer(const std::weak_ptr<std::vector<T>>& base, size_t index)
  : VectorItemPointer(base.lock(), index)
  {
  }

  VectorItemPointer(T*& ref)
  : VectorItemPointer()
  {
    throw std::runtime_error("This should never be called");
  }

  VectorItemPointer()
  : index(0)
  {
  }

  const T* operator->() const {
    return get();
  }

 protected:
  // We don't want any public interface that exposes the underlying item as without
  // a shared_ptr to the vector that contains the item, the memory may be freed.
  friend const T* pybind11::detail::holder_helper<VectorItemPointer<T>>::get(const VectorItemPointer<T>& vip);

  const T* get() const {
    return &**this;
  }

  const T& operator*() const {
    return base->at(index);
  }
};

}
