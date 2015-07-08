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

#ifndef SPACTION_INCLUDE_AUTOMATA_CONTROLBLOCK_H_
#define SPACTION_INCLUDE_AUTOMATA_CONTROLBLOCK_H_

#include <cstddef>
#include <functional>
#include <unordered_set>

namespace spaction {
namespace automata {

/// Control block interface.
/// Acts as the real memory manager: pass it newly acquired pointers,
/// and tell it to destroy the pointer when ref count reaches 0.
template<typename T>
class ControlBlock {
public:
    virtual ~ControlBlock() { }

    /// Called when an object starts being managed.
    virtual void declare(const T *t) = 0;
    /// Called when an object is no longer managed.
    virtual void release(const T *t) = 0;
};

/// A smart pointer manager with unique ownership semantics.
template<typename T>
class RefControlBlock : public ControlBlock<T> {
 public:
    explicit RefControlBlock(const std::function<void(const T *)> &d): _destroy(d) {}
    ~RefControlBlock() {
        for (auto &r : _pool) {
            _destroy(r);
        }
        _pool.clear();
    }

    /// Ensures that the declared object is not already managed.
    /// Throws if already managed.
    virtual void declare(const T *t) override {
        if (_pool.count(t))
            throw "object is already managed";

        _pool.insert(t);
    }

    /// Called when an object is no longer managed.
    virtual void release(const T *t) override {
        _pool.erase(t);
        _destroy(t);
    }

 private:
    std::unordered_set<const T *> _pool;
    std::function<void(const T *)> _destroy;
};

/// The structure actually stored by the smart pointer (see below).
template<typename T>
struct SmartBlock {
    // are all default ctors, dtor, copy and move semantics OK?
    std::size_t _refcount;
    const T * const _pointer;
    ControlBlock<T> * const _control;

    /// constructor
    explicit SmartBlock(size_t r, const T *p, ControlBlock<T> *cb):
    _refcount(r), _pointer(p), _control(cb) {
        // declare the newly managed pointer to the controller
        _control->declare(_pointer);
    }
    /// destructor
    ~SmartBlock() {
        _control->release(_pointer);
    }
};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_CONTROLBLOCK_H_
