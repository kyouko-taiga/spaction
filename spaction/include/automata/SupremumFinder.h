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

#ifndef SPACTION_INCLUDE_AUTOMATA_SUPREMUMFINDER_H_
#define SPACTION_INCLUDE_AUTOMATA_SUPREMUMFINDER_H_

#include "automata/ConfigurationAutomaton.h"

#include "Logger.h"

namespace spaction {
namespace automata {

/// a class to compute the supremum in a configuration automaton
template<typename Q, typename S, template<typename, typename> class TS>
class SupremumFinder {
 public:
    /// constructor
    explicit SupremumFinder(MinMaxConfigurationAutomaton<Q, S, TS> &aut,
                            bool poprem): _automaton(aut), _poprem(poprem), _removed_components(0) {}

    /// compute the supremum by exploring the accepting SCC of the given configuration automaton
    /// by a variant of the Couvreur algorithm (FM99).
    /// A SCC in this automaton has a single value.
    /// Find the maximal value among all accepting SCC.
    /// @todo The SCC with low values (lower than the current max) should not be explored.
    /// @note   This implementation is derived from the implementation of Couvreur emptiness check
    ///         algo in spot. spot-related comments may remain in the code...
    value_t find_supremum(unsigned int bound) {
        // the searched value, initialized at 0 (since sup \emptyset = 0)
        unsigned int max_val = 0;

        // the number of visited nodes, used to set the order of each visited node.
        int num = 1;
        // the DFS stack
        std::stack<state_iter> todo;

        // setup DFS from the initial state
        {
            const MinMaxConfiguration<Q> &init = *_automaton.initial_state();
            auto insert_res = _h.emplace(init, num);
            assert(insert_res.second);  // ensures insertion did take place
            _root.push(scc_t(num));
            _arc.push(accs_t());
            auto tmp_init = (*_automaton.transition_system())(init);
            todo.push(state_iter(init, tmp_init.successors().begin(), tmp_init.successors().end()));
            // inc_depth();  // for stats
        }

        // counts the number of shortcuts: when we avoid exploring a SCC with a value lower than
        // the current candidate value.
        std::size_t number_shortcuts = 0;

        while (!todo.empty()) {
            //@debug
            //print_debug(std::cerr);

            // @todo merge _root and _arc, since they are always pushed and popped together
            assert(_root.size() == _arc.size());

            auto &succ = todo.top().iter;
            // if there is no more successors, backtrack
            if (! (succ != todo.top().iter_end)) {
                // we have explored all successors of state curr
                MinMaxConfiguration<Q> curr = todo.top().state;

                // Backtrack
                todo.pop();
                // dec_depth();  // for stats

                // If poprem is used, fill rem with any component removed,
                // so that remove_component() does not have to traverse
                // the SCC again.
                // first assert that curr is already in h
                auto spi = _h.find(curr);
                assert(spi != _h.end());

                if (_poprem)
                {
                    _root.top().rem.push_front(curr);
                    // inc_depth();  // for stats
                }
                // When backtracking the root of an SCC, we must also
                // remove that SCC from the ARC/ROOT stacks.  We must
                // discard from H all reachable states from this SCC.
                assert(!_root.empty());
                if (_root.top().index == spi->second)
                {
                    assert(!_arc.empty());
                    _arc.pop();
                    remove_component(curr);
                    _root.pop();
                }

                // delete succ;
                // Do not destroy CURR: it is a key in H.
                continue;
            }

            // We have a successor to look at.
            // inc_transitions();  // for stats
            // Fetch the values (destination state, acceptance conditions
            // of the arc) we are interested in...
            MinMaxConfiguration<Q> dest = succ.get_sink();
            accs_t acc = succ.get_label().get_acceptance();

            //{@logging
//            std::cerr << " ------- " << std::endl;
//            std::cerr << "current is ";
//            _automaton.transition_system()->print_state(std::cerr, (*succ)->source());
//            std::cerr << std::endl;
//            std::cerr << "success is ";
//            _automaton.transition_system()->print_state(std::cerr, dest);
//            std::cerr << std::endl << std::endl;
            //}

            // ... and point the iterator to the next successor, for
            // the next iteration.
            ++succ;
            // We do not need SUCC from now on.


            // A component with a value lower than the current one is not worth exploring.
            if (dest.is_bounded() and dest.current_value() <= max_val) {
                ++number_shortcuts;
                continue;
            }

            // Are we going to a new state?
            auto spit = _h.emplace(dest, num+1);
            if (spit.second)
            {
                // Yes, we are going to a new state.
                //  Number it, stack it, and register its successors for later processing.
                _root.push(scc_t(++num));
                _arc.push(acc);
                auto tmp_dest = (*_automaton.transition_system())(dest);
                todo.push(state_iter(dest, tmp_dest.successors().begin(), tmp_dest.successors().end()));
                // inc_depth();  // for stats

                continue;
            }

            //{@logging
//            std::cerr << "already encountered state, index = " << spit->second << std::endl;
//            std::cerr << "state = ";
//            _automaton.transition_system()->print_state(std::cerr, dest);
//            std::cerr << std::endl;
//            std::cerr << "found = ";
//            _automaton.transition_system()->print_state(std::cerr, spit->first);
//            std::cerr << std::endl << std::endl;
            //}

            // If we have reached a dead component, ignore it.
            if (spit.first->second == -1)
                continue;

            // Now this is the most interesting case.  We have reached a
            // state S1 which is already part of a non-dead SCC.  Any such
            // non-dead SCC has necessarily been crossed by our path to
            // this state: there is a state S2 in our path which belongs
            // to this SCC too.  We are going to merge all states between
            // this S1 and S2 into this SCC.
            //
            // This merge is easy to do because the order of the SCC in
            // ROOT is ascending: we just have to merge all SCCs from the
            // top of ROOT that have an index greater to the one of
            // the SCC of S2 (called the "threshold").
            int threshold = spit.first->second;
            std::list<MinMaxConfiguration<Q>> rem;
            while (threshold < _root.top().index)
            {
                assert(!_root.empty());
                assert(!_arc.empty());
                acc |= _root.top().conditions;
                acc |= _arc.top();
                rem.splice(rem.end(), _root.top().rem);
                _root.pop();
                _arc.pop();
            }
            // Note that we do not always have
            //  threshold == ecs_->root.top().index
            // after this loop, the SCC whose index is threshold might have
            // been merged with a lower SCC.

            // Accumulate all acceptance conditions into the merged SCC.
            _root.top().conditions |= acc;
            _root.top().rem.splice(_root.top().rem.end(), rem);


            // Have we found an accepting SCC?
            //{@logging
//            std::cerr << "SCC found, is it accepting?" << std::endl;
            //}
            if (_root.top().conditions.count() == _automaton.num_acceptance_sets())
            {

                // Yes, we have found an accepting SCC.
                // use it value to update our supremum (if bounded)
                if (dest.is_bounded()) {
                    max_val = dest.current_value() > max_val ? dest.current_value() : max_val;
                }
                //{@logging
                LOG_INFO << "accepting SCC encountered, its value is " << (dest.is_bounded()?dest.current_value():-1) << std::endl;
                LOG_INFO << "new candidate value is " << max_val << std::endl;
                LOG_INFO << "the given bound is " << bound << std::endl;
                //}
                // if unbounded, or if beyond the given bound, we have reached \infty
                if (!dest.is_bounded() or max_val > bound) {
                    // we are done, return
                    // release all iterators in TODO.
                    while (!todo.empty()) {
                        //@todo what is to release? is this loop necessary?
                        todo.pop();
                        // dec_depth();  // for stats
                    }
                    // @todo have only one return (and one log of the number of shortcuts)
                    LOG_INFO << "took " << number_shortcuts << " shortcuts" << std::endl;
                    return { true, 0 };
                }
                // @todo    compute a lasso that witnesses newly found value
            }
        }

        LOG_INFO << "took " << number_shortcuts << " shortcuts" << std::endl;

        // We are done exploring the configuration automaton, and a finite supremum has been found.
        assert(max_val <= bound);
        return { false, max_val };
    }

 private:
    MinMaxConfigurationAutomaton<Q, S, TS> &_automaton;
    bool _poprem;

    /// an internal struct to represent an SCC in the stack
    /// an SCC has an index (lowlink number)
    /// a set of accepting conditions
    /// a list of remaining states
    struct scc_t {
        explicit scc_t(int i = -1): index(i), conditions(accs_t()) {}

        int index;
        accs_t conditions;
        std::list<MinMaxConfiguration<Q>> rem;
    };

    /// a pair state/iterator in the stack representing the current DFS path
    /// to test whether the iterator is done, we have to store the end iterator as well
    struct state_iter {
        explicit state_iter(const MinMaxConfiguration<Q> &s,
                            typename MinMaxConfigTS<Q, CounterLabel<S>, TS>::TransitionIterator &&i,
                            typename MinMaxConfigTS<Q, CounterLabel<S>, TS>::TransitionIterator &&ie)
        : state(s)
        , iter(std::move(i))
        , iter_end(std::move(ie))
        {}

        MinMaxConfiguration<Q> state;
        typename MinMaxConfigTS<Q, CounterLabel<S>, TS>::TransitionIterator iter;
        typename MinMaxConfigTS<Q, CounterLabel<S>, TS>::TransitionIterator iter_end;
    };

    // a stack of SCC
    std::stack<scc_t> _root;
    // a stack of acceptance conditions between SCC
    std::stack<accs_t> _arc;
    // a hash of states
    std::unordered_map<MinMaxConfiguration<Q>, int> _h;

    // A logging function that prints the current stacks
    // @todo incorporate it properly into a logging mechanism
    void print_debug(std::ostream &os) {
        os << std::endl;
        std::stack<scc_t> root2;
        std::stack<accs_t> arc2;
        assert(_root.size() == _arc.size());
        while (!_root.empty()) {
            os << "(" << _root.top().index << " ";
            for (auto it : _h) {
                if (it.second == _root.top().index) {
                    _automaton.transition_system()->print_state(os, it.first);
                }
            }
            os << "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t|";

            for (auto i : _root.top().conditions.sets())
                os << i << ",";
            os << "\t\tarc : ";
            for (auto i : _arc.top().sets())
                os << i << ",";
            os << ")" << std::endl;

            root2.push(_root.top());
            arc2.push(_arc.top());
            _root.pop();
            _arc.pop();
        }
        os << "***********" << std::endl << std::endl;

        while (!root2.empty()) {
            _root.push(root2.top());
            _arc.push(arc2.top());
            root2.pop();
            arc2.pop();
        }
    }

    unsigned _removed_components;

    void remove_component(const MinMaxConfiguration<Q> &from) {
        ++_removed_components;
        // If rem has been updated, removing states is very easy.
        if (_poprem)
        {
            assert(!_root.top().rem.empty());
            //            dec_depth(_root.top()rem.size());  // for stats
            typename std::list<MinMaxConfiguration<Q>>::iterator i;
            for (i = _root.top().rem.begin(); i != _root.top().rem.end(); ++i)
            {
                auto spit = _h.find(*i);
                assert(spit != _h.end());
                assert(spit->second != -1);
                spit->second = -1;
            }
            // ecs_->root.rem().clear();
            return;
        }

        // Remove from H all states which are reachable from state FROM.

        // Stack of iterators towards states to remove.
        // @todo    get this struct def outside of the body of the function
        using iterator_type = decltype((*_automaton.transition_system())(from).successors().begin());
        struct my_iterator_pair {
        private:
            iterator_type _begin, _end;

        public:
            explicit my_iterator_pair(iterator_type &&b, iterator_type &&e)
            : _begin(std::move(b))
            , _end(std::move(e))
            {}

            const iterator_type & begin() const { return _begin; }
            const iterator_type & end() const { return _end; }
        };
        std::stack<my_iterator_pair> to_remove;


        // Remove FROM itself, and prepare to remove its successors.
        // (FROM should be in H, otherwise it means all reachable
        // states from FROM have already been removed and there is no
        // point in calling remove_component.)
        auto spit = _h.find(from);
        assert(spit != _h.end());
        assert(spit->second != -1);
        spit->second = -1;
        auto ts = _automaton.transition_system();
        auto tmp = (*ts)(from);
        my_iterator_pair succs(tmp.successors().begin(), tmp.successors().end());
        
        for (;;) {
            for (iterator_type i = succs.begin() ; i != succs.end() ; ++i) {
                //                inc_transitions();  // for stats
                
                MinMaxConfiguration<Q> s = i.get_sink();
                auto spi = _h.find(s);
                
                // This state is not necessarily in H, because if we were doing inclusion checking
                // during the emptiness-check (refining find()), the index `s' can be included in a
                // larger state and will not be found by index(). We can safely ignore such states.
                if (spi == _h.end())
                    continue;
                
                if (spi->second != -1) {
                    spi->second = -1;
                    auto tmp = (*ts)(s);
                    my_iterator_pair topush(tmp.successors().begin(), tmp.successors().end());
                    to_remove.push(topush);
                }
            }
            if (to_remove.empty())
                break;
            
            succs = to_remove.top();
            to_remove.pop();
        }
    }
    
    
    // @note depth for stats
    int _depth;
    
    void inc_depth() {
        ++_depth;
    }
    
    void dec_depth() {
        assert(_depth != 0);
        --_depth;
    }
};

template<typename Q, typename S, template<typename, typename> class TS>
SupremumFinder<Q, S, TS>
make_sup_comput(MinMaxConfigurationAutomaton<Q, S, TS> &aut) {
    return SupremumFinder<Q, S, TS>(aut, false);
}

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_SUPREMUMFINDER_H_
