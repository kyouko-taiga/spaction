// This source file is part of spaction
//
// Copyright 2014 Software Modeling and Verification Group
// University of Geneva
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPACTION_INCLUDE_HASH_HASH_H_
#define SPACTION_INCLUDE_HASH_HASH_H_

#include <vector>

namespace std {

/// hash combination and magic numbers are taken from Boost `hash_combine_impl`
template<typename S>
struct hash<std::vector<S>> {
    typedef std::vector<S> argument_type;
    typedef std::size_t result_type;

    result_type operator()(const argument_type &v) const {
        hash<S> h;
        result_type res = 0;
        for (auto s : v) {
            res ^= h(s) + 0x9e3779b9 + (res << 6) + (res >> 2);
        }
        return res;
    }
};

/// hash combination and magic numbers are taken from Boost `hash_combine_impl`
template<typename A, typename B>
struct hash<std::pair<A,B>> {
    typedef std::pair<A,B> argument_type;
    typedef std::size_t result_type;

    result_type operator()(const argument_type &p) const {
        hash<A> ha;
        hash<B> hb;
        result_type res = 0;
        res ^= ha(p.first) + 0x9e3779b9 + (res << 6) + (res >> 2);
        res ^= hb(p.second) + 0x9e3779b9 + (res << 6) + (res >> 2);
        return res;
    }
};

}  // namespace std

#endif  // SPACTION_INCLUDE_HASH_HASH_H_
