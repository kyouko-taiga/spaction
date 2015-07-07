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

#ifndef SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRODUCT_H_
#define SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRODUCT_H_

#include "automata/TransitionSystem.h"

namespace spaction {
namespace automata {

/// The product of two states, merely a typedef for now
template<typename A, typename B> using StateProd = std::pair<A, B>;

/// An interface for the product of two labels.
/// An instance of this class serves as a helper for the product transition system, to build a
/// product labels from two labels, and to retrieve the underlying labels in a product label.
template<typename A, typename B, typename C>
class ILabelProd {
 public:
    virtual ~ILabelProd() {}

    typedef A lhs_type;
    typedef B rhs_type;
    typedef C product_type;

    virtual const lhs_type lhs(const product_type &) const = 0;
    virtual const rhs_type rhs(const product_type &) const = 0;

    virtual const product_type build(const lhs_type &, const rhs_type &) const = 0;
    virtual bool is_false(const product_type &) const = 0;
};

/// forward declaration
template<   typename Q1, typename S1, typename D1,
            typename Q2, typename S2, typename D2,
            template<typename S1_, typename S2_> class LabelProd>
class TransitionSystemProduct;

/// Underlying transition iterator class.
template<   typename Q1, typename S1, typename Iterator1, typename D1,
            typename Q2, typename S2, typename Iterator2, typename D2,
            template<typename, typename> class LabelProd>
class TSProductIterator : public ITransitionBaseIterator<   StateProd<Q1,Q2>, typename LabelProd<S1, S2>::product_type,
                                                            TSProductIterator<Q1, S1, Iterator1, D1, Q2, S2, Iterator2, D2, LabelProd>> {
    using super_type = ITransitionBaseIterator< StateProd<Q1,Q2>, typename LabelProd<S1, S2>::product_type,
                                                TSProductIterator<Q1, S1, Iterator1, D1, Q2, S2, Iterator2, D2, LabelProd>>;
    /// useful typedefs for state and label product types
    typedef StateProd<Q1, Q2> Q;
    typedef typename LabelProd<S1, S2>::product_type S;

    using TSProduct = TransitionSystemProduct<Q1, S1, D1, Q2, S2, D2, LabelProd>;
 public:
    explicit TSProductIterator(const Iterator1 &l,
                               const Iterator1 &lend,
                               const Iterator2 &r,
                               const Iterator2 &rbegin,
                               const Iterator2 &rend,
                               TSProduct *t)
    : _lhs(l)
    , _lend(lend)
    , _rhs(r)
    , _rbegin(rbegin)
    , _rend(rend)
    , _ts(t) {
        if (!(_rbegin != _rend)) {
            _lhs = _lend;
            _rhs = _rend;
        }
        if (!(_lhs != _lend)) {
            _rhs = _rend;
        }
        assert((_lhs != _lend) or (!(_rhs != _rend)));

        while (!done() && conditions_invalid()) {
            incr();
        }
    }

    explicit TSProductIterator(Iterator1 &&l,
                               Iterator1 &&lend,
                               Iterator2 &&r,
                               Iterator2 &&rbegin,
                               Iterator2 &&rend,
                               TSProduct *t)
    : _lhs(std::move(l))
    , _lend(std::move(lend))
    , _rhs(std::move(r))
    , _rbegin(std::move(rbegin))
    , _rend(std::move(rend))
    , _ts(t) {
        if (!(_rbegin != _rend)) {
            _lhs = _lend;
            _rhs = _rend;
        }
        if (!(_lhs != _lend)) {
            _rhs = _rend;
        }
        assert((_lhs != _lend) or (!(_rhs != _rend)));

        while (!done() && conditions_invalid()) {
            incr();
        }
    }

    virtual ~TSProductIterator() { }

    virtual super_type *clone() const override {
        return new TSProductIterator(*this);
    }

    virtual bool is_equal(const super_type& rhs) const override {
        const TSProductIterator &other = static_cast<const TSProductIterator &>(rhs);
        assert(!(_lend != other._lend or _rend != other._rend));
        assert(!(other._lhs != other._lend));
        assert(!(other._rhs != other._rend));
        bool res = !(_lhs != other._lhs or _rhs != other._rhs);
        assert((_lhs != _lend) or (!(_rhs != _rend)));
        assert((!(_lhs != _lend))?res:true);
        return res;
    }

    virtual TransitionPtr<Q, S> operator*() override {
        assert(_lhs != _lend and _rhs != _rend);
        TransitionPtr<Q1, S1> l = *_lhs;
        TransitionPtr<Q2, S2> r = *_rhs;
        auto res = _ts->add_transition(std::make_pair(l->source(), r->source()),
                                       std::make_pair(l->sink(), r->sink()),
                                       _ts->_helper.build(l->label(), r->label()));
        return TransitionPtr<Q, S>(res, _ts->get_control_block());
    }

    S get_label() const override {
        return _ts->_helper.build(_lhs.get_label(), _rhs.get_label());
    }
    const Q get_source() const override {
        return std::make_pair(_lhs.get_source(), _rhs.get_source());
    }
    const Q get_sink() const override {
        return std::make_pair(_lhs.get_sink(), _rhs.get_sink());
    }
    /// @TODO the implementations should depend on whether
    ///     S1 is a CounterLabel
    ///     S2 is a CounterLabel
    template<bool U = is_counter_label<S>::value>
    typename std::enable_if<U && is_counter_label<S1>::value && is_counter_label<S2>::value, typename LetterType<S>::type>::type _get_letter() const {
        return _ts->_helper.build_letter(_lhs.get_letter(), _rhs.get_letter());
    }
    template<bool U = is_counter_label<S>::value>
    typename std::enable_if<U, accs_t>::type _get_acceptance() const {
        return _ts->_helper.build_acceptance(_lhs.get_acceptance(), _rhs.get_acceptance());
    }
    template<bool U = is_counter_label<S>::value>
    typename std::enable_if<U, std::vector<CounterOperationList>>::type _get_operations() const {
        return _ts->_helper.build_operations(_lhs.get_operations(), _rhs.get_operations());
    }


    virtual const super_type& operator++() override {
        incr();
        while (!done() && conditions_invalid()) {
            incr();
        }
        return *this;
    }

 protected:
    bool done() const { return !(_lhs != _lend or _rhs != _rend); }
    bool conditions_invalid() {
        return _ts->_helper.is_false(this->_get_letter());
    }

    void incr() {
        assert(_lhs != _lend);
        if (++_rhs != _rend)
            return;

        if (++_lhs != _lend) {
            _rhs = _rbegin;
            assert(_rhs != _rend);
            return;
        }
        assert((_lhs != _lend) or (!(_rhs != _rend)));
    }

 protected:
    Iterator1 _lhs, _lend;
    Iterator2 _rhs, _rbegin, _rend;
    TSProduct *_ts;
};

/// The class for a product of transition systems.
/// Makes the product between a TS<Q1,S1> and a TS<Q2,S2>
/// The type for a product state is StateProd (see above)
/// The type for a product label is explicitly given as a template argument,
/// and must implement the above ILabelProd (enforced at compilation).
template<   typename Q1, typename S1, typename D1,
            typename Q2, typename S2, typename D2,
            template<typename S1_, typename S2_> class LabelProd>
class TransitionSystemProduct :
    public TransitionSystem<StateProd<Q1, Q2>,
                            typename LabelProd<S1, S2>::product_type,
                            TransitionSystemProduct<Q1, S1, D1, Q2, S2, D2, LabelProd>,
                            TSProductIterator<  Q1, S1, typename D1::TransitionIterator, D1,
                                                Q2, S2, typename D2::TransitionIterator, D2,
                                                LabelProd>> {
    /// Enforce the label product type to implement ILabelProd
    static_assert(std::is_base_of<ILabelProd<S1, S2, typename LabelProd<S1, S2>::product_type>, LabelProd<S1, S2>>(),
                  "Template argument LabelProd does not derive from ILabelProd");

    /// useful typedefs for state and label product types
    typedef StateProd<Q1, Q2> Q;
    typedef typename LabelProd<S1, S2>::product_type S;
 public:
    using TransitionBaseIterator = TSProductIterator<Q1, S1, typename D1::TransitionIterator, D1, Q2, S2, typename D2::TransitionIterator, D2, LabelProd>;
    friend class TSProductIterator<Q1, S1, typename D1::TransitionIterator, D1, Q2, S2, typename D2::TransitionIterator, D2, LabelProd>;
    /// a typedef for the base class
    typedef TransitionSystem<   StateProd<Q1, Q2>, typename LabelProd<S1, S2>::product_type,
                                TransitionSystemProduct<Q1, S1, D1, Q2, S2, D2, LabelProd>,
                                TransitionBaseIterator> super_type;
    typedef ITransitionBaseIterator<StateProd<Q1, Q2>, typename LabelProd<S1,S2>::product_type, TransitionBaseIterator> ITransitionIterator;

 public:
    /// constructor
    /// @note the product does not become responsible for its operands `lhs` and `rhs`
    explicit TransitionSystemProduct(D1 *lhs,
                                     D2 *rhs,
                                     const LabelProd<S1, S2> &h, std::shared_ptr<Data> d):
    super_type(new RefControlBlock<Transition<Q, S>>(
         std::bind(&TransitionSystemProduct::_delete_transition, this, std::placeholders::_1)), d),
    _lhs(lhs), _rhs(rhs), _helper(h) { }

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

    /// method to print state: handles pair of states
    virtual void print_state(std::ostream &os, const Q &q) const override {
        os << "(";
        _lhs->print_state(os, q.first);
        os << ",";
        _rhs->print_state(os, q.second);
        os << ")";
    }

    virtual void print_label(std::ostream &os, const S &s) const override {
        os << s;
    }

 protected:
    /// the left-hand side of the product
    D1 *_lhs;
    /// the right-hand side of the product
    D2 *_rhs;
    /// the helper for label products
    const LabelProd<S1, S2> _helper;

    /// Underlying state iterator class.
    class StateBaseIterator : public super_type::StateBaseIterator {
     public:
        explicit StateBaseIterator(const typename D1::StateIterator &l,
                                   const typename D1::StateIterator &lend,
                                   const typename D2::StateIterator &r,
                                   const typename D2::StateIterator &rbegin,
                                   const typename D2::StateIterator &rend)
        : _lhs(l)
        , _lend(lend)
        , _rhs(r)
        , _rbegin(rbegin)
        , _rend(rend) {
            if (!(_rbegin != _rend)) {
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
            return !(_lhs != other._lhs or _rhs != other._rhs);
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
        typename D1::StateIterator _lhs, _lend;
        typename D2::StateIterator _rhs, _rbegin, _rend;
    };

    virtual ITransitionIterator *_successor_begin(const Q &state, const S *label) override {
        // differentiate the labeled and unlabeled versions
        if (label == nullptr) {
            auto rb = (*_rhs)(state.second).successors().begin();
            auto rb2 = rb;
            return new TransitionBaseIterator((*_lhs)(state.first).successors().begin(),
                                              (*_lhs)(state.first).successors().end(),
                                              std::move(rb),
                                              std::move(rb2),
                                              (*_rhs)(state.second).successors().end(),
                                              this);
        }
        // else
        auto rb = (*_rhs)(state.second).successors(_helper.rhs(*label)).begin();
        auto rb2 = rb;
        return new TransitionBaseIterator((*_lhs)(state.first).successors(_helper.lhs(*label)).begin(),
                                          (*_lhs)(state.first).successors(_helper.lhs(*label)).end(),
                                          std::move(rb),
                                          std::move(rb2),
                                          (*_rhs)(state.second).successors(_helper.rhs(*label)).end(),
                                          this);
    }
    virtual ITransitionIterator *_successor_end(const Q &state) override {
        auto le = (*_lhs)(state.first).successors().end();
        auto le2 = le;
        auto re = (*_rhs)(state.second).successors().end();
        auto re2 = re;
        return new TransitionBaseIterator(std::move(le),
                                          std::move(le2),
                                          std::move(re),
                                          (*_rhs)(state.second).successors().begin(),
                                          std::move(re2),
                                          this);
    }

    /// @note not implemented yet
    virtual ITransitionIterator *_predecessor_begin(const Q &state, const S *label) override { return nullptr; }
    /// @note not implemented yet
    virtual ITransitionIterator *_predecessor_end(const Q &state) override { return nullptr; }

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

#endif  // SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRODUCT_H_
