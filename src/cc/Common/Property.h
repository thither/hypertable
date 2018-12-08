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
#include <atomic>
#include <mutex>
#include <algorithm>

#include <Common/Compat.h>
#include <Common/Logger.h>
#include <Common/Error.h>

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

const ValueType get_value_type(const std::type_info &v_type);

/**
 * Convertors & Validators from String
*/
double double_from_string(String s);
int64_t int64_t_from_string(String s);
uint16_t uint16_t_from_string(String s);
int32_t int32_t_from_string(String s);

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
    static constexpr bool is_enum() {  return std::is_enum<T>::value; }

    ValueDef(ValueType typ, Strings values, T defaulted) {
      set_type(typ);
      if(!values.empty())
        from_strings(values);
      else
        set_value(defaulted);
    }

    ValueDef(T nv) { 
      if(is_enum())
        set_type(ValueType::ENUM);
      else
        set_type(get_value_type(typeid(T)));
      set_value(nv);  
    }

    void set_value(T nv) { v.store(nv);         }

    void from_strings(Strings values) {};

    T get_value()         { return v.load();       }

    T* get_ptr()          { return &v;      }

    std::ostream& operator<<(std::ostream& ostream) {
      return ostream << str();
    }
    
    String str(){
      switch(get_type()){
        case ValueType::BOOL: 
          return get_value() ? "true" : "false";;
        case ValueType::DOUBLE:
          return format("%g", get_value());
        case ValueType::UINT16_T:
          return format("%u", (unsigned)get_value());
        case ValueType::INT32_T:
          return format("%d", get_value());
        case ValueType::INT64_T:
          return format("%ld", get_value());
        case ValueType::ENUM:
          return "An ENUM TYPE";
        default:
          return "invalid option type";
      }
    }
    operator TypeDef*()   { return this;    }
    // ValueDef<T> operator *()   { return this;    }
    virtual ~ValueDef()   {}

  private:
    std::atomic<T> v;
};
template <>
class  ValueDef<String> : public TypeDef {
  public:

    ValueDef(ValueType typ, Strings values, String defaulted) {
      set_type(typ);
      if(!values.empty())
        from_strings(values);
      else
        set_value(defaulted);
    }

    ValueDef(String nv) { 
      set_type(ValueType::STRING);
      set_value(nv);  
    }
    void set_value(String nv)  {
       v = nv;       
         }
    void from_strings(Strings values){
      set_value(values.back());
    }
    String get_value()         { return v;       }
    String* get_ptr()          { return &v;      }
    String str()               { return get_value(); }
  private:
    String v;
};
template <>
class  ValueDef<Strings> : public TypeDef {
  public:

    ValueDef(ValueType typ, Strings values, Strings defaulted) {
      set_type(typ);
      if(!values.empty())
        from_strings(values);
      else
        set_value(defaulted);
    }
    ValueDef(Strings nv) { 
      set_type(ValueType::STRINGS);
      set_value(nv);  
    }
    void set_value(Strings nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    void from_strings(Strings values){
      set_value(values);
    };

    Strings get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Strings* get_ptr()  { return &v;      }
    String str()        { return format_list(get_value()); }
  private:
    std::mutex mutex;
    Strings v;
};
template <>
class  ValueDef<Doubles> : public TypeDef {
  public:
    ValueDef(ValueType typ, Strings values, Doubles defaulted) {
      set_type(typ);
      if(!values.empty())
        from_strings(values);
      else
        set_value(defaulted);
    }
    ValueDef(Doubles nv) { 
      set_type(ValueType::DOUBLES);
      set_value(nv);  
    }
    void set_value(Doubles nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    void from_strings(Strings values){
      Doubles value;
      for(String s: values)
        value.push_back(double_from_string(s));
      set_value(value);
    };
    Doubles get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Doubles* get_ptr()  { return &v;      }
    String str()        { return format_list(get_value()); }
  private:
    std::mutex mutex;
    Doubles v;
};
template <>
class  ValueDef<Int64s> : public TypeDef {
  public:
    ValueDef(ValueType typ, Strings values, Int64s defaulted) {
      set_type(typ);
      if(!values.empty())
        from_strings(values);
      else
        set_value(defaulted);
    }
    ValueDef(Int64s nv) { 
      set_type(ValueType::INT64S);
      set_value(nv);  
    }
    void set_value(Int64s nv)  {
      std::lock_guard<std::mutex> lock(mutex);
      v = nv;         
    }
    void from_strings(Strings values){
      Int64s value;
      for(String s: values)
        value.push_back(int64_t_from_string(s));
      set_value(value);
    };
    Int64s get_value() { 
      std::lock_guard<std::mutex> lock(mutex);
      return v;       
    }
    Int64s* get_ptr()  { return &v;      }
    String str()        { return format_list(get_value()); }
  private:
    std::mutex mutex;
    Int64s v;
};

template <>
void ValueDef<bool>::from_strings(Strings values);
template <>
void ValueDef<double>::from_strings(Strings values);
template <>
void ValueDef<uint16_t>::from_strings(Strings values);
template <>
void ValueDef<int32_t>::from_strings(Strings values);
template <>
void ValueDef<int64_t>::from_strings(Strings values);

/**
* Property::Value 
* holds:
*   * the pointer to TypeDef and short forms to ValueDef operations
*   * the whether the value is a default value
 */

class Value;
typedef Value* ValuePtr;

class Value {
  public:

    template<typename T>
    Value(T v, bool skippable=false) {
      m_skippable = skippable;
      set_value(v);
    }
    /* init from (TypeDef*)ValueDef<T> */
    Value(TypeDef* v) {
      type_ptr = v;
    }
    
    // update/set the new value to the ValueDef<T>
    template<typename T>
    void set_value(T v) {
      if(type_ptr == nullptr){
        type_ptr = new ValueDef<T>(v);
        return;
      } 
      ((ValueDef<T>*)type_ptr)->set_value(v);
    }

    void set_value_from(ValuePtr from){
      switch(get_type()){
        case ValueType::STRING:
          return set_value(from->get<String>());
        case ValueType::BOOL: 
          return set_value(from->get<bool>());
        case ValueType::DOUBLE:
          return set_value(from->get<double>());
        case ValueType::UINT16_T:
          return set_value(from->get<uint16_t>());
        case ValueType::INT32_T:
          return set_value(from->get<int32_t>());
        case ValueType::INT64_T:
          return set_value(from->get<int64_t>());
        case ValueType::STRINGS:
          return set_value(from->get<Strings>());
        case ValueType::INT64S:
          return set_value(from->get<Int64s>());
        case ValueType::DOUBLES:
          return set_value(from->get<Doubles>());
        case ValueType::ENUM:
        default:
          HT_THROWF(Error::CONFIG_GET_ERROR,
                    "Bad Type %s", str().c_str());
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

    template<typename T>
    T* get_ptr() {
      return ((ValueDef<T>*)type_ptr)->get_ptr();
    }

    TypeDef* get_type_ptr() {
      return type_ptr;
    }

    ValueType get_type() {
      return type_ptr->get_type();
    }
    
    String str(){
      if (type_ptr == nullptr)
        return "nullptr";
      
      switch(get_type()){
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
    
    /* a Default Value */
    ValuePtr default_value(bool defaulted=true){
      m_defaulted = defaulted;
      return *this;
    }
    bool is_skippable() {
      return m_skippable;
    }
    bool is_default() {
      return m_defaulted;
    }
    
    /* a Zero Token Type */
    ValuePtr zero_token(){
      m_no_token = true;
      return *this;
    }
    bool is_zero_token(){
      return m_no_token;
    }

    operator Value*()   { return this;    }
    virtual ~Value() {}

  private:
    TypeDef* type_ptr = nullptr; //

    std::atomic<bool> m_defaulted = false;
    std::atomic<bool> m_no_token = false;
    std::atomic<bool> m_skippable = false;
    

};
 
ValuePtr make_new(ValuePtr p, Strings values = Strings());



} // namespace Property

/** @} */

} // namespace Hypertable

#endif // Common_Property_h
