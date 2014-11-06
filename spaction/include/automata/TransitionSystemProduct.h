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

#ifndef SPACTION_INCLUDE_TRANSITIONSYSTEM_PRODUCT_H_
#define SPACTION_INCLUDE_TRANSITIONSYSTEM_PRODUCT_H_

#include "automata/TransitionSystem.h"

namespace spaction {
namespace automata {

/// @todo add loads of comments

/// The product of two states, merely a typedef for now
template<typename A, typename B> using StateProd = std::pair<A,B>;

/// An interface for the product of two labels.
/// An instance of this class serves as a helper for the product transition system, to build a
/// product labels from two labels, and to retrieve the underlying labels in a product label.
template<typename A, typename B, typename C>
class ILabelProd {
public:
    virtual ~ILabelProd() {}

    /// @todo   this typedef is not very explicit, give it a better name (such as prod_type)
    ///         also, why is C alone exposed, what about A and B?
    typedef C type;

    virtual const A lhs(const type &) const = 0;
    virtual const B rhs(const type &) const = 0;

    virtual const C build(const A &, const B &) const = 0;
};

/// The class for a product of transition systems.
/// Makes the product between a TS<Q1,S1> and a TS<Q2,S2>
/// The type for a product state is StateProd (see above)
/// The type for a product label is explicitly given as a template argument,
/// and must implement the above ILabelProd (enforced at compilation).
template<   typename Q1, typename S1,
            typename Q2, typename S2,
            template<typename S1_, typename S2_> class LabelProd>
class TransitionSystemProduct : public TransitionSystem<StateProd<Q1,Q2>, typename LabelProd<S1,S2>::type> {
    /// Enforce the label product type to implement ILabelProd
    static_assert(std::is_base_of<ILabelProd<S1, S2, typename LabelProd<S1, S2>::type>, LabelProd<S1, S2>>(),
                  "Template argument LabelProd does not derive from ILabelProd");

    /// useful typedefs for state and label product types
    typedef StateProd<Q1,Q2> Q;
    typedef typename LabelProd<S1,S2>::type S;
    /// a typedef for the base class
    typedef TransitionSystem<StateProd<Q1,Q2>, typename LabelProd<S1,S2>::type> super_type;
public:
    /// default constructor
    // @todo check whether the helper is default-constructible
    explicit TransitionSystemProduct(): TransitionSystemProduct(nullptr, nullptr, LabelProd<S1, S2>()) {}

    /// constructor
    /// @note the product does not become responsible for its operands `lhs` and `rhs`
    explicit TransitionSystemProduct(TransitionSystem<Q1,S1> *lhs, TransitionSystem<Q2,S2> *rhs, const LabelProd<S1, S2> &h)
    : _lhs(lhs), _rhs(rhs), _helper(h) {}

    /// Destructor.
    /// @note the product does not become responsible for its operands.
    virtual ~TransitionSystemProduct() { }

    /// @note deliberately left unimplemented
    virtual void add_state(const Q &state) override { assert(false); }
    /// @note deliberately left unimplemented
    virtual void remove_state(const Q &state) override { assert(false); }

    /// a product state (q1,q2) is in the product iff q1 is in its LHS and q2 is in its RHS
    virtual bool has_state(const Q &state) const override {
        return _lhs->has_state(state.first) and _rhs->has_state(state.second);
    }

    /// @note has no side-effect, serves as a public access to `_make_transition`
    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink, const S &label) override {
        return super_type::_make_transition(source, sink, label);
    }
    /// @note deliberately left unimplemented.
    /// Since `add_transition` has no side-effect, doing void would be ok
    virtual void remove_transition(const Q &source, const Q &sink, const S &label) override {
        assert(false);
    }

protected:
    /// the left-hand side of the product
    TransitionSystem<Q1,S1> *_lhs;
    /// the right-hand side of the product
    TransitionSystem<Q2,S2> *_rhs;
    /// the helper for label products
    const LabelProd<S1, S2> _helper;

    /// Underlying transition iterator class.
    class TransitionBaseIterator : public super_type::TransitionBaseIterator {
    public:
        explicit TransitionBaseIterator(const typename TransitionSystem<Q1,S1>::TransitionIterator &l,
                                        const typename TransitionSystem<Q1,S1>::TransitionIterator &lend,
                                        const typename TransitionSystem<Q2,S2>::TransitionIterator &r,
                                        const typename TransitionSystem<Q2,S2>::TransitionIterator &rbegin,
                                        const typename TransitionSystem<Q2,S2>::TransitionIterator &rend,
                                        TransitionSystemProduct *t)
        : _lhs(l)
        , _lend(lend)
        , _rhs(r)
        , _rbegin(rbegin)
        , _rend(rend)
        , _ts(t)
        {
            if (! (_rbegin != _rend)) {
                _lhs = _lend;
                _rhs = _rend;
            }
            if (! (_lhs != _lend)) {
                _rhs = _rend;
            }
            assert((_lhs != _lend) or (!(_rhs != _rend)));
        }
        virtual ~TransitionBaseIterator() { }

        virtual typename super_type::TransitionBaseIterator *clone() const override {
            return new TransitionBaseIterator(*this);
        }

        virtual bool is_equal(const typename super_type::TransitionBaseIterator& rhs) const override {
            const TransitionBaseIterator &other = static_cast<const TransitionBaseIterator &>(rhs);
            assert(!(_lend != other._lend or _rend != other._rend));
            assert(!(other._lhs != other._lend));
            assert(!(other._rhs != other._rend));
            bool res = ! (_lhs != other._lhs or _rhs != other._rhs);
            assert((_lhs != _lend) or (!(_rhs != _rend)));
            assert(
                   (!(_lhs != _lend))?res:true
            );
            return res;
        }

        virtual Transition<Q,S>* operator*() override {
            assert(_lhs != _lend and _rhs != _rend);
            Transition<Q1,S1> *l = *_lhs;
            Transition<Q2,S2> *r = *_rhs;
            auto res = _ts->add_transition(std::make_pair(l->source(), r->source()),
                                           std::make_pair(l->sink(), r->sink()),
                                           _ts->_helper.build(l->label(), r->label()));
            // @todo if l and r get deleted, the memory gets corrupted...
//            delete l;
//            delete r;
            return res;
        }

        virtual const typename super_type::TransitionBaseIterator& operator++() override {
            assert(_lhs != _lend);
            if (++_rhs != _rend)
                return *this;

            if (++_lhs != _lend) {
                _rhs = _rbegin;
                assert(_rhs != _rend);
                return *this;
            }
            assert((_lhs != _lend) or (!(_rhs != _rend)));
            return *this;
        }

    protected:
        typename TransitionSystem<Q1,S1>::TransitionIterator _lhs, _lend;
        typename TransitionSystem<Q2,S2>::TransitionIterator _rhs, _rbegin, _rend;
        TransitionSystemProduct *_ts;
    };

    class StateBaseIterator : public super_type::StateBaseIterator {
    public:
        explicit StateBaseIterator(const typename TransitionSystem<Q1, S1>::StateIterator &l,
                                   const typename TransitionSystem<Q1, S1>::StateIterator &lend,
                                   const typename TransitionSystem<Q2, S2>::StateIterator &r,
                                   const typename TransitionSystem<Q2, S2>::StateIterator &rbegin,
                                   const typename TransitionSystem<Q2, S2>::StateIterator &rend)
        : _lhs(l)
        , _lend(lend)
        , _rhs(r)
        , _rbegin(rbegin)
        , _rend(rend)
        {
            if (! (_rbegin != _rend)) {
                _lhs = _lend;
                _rhs = _rend;
            }
        }

        virtual ~StateBaseIterator() { }

        virtual typename super_type::StateBaseIterator *clone() const override {
            return new StateBaseIterator(*this);
        }

        virtual bool is_equal(const typename super_type::StateBaseIterator& rhs) const override {
            const StateBaseIterator &other = static_cast<const StateBaseIterator &>(rhs);
            return ! (_lhs != other._lhs or _rhs != other._rhs);
        }

        virtual Q operator*() override {
            return StateProd<Q1, Q2>(*_lhs, *_rhs);
        }

        virtual const typename super_type::StateBaseIterator& operator++() override {
            if (++_rhs != _rend)
                return *this;

            if (++_lhs != _lend) {
                _rhs = _rbegin;
                return *this;
            }
            return *this;
        }

    protected:
        typename TransitionSystem<Q1, S1>::StateIterator _lhs, _lend;
        typename TransitionSystem<Q2, S2>::StateIterator _rhs, _rbegin, _rend;
    };

    virtual typename super_type::TransitionBaseIterator *_successor_begin(const Q &state, const S *label) override {
        if (label == nullptr) {
            auto lb = (*_lhs)(state.first).successors().begin();
            auto le = (*_lhs)(state.first).successors().end();
            auto rb = (*_rhs)(state.second).successors().begin();
            auto re = (*_rhs)(state.second).successors().end();
            return new TransitionBaseIterator(lb, le, rb, rb, re, this);
        }
        // else
        auto lb = (*_lhs)(state.first).successors(_helper.lhs(*label)).begin();
        auto le = (*_lhs)(state.first).successors(_helper.lhs(*label)).end();
        auto rb = (*_rhs)(state.second).successors(_helper.rhs(*label)).begin();
        auto re = (*_rhs)(state.second).successors(_helper.rhs(*label)).end();
        return new TransitionBaseIterator(lb, le, rb, rb, re, this);
    }
    virtual typename super_type::TransitionBaseIterator *_successor_end(const Q &state) override {
        auto lb = (*_lhs)(state.first).successors().begin();
        auto le = (*_lhs)(state.first).successors().end();
        auto rb = (*_rhs)(state.second).successors().begin();
        auto re = (*_rhs)(state.second).successors().end();
        return new TransitionBaseIterator(le, le, re, rb, re, this);
    }

    /// @note not implemented yet
    virtual typename super_type::TransitionBaseIterator *_predecessor_begin(const Q &state, const S *label) override { return nullptr; }
    /// @note not implemented yet
    virtual typename super_type::TransitionBaseIterator *_predecessor_end(const Q &state) override { return nullptr; }

    virtual typename super_type::StateBaseIterator *_state_begin() override {
        auto lb = _lhs->states().begin();
        auto le = _lhs->states().end();
        auto rb = _rhs->states().begin();
        auto re = _rhs->states().end();
        return new StateBaseIterator(lb, le, rb, rb, re);
    }
    virtual typename super_type::StateBaseIterator *_state_end() override {
        auto lb = _lhs->states().begin();
        auto le = _lhs->states().end();
        auto rb = _rhs->states().begin();
        auto re = _rhs->states().end();
        return new StateBaseIterator(le, le, re, rb, re);
    }
};

}  // namespace automata
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_TRANSITIONSYSTEM_PRODUCT_H_
