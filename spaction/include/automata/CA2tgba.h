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

#ifndef SPACTION_INCLUDE_AUTOMATA_CA2TGBA_H_
#define SPACTION_INCLUDE_AUTOMATA_CA2TGBA_H_

#include <cassert>
#include <sstream>

#include <spot/ltlast/atomic_prop.hh>
#include <spot/ltlast/constant.hh>
#include <spot/ltlast/multop.hh>
#include <spot/ltlast/unop.hh>
#include <spot/ltlenv/defaultenv.hh>
#include <spot/tgba/formula2bdd.hh>
#include <spot/tgba/tgba.hh>

#include "cltl2spot.h"
#include "automata/CounterAutomaton.h"

namespace std {

template<>
struct hash<spot::state *> {
    typedef spot::state * argument_type;
    typedef std::size_t result_type;

    result_type operator()(const argument_type &s) const {
        return spot::state_ptr_hash()(s);
    }
};

}  // namespace std

namespace spaction {
namespace automata {

/// forward declaration
template<typename, typename, template<typename, typename> class> class CA2tgba;

template<typename Q>
class state_adapter : public spot::state {
public:
    /// constructor
    explicit state_adapter(const Q &q): _state(q) {}
    /// destructor is protected, use method `destroy` instead
    /// @note the default `destroy` implementation is fine, no need to override it here

    virtual int compare(const spot::state* other) const override {
        assert(dynamic_cast<const state_adapter *>(other));
        const state_adapter *o = static_cast<const state_adapter *>(other);

        /// use of std::less allows to properly compare pairs of pointers
        if (std::less<Q>()(_state, o->_state))
            return -1;
        if (std::less<Q>()(o->_state, _state))
            return 1;
        return 0;
    }

    virtual std::size_t hash() const override {
        return std::hash<Q>()(_state);
    }

    virtual state* clone() const override { return new state_adapter(*this); }

    const Q state() const { return _state; }

protected:
    const Q _state;
    ~state_adapter() {}
};

template<typename Q, typename S, template<typename, typename> class TS>
struct _succ_helper {};

template<typename Q, template<typename, typename> class TS>
class _succ_helper<Q, CltlTranslator::FormulaList, TS> {
    using S = CltlTranslator::FormulaList;
public:
    explicit _succ_helper(const CA2tgba<Q,S,TS> *t): _ts(t) {}

    bdd get_condition(const TransitionPtr<Q, CounterLabel<S>> &trans) const {
        const spot::ltl::formula * fspot = spot::ltl::constant::true_instance();
        for (auto f : trans->label().letter()) {
            fspot = spot::ltl::multop::instance(spot::ltl::multop::And, cltl2spot(f), fspot);
        }
        return spot::formula_to_bdd(fspot, _ts->get_dict(), (void*)_ts);
    }

private:
    const CA2tgba<Q,S,TS> *_ts;
};

template<typename Q, typename S, template<typename, typename> class TS>
class succiter_adapter : public spot::tgba_succ_iterator {
public:
    /// constructor
    explicit succiter_adapter(const typename TransitionSystem<Q,CounterLabel<S>>::TransitionIterator &b,
                              const typename TransitionSystem<Q,CounterLabel<S>>::TransitionIterator &e,
                              const CA2tgba<Q,S,TS> *t)
    : _current(b), _begin(b), _end(e), _ts(t) {}
    /// destructor @todo make it private?
    ~succiter_adapter() {}

    virtual void first() override {
        _current = _begin;
    }
    virtual void next() override {
        ++_current;
    }
    virtual bool done() const override {
        return !(_current != _end);
    }

    virtual spot::state* current_state() const override {
        return new state_adapter<Q>((*_current)->sink());
    }

    virtual bdd current_condition() const override {
        return _succ_helper<Q, S, TS>(_ts).get_condition(*_current);
    }
    virtual bdd current_acceptance_conditions() const override {
        bdd result = bddtrue;
        auto accs = (*_current)->label().get_acceptance();
        for (auto i : accs) {
            result &= bdd_ithvar(_ts->accs_maps(i));
        }
        return result;
    }

    TransitionPtr<Q, CounterLabel<S>> get_trans() const { return *_current; }

private:
    mutable typename TransitionSystem<Q,CounterLabel<S>>::TransitionIterator _current;
    const typename TransitionSystem<Q,CounterLabel<S>>::TransitionIterator _begin, _end;
    const CA2tgba<Q,S,TS> *_ts;
};

/// Turn a CounterAutomaton into a TGBA, by simply forgetting the counters
template<typename Q, typename S, template<typename, typename> class TS>
class CA2tgba : public spot::tgba {
 public:
    explicit CA2tgba(CounterAutomaton<Q, S, TS> *a, spot::bdd_dict *d = nullptr)
    : _automaton(a)
    , _dict(d?d:new spot::bdd_dict())
    , _delete_dict(!d) {
        // DO NOT declare all the AP to the bdd dictionnary yet, to be done on the fly
        // declare all the acceptance conditions to the bdd dictionnary
        for (std::size_t i = 0 ; i != _automaton->num_acceptance_sets() ; ++i) {
            // register in bdd dict (requires to go through a spot AP
            std::stringstream ss;
            ss << "_Acc" << i;
            const spot::ltl::formula *form = spot::ltl::atomic_prop::instance(ss.str(), spot::ltl::default_environment::instance());
            int bddvar = _dict->register_acceptance_variable(form, this);
            _acc_bdd[i] = bddvar;
        }
    }

    ~CA2tgba() {
        // clean up the dictionary
        _dict->unregister_all_my_variables(this);
        if (_delete_dict)
            delete _dict;
    }

    virtual spot::state* get_init_state() const override {
        return new state_adapter<Q>(*_automaton->initial_state());
    }

    virtual spot::tgba_succ_iterator*
    succ_iter(const spot::state* local_state,
              const spot::state* global_state = 0,
              const spot::tgba* global_automaton = 0) const override {
        assert(dynamic_cast<const state_adapter<Q>*>(local_state));
        const state_adapter<Q> *lstate = static_cast<const state_adapter<Q> *>(local_state);
        const Q &q = lstate->state();
        return new succiter_adapter<Q, S, TS>((*_automaton->transition_system())(q).successors().begin(),
                                              (*_automaton->transition_system())(q).successors().end(),
                                              this);
    }

    virtual spot::bdd_dict* get_dict() const override { return _dict; }

    virtual std::string format_state(const spot::state* state) const override {
        assert(dynamic_cast<const state_adapter<Q> *>(state));
        const state_adapter<Q> *o = static_cast<const state_adapter<Q> *>(state);
        std::stringstream stmp;
        _automaton->transition_system()->print_state(stmp, o->state());
        return stmp.str();
    }

    virtual std::string
    transition_annotation(const spot::tgba_succ_iterator* t) const override {
        using iter_type = succiter_adapter<Q, S, TS>;
        assert(dynamic_cast<const iter_type *>(t));
        const iter_type *o = static_cast<const iter_type *>(t);
        std::stringstream stmp;
        _automaton->transition_system()->print_label(stmp, o->get_trans()->label());
        return stmp.str();
    }

    virtual bdd all_acceptance_conditions() const override {
        bdd result = bdd_true();
        for (std::size_t i = 0 ; i != _automaton->num_acceptance_sets() ; ++i) {
            auto it = _acc_bdd.find(i);
            assert(it != _acc_bdd.end());
            result = bdd_and(bdd_ithvar(it->second), result);
        }
        return result;
    }

    virtual bdd neg_acceptance_conditions() const override {
        bdd result = bdd_true();
        for (std::size_t i = 0 ; i != _automaton->num_acceptance_sets() ; ++i) {
            auto it = _acc_bdd.find(i);
            assert(it != _acc_bdd.end());
            result = bdd_and(bdd_not(bdd_ithvar(it->second)), result);
        }
        return result;
    }

    int accs_maps(std::size_t i) const {
        auto it = _acc_bdd.find(i);
        assert(it != _acc_bdd.end());
        return it->second;
    }

 private:
    /// the underlying counter automaton
    CounterAutomaton<Q, S, TS> *_automaton;
    /// the bdd dictionnary
    spot::bdd_dict *_dict;
    bool _delete_dict;
    /// maps CltlFormula (atomic propositions) to bdd variables
    std::map<CltlFormulaPtr, int> _ap_bdd;
    /// maps acceptance conditions to bdd variables
    std::map<std::size_t, int> _acc_bdd;

    /// Do the actual computation of tgba::support_conditions().
    virtual bdd compute_support_conditions(const spot::state* state) const override {
        bdd res = bddfalse;
        auto it = succ_iter(state);
        for (it->first() ; !it->done() ; it->next()) {
            res |= it->current_condition();
        }
        return res;
    }

    /// Do the actual computation of tgba::support_variables().
    virtual bdd compute_support_variables(const spot::state* state) const override {
        bdd res = bddtrue;
        auto it = succ_iter(state);
        for (it->first() ; !it->done() ; it->next()) {
            res &= bdd_support(it->current_condition());
        }
        return res;
    }
};

template<typename Q, typename S, template<typename, typename> class TS>
CA2tgba<Q, S, TS> * make_tgba(CounterAutomaton<Q, S, TS> *a) {
    return new CA2tgba<Q, S, TS>(a);
}

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_CA2TGBA_H_
