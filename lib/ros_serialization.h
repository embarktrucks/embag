/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ROSCPP_SERIALIZATION_H
#define ROSCPP_SERIALIZATION_H

namespace ros
{
namespace serialization
{

class StreamOverrunException : public std::exception
{
  const std::string what_; 
 public:
  StreamOverrunException(const std::string& what) : what_(what)
  {}
  const char * what () const throw ()
  {
    return what_.c_str();
  }
};
	
void throwStreamOverrun()
{
  throw StreamOverrunException("Buffer Overrun");
}

template<typename T>
struct Serializer
{
  template<typename Stream>
  inline static void write(Stream& stream, typename boost::call_traits<T>::param_type t)
  {
    t.serialize(stream.getData(), 0);
  }

  template<typename Stream>
  inline static void read(Stream& stream, typename boost::call_traits<T>::reference t)
  {
    t.__serialized_length = stream.getLength();
    t.deserialize(stream.getData());
  }

  inline static uint32_t serializedLength(typename boost::call_traits<T>::param_type t)
  {
    return t.serializationLength();
  }
};


template<typename T, typename Stream>
inline void deserialize(Stream& stream, T& t)
{
  Serializer<T>::read(stream, t);
}

namespace stream_types
{
enum StreamType
{
  Input,
  Output,
  Length
};
}
typedef stream_types::StreamType StreamType;

struct Stream
{
  /*
   * \brief Returns a pointer to the current position of the stream
   */
  inline uint8_t* getData() { return data_; }
  // ROS_FORCE_INLINE uint8_t* advance(uint32_t len)
  uint8_t* advance(uint32_t len)
  {
    uint8_t* old_data = data_;
    data_ += len;
    if (data_ > end_)
    {
      // Throwing directly here causes a significant speed hit due to the extra code generated
      // for the throw statement
      throwStreamOverrun();
    }
    return old_data;
  }

  inline uint32_t getLength() { return (uint32_t)(end_ - data_); }

 protected:
  Stream(uint8_t* _data, uint32_t _count)
  : data_(_data)
  , end_(_data + _count)
  {}

 private:
  uint8_t* data_;
  uint8_t* end_;
};

struct IStream : public Stream
{
  static const StreamType stream_type = stream_types::Input;

  IStream(uint8_t* data, uint32_t count)
  : Stream(data, count)
  {}

  template<typename T>
  void next(T& t)
  {
    deserialize(*this, t);
  }

  template<typename T>
  IStream& operator>>(T& t)
  {
    deserialize(*this, t);
    return *this;
  }
};

} // namespace serialization
} // namespace ros

#endif // ROSCPP_SERIALIZATION_H
