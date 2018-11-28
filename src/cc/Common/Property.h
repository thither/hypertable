/*
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/** @file
 * A Property Value used with Properties 
 */

#ifndef Common_Property_h
#define Common_Property_h

#include <vector>
#include <string>
#include <any>
#include <atomic>
#include <mutex>
#include <boost/version.hpp>
#include <boost/any.hpp>
#include <Common/Logger.h>

namespace Hypertable {

/** @addtogroup Common
 *  @{
 */

const uint64_t K = 1000;
const uint64_t KiB = 1024;
const uint64_t M = K * 1000;
const uint64_t MiB = KiB * 1024;
const uint64_t G = M * 1000;
const uint64_t GiB = MiB * 1024;

typedef std::vector<String> Strings;
typedef std::vector<int64_t> Int64s;
typedef std::vector<double> Doubles;

namespace Property {

/*
class EnumExt {
    operator EnumExt*(){
      return this;
    }
};
*/

enum ValueType {
  UNKNOWN,
  DOUBLE,
  BOOL,
  STRING,
  UINT16_T,
  INT32_T,
  INT64_T,
  STRINGS,
  INT64S,
  DOUBLES,
  ENUMEXT,
  ENUM
};

inline const ValueType get_value_type(const std::type_info &v_type){
  if(v_type == typeid(double))
    return ValueType::DOUBLE;
  if(v_type == typeid(bool))
    return ValueType::BOOL;  
  if(v_type == typeid(String))
    return ValueType::STRING;  
  if(v_type == typeid(uint16_t))
    return ValueType::UINT16_T;  
  if(v_type == typeid(int32_t))
    return ValueType::INT32_T;  
  if(v_type == typeid(int64_t))
    return ValueType::INT64_T;  
  if(v_type == typeid(Strings))
    return ValueType::STRINGS;  
  if(v_type == typeid(Int64s))
    return ValueType::INT64S;  
  if(v_type == typeid(Doubles))
    return ValueType::DOUBLES;
  /*
  if(v_type == typeid(EnumExt))
    return ValueType::ENUMEXT;
  */
  return ValueType::UNKNOWN;
}


/**
* Property::TypeDef
* a Helper class for casting ValueDef<T> on a single type Pointer
* with a hold for corresponding ValueType enum
 */
class TypeDef {
  public:
    ValueType get_type()  { return typ; }
    void set_type(const ValueType t){ 
      if(typ != t)
        typ = t;  
    }
    virtual ~TypeDef() {}
  private:
    ValueType typ;
};

/**
* Property::ValueDef
* the Class handle the specific requirments to the 
* value type
 */
template <class T>
class  ValueDef : public TypeDef {
  public:
    static constexpr bool is_enum() {
      return std::is_enum<T>::value;
    }
    ValueDef(T nv) { 
      if(is_enum())
        set_type(ValueType::ENUM);
      else
        set_type(get_value_type(typeid(T)));
      set_value(nv);  
    }
    void set_value(T nv)  { v = nv;         }
    T get_value()         { return v;       }

    T* get_ptr()          { return &v;      }

    operator TypeDef*()   { return this;    }
    virtual ~ValueDef()   {}

  private:
    std::atomic<T> v;
};
template <>
class  ValueDef<String> : public TypeDef {
  public:
    ValueDef(String nv) { 
      set_type(ValueType::STRING);
      set_value(nv);  
    }
    void set_value(String nv)  { v = nv;         }
    String get_value()         { return v;       }
    String* get_ptr()          { return &v;      }
  private:
    String v;
};
template <>
class  ValueDef<Strings> : public TypeDef {
  public:
    ValueDef(Strings nv) { 
      set_type(ValueType::STRINGS);
      set_value(nv);  
    }
    void set_value(Strings nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    Strings get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Strings* get_ptr()          { return &v;      }
  private:
    std::mutex mutex;
    Strings v;
};
template <>
class  ValueDef<Doubles> : public TypeDef {
  public:
    ValueDef(Doubles nv) { 
      set_type(ValueType::DOUBLES);
      set_value(nv);  
    }
    void set_value(Doubles nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    Doubles get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Doubles* get_ptr()          { return &v;      }
  private:
    std::mutex mutex;
    Doubles v;
};
template <>
class  ValueDef<Int64s> : public TypeDef {
  public:
    ValueDef(Int64s nv) { 
      set_type(ValueType::INT64S);
      set_value(nv);  
    }
    void set_value(Int64s nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    Int64s get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Int64s* get_ptr()          { return &v;      }
  private:
    std::mutex mutex;
    Int64s v;
};
/**
* Property::Value 
* holds:
*   * the pointer to TypeDef and short forms to ValueDef operations
*   * the whether the value is a default value
 */
class Value {
  public:
    template<typename T>
    Value(T v, bool defaulted) {
      m_defaulted = defaulted;
      set_value(v);
      // HT_INFOF("Value: type=%s value=%s", typeid(v).name(), str().c_str());
    }
    // update/set the new value to the ValueDef<T>
    template<typename T>
    void set_value(T v) {
      if(type_ptr == nullptr){
        type_ptr = new ValueDef<T>(v);
        // HT_INFOF("set_value new type: %s, value: %s", typeid(v).name(), str().c_str());
        return;
      } 
      ((ValueDef<T>*)type_ptr)->set_value(v);
      // m_defaulted = false;
      //HT_INFOF("set_value chg type: %s, value: %s", typeid(v).name(), str().c_str());
    }
    template<typename T>
    T* get_ptr() {
      return ((ValueDef<T>*)type_ptr)->get_ptr();
    }
    
    void set_value(boost::any v){
      // HT_INFOF("setting value-type=%s", v.type().name());
      switch(get_value_type(v.type())){
        case ValueType::DOUBLE:
          set_value(boost::any_cast<double>(v));
          return;
        case ValueType::BOOL: 
          set_value(boost::any_cast<bool>(v));
          return;
        case ValueType::STRING:
          set_value(boost::any_cast<String>(v));
          return;
        case ValueType::UINT16_T:
          set_value(boost::any_cast<uint16_t>(v));
          return;
        case ValueType::INT32_T:
          set_value(boost::any_cast<int32_t>(v));
          return;
        case ValueType::INT64_T:
          set_value(boost::any_cast<int64_t>(v));
          return;
        case ValueType::STRINGS:
          set_value(boost::any_cast<Strings>(v));
          return;
        case ValueType::INT64S:
          set_value(boost::any_cast<Int64s>(v));
          return;
        case ValueType::DOUBLES:
          set_value(boost::any_cast<Doubles>(v));
          return;
        default:
          return;
          // HT_WARN("Property::Value::set_value(boost::any): (UNKNOWN VALUE TYPE)");
      }
    }

    template<typename T>
    T get() {
      //HT_INFOF("T get(): type=%s", typeid(T).name());
      if (type_ptr == nullptr)
        HT_THROWF(Error::CONFIG_GET_ERROR, 
                  "T get(): type=%s (UNKNOWN VALUE TYPE)", typeid(T).name());;

      return ((ValueDef<T>*)type_ptr)->get_value();
    }
    bool defaulted() {
      return m_defaulted;
    }
    TypeDef* get_type_ptr() {
      return type_ptr;
    }

    /* call will require a <type>
    template<typename T>
    boost::any to_any() {
      return get<T>();
    }
    */
    inline boost::any to_any(){ 
      if (type_ptr == nullptr)
        return "invalid option type";
      
      switch(type_ptr->get_type()){
        case ValueType::DOUBLE:
          return get<double>();
        default:
          return "invalid option type";
      }
    }

    inline String str(){
      if (type_ptr == nullptr)
        return "invalid option type";
      
      switch(type_ptr->get_type()){
        case ValueType::STRING:
          return get<String>();
        case ValueType::BOOL: 
          return get<bool>() ? "true" : "false";;
        case ValueType::DOUBLE:
          return format("%g", get<double>());
        case ValueType::UINT16_T:
          return format("%u", (unsigned)get<uint16_t>());
        case ValueType::INT32_T:
          return format("%d", get<int32_t>());
        case ValueType::INT64_T:
          return format("%ld", get<int64_t>());
        case ValueType::STRINGS:
          return format_list(get<Strings>());
        case ValueType::INT64S:
          return format_list(get<Int64s>());
        case ValueType::DOUBLES:
          return format_list(get<Doubles>());
        case ValueType::ENUM:
          return "An ENUM TYPE";
        default:
          return "invalid option type";
      }
    }

    virtual ~Value() {}

  private:
    TypeDef* type_ptr = nullptr; //
    std::atomic<bool> m_defaulted;
};
 
typedef Value* ValuePtr;

} // namespace Property

/** @} */

}

#endif // Common_Property_h
