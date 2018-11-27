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
 * Program options handling.
 * Based on boost::program_options, the options are configured in Config.h.
 */

#ifndef Common_Properties_h
#define Common_Properties_h

#include <vector>
#include <string>
#include <any>
#include <mutex>
#include <boost/version.hpp>
#include <boost/any.hpp>
#include <Common/Logger.h>
#include <Common/Property.h>

// Required declarations for custom validators *before* including the header,
// otherwise no overloading would happen in standard conforming compilers.

/** Boost library. */
namespace boost {

/** Program options. */
namespace program_options {

void validate(boost::any &v, const std::vector<std::string> &values, ::int64_t *, int);
void validate(boost::any &v, const std::vector<std::string> &values, ::int32_t *, int);
void validate(boost::any &v, const std::vector<std::string> &values, ::uint16_t *, int);
void validate(boost::any &v, const std::vector<std::string> &values, double *, int);

// pre 1.35 vector<T> validate doesn't pickup user defined validate for T
#if BOOST_VERSION < 103500
template<typename T>
void validate(boost::any& v, const std::vector<std::string> &s, std::vector<T>*, int);
#endif

}} // namespace boost::program_options

#include <Common/Error.h>

#include <boost/program_options.hpp>


namespace boost { namespace program_options {

#if BOOST_VERSION < 103500
/** Implement validation function for vector<T>, which is not
 * implemented in boost prior to 1.35
 */
template<typename T>
void validate(boost::any& v, const std::vector<std::string> &s, std::vector<T>*, int) {
  if (v.empty())
      v = boost::any(std::vector<T>());

  std::vector<T>* tv = boost::any_cast<std::vector<T> >(&v);
  assert(NULL != tv);

  for (unsigned i = 0; i < s.size(); ++i) {
    try {
      // so we can pick up user defined validate for T here
      boost::any a;
      std::vector<std::string> sv;
      sv.push_back(s[i]);
      validate(a, sv, (T*)0, 0);
      tv->push_back(boost::any_cast<T>(a));
    }
    catch (const bad_lexical_cast &/*e*/) {
      boost::throw_exception(invalid_option_value(s[i]));
    }
  }
}
#endif // < boost 1.35

}} // namespace boost::program_options


namespace {  namespace Po = boost::program_options; } // local

// convenience/abbreviated accessors
#define HT_PROPERTIES_ABBR_ACCESSORS() \
  inline bool get_bool(const String &name)  { \
    return get<bool>(name); } \
  inline String get_str(const String &name)  { \
    return get<String>(name); } \
  inline Strings get_strs(const String &name)  { \
    return get<Strings>(name); } \
  inline uint16_t get_i16(const String &name)  { \
    return get<uint16_t>(name); } \
  inline int32_t get_i32(const String &name)  { \
    return get<int32_t>(name); } \
  inline int64_t get_i64(const String &name)  { \
    return get<int64_t>(name); } \
  inline Int64s get_i64s(const String &name)  { \
    return get<Int64s>(name); } \
  inline double get_f64(const String &name)  { \
    return get<double>(name); } \
  inline Doubles get_f64s(const String &name)  { \
    return get<Doubles>(name); } \
  inline bool get_bool(const String &name, bool default_value) \
     { return get<bool>(name, default_value); } \
  inline String get_str(const String &name, String &default_value) \
     { return get<String>(name, static_cast<String>(default_value)); } \
  inline String get_str(const String &name, String default_value) \
     { return get<String>(name, default_value); } \
  inline Strings get_strs(const String &name, Strings default_value) \
     { return get<Strings>(name, default_value); } \
  inline uint16_t get_i16(const String &name, uint16_t default_value) \
     { return get<uint16_t>(name, default_value); } \
  inline int32_t get_i32(const String &name, int32_t default_value) \
     { return get<int32_t>(name, default_value); } \
  inline int64_t get_i64(const String &name, int64_t default_value) \
     { return get<int64_t>(name, default_value); } \
  inline Int64s get_i64s(const String &name, Int64s &default_value) \
     { return get<Int64s>(name, default_value); } \
  inline double get_f64(const String &name, double default_value) \
     { return get<double>(name, default_value); } \
  inline Doubles get_f64s(const String &name, Doubles default_value) \
     { return get<Doubles>(name, default_value); }
  /*     \
  inline bool get_bool(const String &name, bool default_value) \
    const { return get<bool>(name, default_value); } \
  inline String get_str(const String &name, String default_value) \
    const { return get<String>(name, default_value); } \
  inline Strings get_strs(const String &name, Strings &default_value) \
    const { return get<Strings>(name, default_value); } \
  inline uint16_t get_i16(const String &name, uint16_t default_value) \
    const { return get<uint16_t>(name, default_value); } \
  inline int32_t get_i32(const String &name, int32_t default_value) \
    const { return get<int32_t>(name, default_value); } \
  inline int64_t get_i64(const String &name, int64_t default_value)  \
    const { return get<int64_t>(name, default_value); } \
  inline Int64s get_i64s(const String &name, Int64s default_value) \
    const { return get<Int64s>(name, default_value); } \
  inline double get_f64(const String &name, double default_value) \
    const { return get<double>(name, default_value); } \
  inline Doubles get_f64s(const String &name, Doubles default_value) \
    const { return get<Doubles>(name, default_value); }
  */
 

namespace Hypertable {

typedef Po::options_description PropertiesDesc;
typedef Po::positional_options_description PositionalDesc;

/** @addtogroup Common
 *  @{
 */


// Abbrs for some common types
inline Po::typed_value<bool> *boo(bool *v = 0) {
  return Po::value<bool>(v);
}

inline Po::typed_value<String> *str(String *v = 0) {
  return Po::value<String>(v);
}

inline Po::typed_value<Strings> *strs(Strings *v = 0) {
  return Po::value<Strings>(v);
}

inline Po::typed_value<uint16_t> *i16(uint16_t *v = 0) {
  return Po::value<uint16_t>(v);
}

inline Po::typed_value<int32_t> *i32(int32_t *v = 0) {
  return Po::value<int32_t>(v);
}

inline Po::typed_value<int64_t> *i64(int64_t *v = 0) {
  return Po::value<int64_t>(v);
}

inline Po::typed_value<Int64s> *i64s(Int64s *v = 0) {
  return Po::value<Int64s>(v);
}

inline Po::typed_value<double> *f64(double *v = 0) {
  return Po::value<double>(v);
}

inline Po::typed_value<Doubles> *f64s(Doubles *v = 0) {
  return Po::value<Doubles>(v);
}

/*
template<typename T>
inline Po::typed_value<T> *eNum(T *v = 0) {
  return Po::value<T>(v);
}
*/

/**
 * Manages a collection of program options.
 *
 * Implements getters and setters, aliases and default values.
 */
class Properties {
  typedef std::map<String, Property::ValuePtr> Map;
  typedef std::pair<String, Property::ValuePtr> MapPair;
  typedef std::pair<Map::iterator, bool> InsRet;
  typedef std::map<String, String> AliasMap;

public:
  /** Default constructor; creates an empty set */
  Properties()
    : m_need_alias_sync(false) {
  }

  /** Constructor; load properties from a filename
   *
   * @param filename The name of the file with the properties
   * @param desc A Property description with valid properties, default values
   * @param allow_unregistered If true, unknown/unregistered properties are
   *        accepted
   */
  Properties(const String &filename, const PropertiesDesc &desc,
             bool allow_unregistered = false)
      : m_need_alias_sync(false) {
    load(filename, desc, allow_unregistered);
  }

  /**
   * Loads a configuration file with properties
   *
   * @param filename The name of the configuration file
   * @param desc A property description
   * @param allow_unregistered If true, unknown/unregistered properties are
   *        accepted
   */
  void load(const String &filename, const PropertiesDesc &desc,
	  bool allow_unregistered = false);
  
  /**
   * ReLoads a configuration file with properties
   *
   * @param filename The name of the configuration file
   * @param desc A property description
   * @param allow_unregistered If true, unknown/unregistered properties are
   *        accepted
   */
  String reload(const String &filename, const PropertiesDesc &desc,
	  bool allow_unregistered = false);

  /**
   * Parses command line arguments
   *
   * @param argc The argument count (from main)
   * @param argv The argument array (from main)
   * @param desc The options description
   * @param hidden The hidden options description
   * @param p The positional options description
   * @param allow_unregistered If true, unknown/unregistered properties are
   *        accepted
   * @throw Error::CONFIG_INVALID_ARGUMENT on error
   */
  void parse_args(int argc, char *argv[], const PropertiesDesc &desc,
             const PropertiesDesc *hidden = 0, const PositionalDesc *p = 0,
             bool allow_unregistered = false);

  /**
   * Parses command line arguments
   *
   * @param args A vector of Strings with the command line arguments
   * @param desc The options description
   * @param hidden The hidden options description
   * @param p The positional options description
   * @param allow_unregistered If true, unknown/unregistered properties are
   *        accepted
   * @throw Error::CONFIG_INVALID_ARGUMENT on error
   */
  void parse_args(const std::vector<String> &args, const PropertiesDesc &desc,
             const PropertiesDesc *hidden = 0, const PositionalDesc *p = 0,
             bool allow_unregistered = false);

  /**
   * Get the ptr of property value. Throws if name is not defined.
   * 
   * @param name The name of the property
   * @throw Error::CONFIG_GET_ERROR if the requested property is not defined
   * @return Property::ValuePtr
   */
  Property::ValuePtr get_value_ptr(const String &name) {
    try{
      return m_map.at(name);
    }
    catch (std::exception &e) {
      HT_THROWF(Error::CONFIG_GET_ERROR, "getting value of '%s': %s",
                name.c_str(), e.what());
    }
    // 
    // return nullptr;
  }

  /**
   * Get the ptr of property value type. Throws if name is not defined.
   * Property::ValueDef<int32_t>* threads = props->get_type_ptr<int32_t>("threads");
   * 
   * @param name The name of the property
   * @throw Error::CONFIG_GET_ERROR if the requested property is not defined
   * @return Property::ValueDef<T>*
   */
  template <typename T>
  Property::ValueDef<T>* get_type_ptr(const String &name) {
    return (Property::ValueDef<T>*)get_value_ptr(name)->get_type_ptr();
  }

  /**
   * Get the String representation of property name 
   *
   * @param name The name of the property
   * @throw Error::CONFIG_GET_ERROR if the requested property is not defined
   * @return const String
   */
  const String str(const String &name) {
    try {
      return get_value_ptr(name)->str();
    }
    catch (std::exception &e) {
      HT_THROWF(Error::CONFIG_GET_ERROR, "getting value of '%s': %s",
                name.c_str(), e.what());
    }
  }
  /**
   * Get the value of option of type T. Throws if option is not defined.
   *
   * @param name The name of the property
   * @throw Error::CONFIG_GET_ERROR if the requested property is not defined
   */
  template <typename T>
  T get(const String &name) {
    try {
      return get_value_ptr(name)->get<T>();
    }
    catch (std::exception &e) {
      HT_THROWF(Error::CONFIG_GET_ERROR, "getting value of '%s': %s",
                name.c_str(), e.what());
    }
  }

  /**
   * Get the value of option of type T. Returns supplied default value if
   * not found. Try use the first form in usual cases and supply default
   * values in the config descriptions, as it validates the name and is less
   * error prone.
   * If no value, sets the default value to properties 
   *
   * @param name The name of the property
   * @param default_value The default value to return if not found
   */
  template <typename T>
  T get(const String &name, T default_value) {
    try {
      Map::const_iterator it = m_map.find(name);

      if (it != m_map.end())
        return (*it).second->get<T>();
      
      set(name, default_value, true);
      return get<T>(name);
    }
    catch (std::exception &e) {
      HT_THROWF(Error::CONFIG_GET_ERROR, "getting value of '%s': %s",
                name.c_str(), e.what());
    }
  }
  /*
  template <typename T>
  T get(const String &name, T &default_value)  {
    T v = get(name, default_value);
    return std::as_const(v);
  }
  */

  /**
   * Get the underlying boost::any value of 'name'
   *
   * @param name The name of the property

   */
  boost::any operator[](const String &name) {
    return get_value_ptr(name)->to_any();
  }

  /**
   * Check whether a property is by default value
   *
   * @param name The name of the property
   * @return true if the value is default
   */
  bool defaulted(const String &name) {
    return get_value_ptr(name)->defaulted();
  }

  /** Check whether a property exists
   *
   * @param name The name of the property
   * @return true if the property exists
   */
  bool has(const String &name) {
    return m_map.count(name);
  }
  bool has(const String &name) const {
    return m_map.count(name);
  }

  HT_PROPERTIES_ABBR_ACCESSORS()

  /**
   * Add property to the map of type T
   *
   * @param name The name of the property
   * @param v The value of the property
   * @param defaulted True if the value is default
   * @return An iterator to the new item
   */
  template<typename T>
  InsRet add(const String &name, T v, bool defaulted = false) {
    return m_map.insert(MapPair(name, new Property::Value(v, defaulted)));
  }
  /**
   * Add property to the map
   *
   * @param name The name of the property
   * @param v The Property::ValuePtr of the property
   * @param defaulted True if the value is default
   */
  void add(const String &name, Property::ValuePtr v) {
    m_map.insert(MapPair(name, v));
  }
  /**
   * Set a property in the map, create or update, of T type
   *
   * @param name The name of the property
   * @param v The value of the property, name need to correspond to Value Type
   * @param defaulted True if the value is default
   */
  template<typename T>
  void set(const String &name, T v, bool defaulted = false)  {
    InsRet r = add(name, v, defaulted);
    if (!r.second){
      m_need_alias_sync = true;
      (*r.first).second = new Property::Value(v, defaulted);
    } else 
      (*r.first).second->set_value(v);
  }

  /**
   * Remove a property from the map
   *
   * @param name The name of the property
   */
  void remove(const String &name) {
    m_map.erase(name);
  }

  /**
   * Setup an alias for a property.
   *
   * The primary property has higher priority, meaning when
   * aliases are sync'ed the primary value can override secondary value
   *
   * @param primary The primary property name
   * @param secondary The secondary property name
   * @param overwrite True if an existing alias should be overwritten
   */
  void alias(const String &primary, const String &secondary,
          bool overwrite = false);

  /**
   * Sync alias values.
   *
   * After this operation all properties that are aliases to each other
   * have the same value. Value priority: primary non-default >
   * secondary non-default > primary default > secondary default
   */
  void sync_aliases();

  /**
   * Returns all property names
   *
   * @param names reference to vector to hold names of all properties
   */
  void get_names(std::vector<String> &names) {
    for (Map::const_iterator it = m_map.begin(); it != m_map.end(); it++)
      names.push_back((*it).first);
  }

  /**
   * Prints keys and values of the configuration map
   *
   * @param out The output stream
   * @param include_default If true then default values are included
   */
  void print(std::ostream &out, bool include_default = false);

  /**
   * Append to String keys and values of the configuration map for print
   *   
   * @param out The String to append 
   * @param include_default If true then default values are included
   */
  void append_to_string(String &out, bool include_default = false);

  /**
   * Helper to print boost::any used by property values
   *
   * @param a Reference to the boost::any value
   * @return A string with the formatted value
   */
  static String to_str(const boost::any &a);

private:
  /** Whether the aliases need to be synced */
  bool m_need_alias_sync;

  /** The map containing all properties */
  Map m_map;
  
  /** A map with all aliases */
  AliasMap m_alias_map;
};

 typedef std::shared_ptr<Properties> PropertiesPtr;



/** Helper class to access parts of the properties.
 *
 * This snippet extracts all properties that start with
 * "Hypertable.RangeServer." and uses the SubProperty to access
 * "Hypertable.RangeServer.Range.SplitSize". 
 *
 *     SubProperties cfg(props, "Hypertable.RangeServer.");
 *     i64_t foo = cfg.get_i64("Range.SplitSize");
 */
class SubProperties {
public:
  /** Constructor
   *
   * @param props The original Properties object
   * @param prefix The common prefix of all sub-properties
   */
  SubProperties(PropertiesPtr &props, const String &prefix)
    : m_props(props), m_prefix(prefix) {
  }

  /** Returns the full name of a sub-property
   *
   * @param name The name of the sub-property
   * @return The full name (prefix + name)
   */
  String full_name(const String &name) const {
    return m_prefix + name;
  }

  /**
   * Calls Properties::get for a sub-property.
   *
   * @param name The name of the sub-property
   */
  template <typename T>
  T get(const String &name) {
    return m_props->get<T>(full_name(name));
  }

  /**
   * Calls Properties::get for a sub-property.
   *
   * @param name The name of the sub-property
   * @param default_value The default value to return if not found
   */
  template <typename T>
  T get(const String &name, T default_value) {
    return m_props->get<T>(full_name(name), default_value);
  }

  /**
   * Check whether a sub-property has a default value
   *
   * @param name The name of the sub-property
   * @return true if the value is default
   */
  bool defaulted(const String &name) {
    return m_props->defaulted(full_name(name));
  }

  /**
   * Check whether a sub-property exists
   *
   * @param name The name of the sub-property
   * @return true if the property exists
   */
  bool has(const String &name) {
    return m_props->has(full_name(name));
  }

  HT_PROPERTIES_ABBR_ACCESSORS()

private:
  /** Reference to the original Properties object */
  PropertiesPtr m_props;

  /** The common prefix of all sub-properties */
  String m_prefix;
};

/** @} */

}
#endif // Common_Properties_h
