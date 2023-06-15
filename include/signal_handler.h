/*
MIT License

Copyright (c) 2023 https://github.com/yuriiq

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef SignalHandler_H
#define SignalHandler_H

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

/*
 * Use this macros to declare signal 
 * Example: SIGNAL(signal_name, param_type1, param_type2, param_typeN);
 */
#define SIGNAL(name, ...) \
void name (__VA_ARGS__) {}

/*
 * Use this macros to emit signal
 * Example: EMIT(&ClassName::signal_name, param_value1, param_value2, param_valueN);
 */
#define EMIT(name, ...) \
  ParentT::signal_emit(name, __VA_ARGS__); \

/*
 * Use this macros to declare object class
 * Example: 
 * class ClassName : BEGIN_OBJECT(ClassName)
 * ......
 * END_OBJECT
 */

#define BEGIN_OBJECT(class_name) public SignalHandler<class_name> { \
  public: \
  using ParentT = SignalHandler<class_name>; \
  private: \


#define END_OBJECT() };

/*
 * Helper struct for type checking 
 */
template<class T>
struct MethodInfo;

template<class C, class R, class... A>
struct MethodInfo<R(C::*)(A...)> //method pointer
{
    using ClassType = C;
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...> ;
};

template<class C, class R, class... A>
struct MethodInfo<R(C::*)(A...) const> : MethodInfo<R(C::*)(A...)> {}; //const method pointer

/*
 * Helper type definitions
 */
using ResiverPtr = void*;
using SenderPtr = void*;
using SlotPtr = void*;
using SignalPtr = void*;
using ResiverData = std::unordered_map<ResiverPtr, std::unordered_set<SlotPtr> >;

/*
 * This class is needed in order to handle the system of signals and slots
 */
template<typename ObjectT>
class SignalHandler {
public:
  std::unordered_map<SignalPtr, ResiverData> m_connected_slots; // signal to slots
  SignalHandler(const SignalHandler&) = delete;
  SignalHandler& operator= (const SignalHandler&) = delete;

  SignalHandler() {}
  ~SignalHandler() {
    signal_emit(&SignalHandler::deleted, static_cast<ObjectT*>(this));
  }

/*
 * This signal is emitted during the destructor call
 */
  SIGNAL(deleted, SenderPtr);

/*
 * Use this method to connect signal to slot 
 * @param signal pointer to signal method 
 * @param resiver object with slot method
 * @param slot pointer to slot method 
 * @note The signal and slot parameters must be the same.
 */
  template<typename SenderArgT, typename ResiverT, typename ResiverArgT>
  void connect(SenderArgT signal, ResiverT & resiver, ResiverArgT slot) {
    connect(*this, signal, resiver, slot);
  }

/*
 * Use this method to connect signal to slot (no resiver)
 * @param signal pointer to signal method 
 * @param slot pointer to slot method 
 * @note The signal and slot parameters must be the same.
 */
  template<typename SenderArgT, typename ResiverArgT>
  void connect(SenderArgT signal, ResiverArgT slot) {
    connect(*this, signal, slot);
  }

/*
 * Use this method to connect signal to slot 
 * @param sender object with signal method 
 * @param signal pointer to signal method 
 * @param resiver object with slot method
 * @param slot pointer to slot method 
 * @note The signal and slot parameters must be the same.
 */
  template<typename SenderT, typename SenderArgT, typename ResiverT, typename ResiverArgT>
  static void connect(SenderT & sender, SenderArgT signal, ResiverT & resiver, ResiverArgT slot) {
    _connect(sender, signal, resiver, slot);
    _connect(resiver, &ResiverT::deleted, static_cast<typename SenderT::ParentT&>(sender), &SenderT::disconnect_all);
  }

/*
 * Use this method to connect signal to slot (no resiver)
 * @param sender object with signal method 
 * @param signal pointer to signal method 
 * @param slot pointer to slot method 
 * @note The signal and slot parameters must be the same.
 */
  template<typename SenderT, typename SenderArgT, typename ResiverArgT>
  static void connect(SenderT & sender, SenderArgT signal, ResiverArgT slot) {
    _connect(sender, signal, slot);
  }

/*
 * Use this method to disconnect signal to slot 
 * @param sender object with signal method 
 * @param signal pointer to signal method 
 * @param resiver object with slot method
 * @param slot pointer to slot method 
 * @note The signal and slot parameters must be the same.
 */
  template<typename SenderT, typename SenderArgT, typename ResiverT, typename ResiverArgT>
  static void disconnect(SenderT & sender, SenderArgT signal, ResiverT & resiver, ResiverArgT slot) {
    check_signal_slot_types<SenderT, SenderArgT, ResiverT, ResiverArgT>();
    ResiverPtr l_resiver = &resiver;
    SignalPtr l_signal = reinterpret_cast<SignalPtr&>(signal);
    SlotPtr l_slot = reinterpret_cast<SlotPtr&>(slot);

    auto & l_connected_slots = sender.m_connected_slots[l_signal][l_resiver];
    l_connected_slots.erase(l_slot);    
  }

/*
 * Use this method to disconnect all signals and slots associated with this object
 * @param sender pointer to object with signal methods
 */
  void disconnect_all(SenderPtr sender){
    for (auto & signals: m_connected_slots) {
      signals.second.erase(sender);
    }
  }

protected:
  template<typename SignalT, typename ... ArgsT>
  void signal_emit(SignalT signal, ArgsT && ... params) {
    using MI = MethodInfo<SignalT>;
    SignalPtr l_signal = reinterpret_cast<SignalPtr&>(signal);
    for (const auto & data: this->m_connected_slots[l_signal]) {
        typename MI::ClassType * resiver = 0;
        reinterpret_cast<void*&>(resiver) = data.first;
        for (const auto & slot: data.second) {
          resiver->template slot_handle <SignalT>(slot, params...);
        }
    }
  }

private:
  template<typename SlotT, class ... Args>
  void slot_handle(void * slot, Args && ... params) {
      SlotT f_ptr = 0;
      reinterpret_cast<void*&>(f_ptr) = slot;
      (static_cast<ObjectT*>(this)->*f_ptr)(params...);
  }

  template<typename SenderT, typename SenderArgT, typename ResiverT, typename ResiverArgT>
  static inline void check_signal_slot_types() {
    static_assert(std::is_same<typename MethodInfo<ResiverArgT>::ClassType, ResiverT>::value
                  || std::is_base_of<typename MethodInfo<ResiverArgT>::ClassType, ResiverT>::value,
                  "Resiver type error");
    static_assert(std::is_same<typename MethodInfo<SenderArgT>::ClassType, SenderT>::value
                  || std::is_base_of<typename MethodInfo<SenderArgT>::ClassType, SenderT>::value,
                  "Sender type error");

    static_assert(std::is_same<typename MethodInfo<SenderArgT>::ArgsTuple, typename MethodInfo<ResiverArgT>::ArgsTuple>::value,
                  "Signal and slot have different arguments");
  }

  template<typename SenderT, typename SenderArgT, typename ResiverT, typename ResiverArgT>
  static void _connect(SenderT & sender, SenderArgT signal, ResiverT & resiver, ResiverArgT slot) {
    check_signal_slot_types<SenderT, SenderArgT, ResiverT, ResiverArgT>();
    ResiverPtr l_resiver = &resiver;
    SignalPtr l_signal = reinterpret_cast<SignalPtr&>(signal);
    SlotPtr l_slot = reinterpret_cast<SlotPtr&>(slot);
    sender.m_connected_slots[l_signal][l_resiver].insert(l_slot);
  }
};

#endif
