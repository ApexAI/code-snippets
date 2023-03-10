// Copyright 2023 Apex.AI, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>

template<class T>
struct is_container : public std::false_type {};

template<class U>
struct is_container<std::vector<U>>: public std::true_type {};

template<class U>
struct is_container<std::list<U>>: public std::true_type {};

template<class T, typename _>
std::size_t binary_out(std::ostream & stream, const T & t);

template<class T, typename _>
std::size_t binary_in(std::istream & stream, T & t);


template<
  class T,
  typename std::enable_if<
    std::is_pod<
      typename std::remove_cv<T>::type>::value>::type * = nullptr>
inline std::size_t binary_out(std::ostream & stream, const T & t)
{
  stream.write(reinterpret_cast<const char *>(&t), sizeof(t));
  return sizeof(t);
}

template<
  class T,
  typename std::enable_if<
    is_container<T>::value |
    std::is_array<T>::value>::type * = nullptr>
inline std::size_t binary_out(std::ostream & stream, const T & t)
{
  size_t bytes_written = binary_out(stream, t.size());
  for (const auto & element : t) {
    bytes_written += binary_out(stream, element);
  }
  return bytes_written;
}

template<
  class T,
  typename std::enable_if<
    std::is_pod<
      typename std::remove_cv<T>::type>::value>::type * = nullptr>
std::size_t binary_in(std::istream & stream, T & t)
{
  stream.read(reinterpret_cast<char *>(&t), sizeof(t));
  return sizeof(t);
}

template<
  class T,
  typename std::enable_if<
    is_container<T>::value |
    std::is_array<T>::value>::type * = nullptr>
inline std::size_t binary_in(std::istream & stream, T & t)
{
  typename T::size_type elements;
  std::size_t bytes_read = binary_in(stream, elements);

  for (typename T::size_type to_read = elements; to_read-- > 0; ) {
    typename T::value_type element;
    bytes_read += binary_in(stream, element);
    t.emplace_back(std::move(element));
  }
  return bytes_read;
}

int main(int argc, char ** argv)
{
  const int x = 3;
  {
    std::ofstream ofile{"test.data", std::ios::binary};
    printf("Wrote '%zd' bytes to test.data\n", binary_out(ofile, 4));
    printf("Wrote '%zd' bytes to test.data\n", binary_out(ofile, std::vector<int>{1, 2, 3}));
    printf(
      "Wrote '%zd' bytes to test.data\n", binary_out(
        ofile, std::list<double>{M_PI, M_E, M_SQRT2}));
  }

  {
    std::ifstream ifile{"test.data", std::ios::binary};
    int y;
    printf("Read '%d' from 'test.data' ('%zd bytes')\n", y, binary_in(ifile, y));
    std::vector<int> vector_in;
    printf("Read '%zd' bytes from 'test.data': { ", binary_in(ifile, vector_in));
    for (const auto & x : vector_in) {
      printf("'%d' ", x);
    }
    printf("}\n");
    std::list<double> list_in;
    printf("Read '%zd' bytes from 'test.data': { ", binary_in(ifile, list_in));
    for (const auto & x : list_in) {
      printf("'%lf' ", x);
    }
    printf("}\n");
  }
  return 0;
}
