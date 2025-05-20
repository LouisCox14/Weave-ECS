#pragma once
#include <tuple>
#include <vector>
#include "Mathematics.h"

namespace Weave 
{
    namespace Utilites
    {
        /*
            This code is all to deal with hashing for types that C++ doesn't support out of the box like Vectors, Tuples, and any type from Weave Engine itself.
        */

        template <typename TT>
        struct Hash
        {
            size_t operator()(TT const& tt) const
            {
                return std::hash<TT>()(tt);
            }
        };

        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            /*
                Just a brief explanation here : 0x9e3779b9 is a magic constant from the golden ratio that makes for much more uniform hash dispersion.The bit shifts are another
                quick way of accomplishing the same goal. 
            */

            seed ^= Hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
            static void apply(size_t& seed, Tuple const& tuple)
            {
                HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
                hash_combine(seed, std::get<Index>(tuple));
            }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple, 0>
        {
            static void apply(size_t& seed, Tuple const& tuple)
            {
                hash_combine(seed, std::get<0>(tuple));
            }
        };

        template <typename ... TT>
        struct Hash<std::tuple<TT...>>
        {
            size_t operator()(std::tuple<TT...> const& tt) const
            {
                size_t seed = 0;
                HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
                return seed;
            }
        };

        template <typename T>
        struct Hash<std::vector<T>>
        {
            std::size_t operator()(std::vector<T> const& vec) const {
                std::size_t seed = vec.size();

                for (T x : vec) {
                    hash_combine(seed, x);
                }

                return seed;
            }
        };

        template <typename T>
        struct Hash<Mathematics::Vector2<T>>
        {
            std::size_t operator()(const Mathematics::Vector2<T>& vec2) const
            {
                std::size_t seed = 0;

                hash_combine(seed, vec2.x);
                hash_combine(seed, vec2.y);

                return seed;
            }
        };
    }
}