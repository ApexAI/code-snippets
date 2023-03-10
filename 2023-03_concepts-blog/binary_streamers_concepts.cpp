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

#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <concepts>
#include <iterator>
#include <set>
#include <map>
#include <unordered_map>

template<class T>
concept trivial_type = std::is_trivial<T>::value;

template<class T>
concept sequence_container = requires(T t) {
  t.size();
  typename T::iterator;
  typename T::value_type;
  t.emplace(t.end(), typename T::value_type {});
} && std::forward_iterator<typename T::iterator>;

template<class T>
concept tuple_type = requires(T t) {
  std::tuple_size<T>::value;
  std::get<0>(t);
};

template<class T>
std::size_t binary_out(std::ostream & stream, const T & t);

template<class T>
std::size_t binary_in(std::istream & stream, T & t);


template<tuple_type T, std::size_t... I>
inline std::size_t write_tuple_impl(std::ostream & stream, const T & t, std::index_sequence<I...>)
{
  return binary_out(stream, std::get<I>(t)) + ...;
}

template<tuple_type T, std::size_t... I>
inline std::size_t read_tuple_impl(std::istream & stream, T & t, std::index_sequence<I...>)
{
  /* add the result of `binary_in` on each tuple element */
  return binary_in(stream, std::get<I>(t)) + ...;
}

template<tuple_type T>
inline std::size_t binary_out(std::ostream & stream, const T & t)
{
  return write_tuple_impl(
    stream, t,
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>{});
}

template<trivial_type T>
inline std::size_t binary_out(std::ostream & stream, const T & t)
{
  stream.write(reinterpret_cast<const char *>(&t), sizeof(t));
  return sizeof(t);
}

template<sequence_container T>
inline std::size_t binary_out(std::ostream & stream, const T & t)
{
  size_t bytes_written = binary_out(stream, t.size());
  for (const auto & element : t) {
    bytes_written += binary_out(stream, element);
  }
  return bytes_written;
}

template<trivial_type T>
inline std::size_t binary_in(std::istream & stream, T & t)
{
  stream.read(reinterpret_cast<char *>(&t), sizeof(t));
  return sizeof(t);
}

template<tuple_type T>
inline std::size_t binary_in(std::istream & stream, T & t)
{
  return read_tuple_impl(
    stream, t,
    std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<T>>>{});
}

template<sequence_container T>
inline std::size_t binary_in(std::istream & stream, T & t)
{
  typename T::size_type elements;
  std::size_t bytes_read = binary_in(stream, elements);

  for (typename T::size_type to_read = elements; to_read-- > 0; ) {
    typename T::value_type element;
    bytes_read += binary_in(stream, element);
    t.insert(t.end(), element);
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
    printf(
      "Wrote '%zd' bytes to test.data\n",
      binary_out(ofile, std::set<long double>{acosl(-1.0), M_El, M_SQRT2l}));
    printf(
      "Wrote '%zd' bytes to test.data\n",
      binary_out(ofile, std::tuple<int, int, int>{1, 2, 3}));
    // printf("Wrote '%zd' bytes to test.data\n",
    //   binary_out(ofile, std::map<int, double>{{0, M_PI}, {1, M_E}, {2, M_SQRT2}}));
    printf(
      "Wrote '%zd' bytes to test.data\n",
      binary_out(
        ofile, std::vector<std::list<double>>{
      std::list{M_PI, M_E, M_SQRT2}, std::list<double>{3, 2, 1}}));
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
    std::set<long double> set_in;
    printf("Read '%zd' bytes from 'test.data': { ", binary_in(ifile, set_in));
    for (const auto & x : set_in) {
      printf("'%Lf' ", x);
    }
    printf("}\n");
    std::tuple<int, int, int> tuple_in;
    printf("Read '%zd' bytes from 'test.data': {", binary_in(ifile, tuple_in));
    const auto &[x1, x2, x3] = tuple_in;
    printf(" '%d', '%d', '%d' }\n", x1, x2, x3);
    /* NOTE(allenh1): std::map does not work because we can't write to int const. :( */
    // std::map<int, double> map_in;
    // printf("Read '%zd' bytes from 'test.data': {\n", binary_in(ifile, map_in));
    // for (const auto &[key, value] : map_in) {
    //   printf("  '%d': '%lf'\n", key, value);
    // }
    // printf("}\n");
    std::vector<std::list<double>> nested_in;
    printf("Read '%zd' bytes from 'test.data': {\n", binary_in(ifile, nested_in));
    for (const auto & x : nested_in) {
      printf("  { ");
      for (const auto & y : x) {
        printf("'%lf' ", y);
      }
      printf("}\n");
    }
    printf("}\n");
  }
  return 0;
}
