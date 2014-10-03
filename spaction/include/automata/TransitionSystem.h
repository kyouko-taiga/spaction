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

#ifndef SPACTION_INCLUDE_TRANSITIONSYSTEM_H_
#define SPACTION_INCLUDE_TRANSITIONSYSTEM_H_

namespace spaction {
namespace automata {
    
template<typename S> class Transition;

template<typename S> class TransitionSystem {
public:
    virtual ~TransitionSystem() { }
    
    virtual void add_state(const std::string &name) = 0;
    virtual Transition<S> *add_transition(const std::string &source, const std::string &sink,
                                          const S &label) = 0;
    
    virtual bool has_state(const std::string &name) const = 0;
    
    virtual Transition<S> *find_transition(const std::string &source, const S &label) const = 0;
    
    virtual std::vector<Transition<S>*> find_all_transitions(const std::string &source,
                                                             const S &label) const = 0;
    
protected:
    virtual Transition<S> *_make_transition(const std::string &source, const std::string &sink,
                                            const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        return new Transition<S>(source, sink, label);
    }
};

template<typename S> class DeterministicTransitionSystem : public TransitionSystem<S> {
public:
    virtual ~DeterministicTransitionSystem() {}
    
    virtual void add_state(const std::string &name) {
        if (_graph.count(name) > 0) return;
        _graph[name];
    }
    
    virtual inline bool has_state(const std::string &name) const {
        return _graph.count(name) > 0;
    }
    
    virtual Transition<S> *add_transition(const std::string &source, const std::string &sink,
                                          const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        
        Transition<S> *t = this->_make_transition(source, sink, label);
        _graph[source][label] = t;
        return t;
    }
    
    virtual Transition<S> *find_transition(const std::string &source, const S &label) const {
        return this->_graph.at(source).at(label);
    }
    
    virtual std::vector<Transition<S>*> find_all_transitions(const std::string &source,
                                                             const S &label) const {
        return std::vector<Transition<S>*>({find_transition(source, label)});
    }
    
protected:
    std::unordered_map<std::string, std::unordered_map<S, Transition<S>*>> _graph;
};

template<typename S> class UndeterministicTransitionSystem : public TransitionSystem<S> {
public:
    virtual ~UndeterministicTransitionSystem() {}
    
    virtual void add_state(const std::string &name) {
        if (_graph.count(name) > 0) return;
        _graph[name];
    }
    
    virtual inline bool has_state(const std::string &name) const {
        return _graph.count(name) > 0;
    }
    
    virtual Transition<S> *add_transition(const std::string &source, const std::string &sink,
                                          const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        
        Transition<S> *t = this->_make_transition(source, sink, label);
        _graph[source][label].push_back(t);
        return t;
    }
    
    virtual Transition<S> *find_transition(const std::string &source, const S &label) const {
        return this->_graph.at(source).at(label).at(0);
    }
    
    virtual std::vector<Transition<S>*> find_all_transitions(const std::string &source,
                                                             const S &label) const {
        return this->_graph.at(source).at(label);
    }
    
protected:
    std::unordered_map<std::string, std::unordered_map<S, std::vector<Transition<S>*>>> _graph;
};

template <typename S>
class Transition {
public:
    const std::string &source() const { return _source; }
    const std::string &sink()   const { return _sink; }
    const S &label()       const { return _label; }
    
protected:
    friend class TransitionSystem<S>;
    
    const std::string _source;
    const std::string _sink;
    const S _label;
    
    explicit Transition(const std::string &source, const std::string &sink, const S &label) :
    _source(source), _sink(sink), _label(label) {
    }
    
private:
    /// Copy construction is forbidden.
    Transition(const Transition &) = delete;
    /// Copy assignement is forbidden.
    Transition &operator=(const Transition &) = delete;
};
    
}  // namespact automata
}  // namespact spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
