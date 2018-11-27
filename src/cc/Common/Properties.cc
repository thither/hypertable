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

#include <Common/Compat.h>
#include <Common/Properties.h>

#include <errno.h>
#include <fstream>

using namespace Hypertable;

// Custom validator defintions
namespace boost { namespace program_options {

/*
// EnumExt object validator/converter //
template<typename T>
void validate(boost::any &v, const Strings &values, T *, int) {
  const std::string &s = validators::get_single_string(values);
  T tChk = T();

  if(!tChk.valid(s))
    throw invalid_option_value(s + ": enum name not correspond to options");
  v = any(tChk.value(s));
}
*/
void validate(boost::any &v, const Strings &values, ::int64_t *, int) {
  validators::check_first_occurrence(v);
  const std::string &s = validators::get_single_string(values);
  char *last;
  int64_t result = strtoll(s.c_str(), &last, 0);

  if (s.c_str() == last)
    throw invalid_option_value(s);

  switch (*last) {
    case 'k':
    case 'K': result *= 1000LL;         break;
    case 'm':
    case 'M': result *= 1000000LL;      break;
    case 'g':
    case 'G': result *= 1000000000LL;   break;
    case '\0':                          break;
    default: throw invalid_option_value(s + ": unknown suffix");
  }
  v = any(result);
}

void validate(boost::any &v, const Strings &values, double *, int) {
  validators::check_first_occurrence(v);
  const std::string &s = validators::get_single_string(values);
  char *last;
  double result = strtod(s.c_str(), &last);

  if (s.c_str() == last)
    throw invalid_option_value(s);

  switch (*last) {
    case 'k':
    case 'K': result *= 1000LL;         break;
    case 'm':
    case 'M': result *= 1000000LL;      break;
    case 'g':
    case 'G': result *= 1000000000LL;   break;
    case '\0':                          break;
    default: throw invalid_option_value(s + ": unknown suffix");
  }
  v = any(result);
}

void validate(boost::any &v, const Strings &values, ::int32_t *, int) {
  validate(v, values, (int64_t *)0, 0);
  int64_t res = any_cast<int64_t>(v);

  if (res > INT32_MAX || res < INT32_MIN) {
    const std::string &s = validators::get_single_string(values);
    throw invalid_option_value(s +": number out of range of 32-bit integer");
  }
  v = any((int32_t)res);
}

void validate(boost::any &v, const Strings &values, ::uint16_t *, int) {
  validate(v, values, (int64_t *)0, 0);
  int64_t res = any_cast<int64_t>(v);

  if (res > UINT16_MAX) {
    const std::string &s = validators::get_single_string(values);
    throw invalid_option_value(s + ": number out of range of 16-bit integer");
  }
  v = any((uint16_t)res);
}

}} // namespace boost::program_options


namespace {

void
parse(Po::command_line_parser &parser, const PropertiesDesc &desc,
      Po::variables_map &result, const PropertiesDesc *hidden,
      const PositionalDesc *p, bool allow_unregistered) {
  try {
    PropertiesDesc full(desc);

    if (hidden)
      full.add(*hidden);

    parser.options(full);

    if (p)
      parser.positional(*p);

#if BOOST_VERSION >= 103500
  if (allow_unregistered)
    store(parser.allow_unregistered().run(), result);
  else
    store(parser.run(), result);
#else
  store(parser.run(), result);
#endif
  }
  catch (std::exception &e) {
    HT_THROW(Error::CONFIG_BAD_ARGUMENT, e.what());
  }
}

} // local namespace


void
Properties::load(const String &fname, const PropertiesDesc &desc,
                 bool allow_unregistered) {
  m_need_alias_sync = true;

  try {
    std::ifstream in(fname.c_str());

    if (!in)
      HT_THROWF(Error::CONFIG_BAD_CFG_FILE, "%s", strerror(errno));

    Po::variables_map parser_map;
#if BOOST_VERSION >= 103500
    Po::parsed_options parsed_opts = parse_config_file(in, desc, allow_unregistered);
    store(parsed_opts, parser_map);
    
    for (const auto &kv : parser_map) {
      if(has(kv.first)&&kv.second.defaulted())
        continue;
      set(kv.first, kv.second.value(), kv.second.defaulted());
    }
    if(allow_unregistered){
      for (size_t i = 0; i < parsed_opts.options.size(); i++) {
        if (parsed_opts.options[i].unregistered && parsed_opts.options[i].string_key != "")
          set(parsed_opts.options[i].string_key, parsed_opts.options[i].value[0], false);
      }
    }
#else
    store(parse_config_file(in, desc), parser_map);
#endif

  }
  catch (std::exception &e) {
    HT_THROWF(Error::CONFIG_BAD_CFG_FILE, "%s: %s", fname.c_str(), e.what());
  }
}

String 
Properties::reload(const String &fname, const PropertiesDesc &desc, 
                   bool allow_unregistered) {
	m_need_alias_sync = true;
  String out;
	try {
		std::ifstream in(fname.c_str());

    HT_INFOF("Reloading Configuration File %s", fname.c_str());

		if (!in){
      HT_WARNF("Error::CONFIG_BAD_CFG_FILE error: %s", strerror(errno));
      return format("Error::BAD CFG FILE, error: %s", strerror(errno));
    }
    out.append("\n\nCurrent Configurations:\n");
    append_to_string(out, true);

    out.append("\n(parsing)\n");
    
    Po::variables_map parser_map;
    store(parse_config_file(in, desc, allow_unregistered), parser_map);

    out.append("\n(parsed)\n");

    out.append("\n\nUpdated Configurations: \n");
    
    for (const auto &kv : parser_map) {
      const String name = kv.first;
      out.append(format("name=%s\n", name.c_str()));
			if (!has(name))
        continue;

      Property::ValuePtr v_ptr(get_value_ptr(name));
      out.append(format("v_ptr->str=%s\n", v_ptr->str().c_str()));

      // compare current value to boost::any , v_ptr->compare(boost::any nv&)
      Property::Value vn(kv.second.value(), false);
      out.append(format("old=%s new=%s\n", v_ptr->str().c_str(), vn.str().c_str()));
      if(v_ptr->str().compare(vn.str()) == 0) {
        continue;
      }
      String old_value = v_ptr->str();
      v_ptr->set_value(kv.second.value());

      out.append(format("%s => new(%s) old(%s)\n", name.c_str(), 
                 v_ptr->str().c_str(), old_value.c_str()));

      set(name, kv.second.value(), false);
    }

    out.append("\n\nNew Configurations:\n");
    append_to_string(out, true);
    return out;
	}
	catch (std::exception &e) {
		HT_WARNF("Error::CONFIG_BAD_CFG_FILE %s: %s", fname.c_str(), e.what());
    return format("Error::CONFIG_BAD_CFG_FILE %s: %s \noutput:\n%s", 
                    fname.c_str(), e.what(), out.c_str());
	}
}

void
Properties::parse_args(int argc, char *argv[], const PropertiesDesc &desc,
                       const PropertiesDesc *hidden, const PositionalDesc *p,
                       bool allow_unregistered) {
  // command_line_parser can't seem to handle argc = 0
  const char *dummy = "_";

  if (argc < 1) {
    argc = 1;
    argv = (char **)&dummy;
  }
  HT_TRY("parsing arguments",
    Po::command_line_parser parser(argc, argv);
    
  Po::variables_map parser_map;
  parse(parser, desc, parser_map, hidden, p, allow_unregistered);
  for (const auto &kv : parser_map) {
    if(has(kv.first)&&kv.second.defaulted())
      continue;
    set(kv.first, kv.second.value(), kv.second.defaulted());
  }
  );
}

void
Properties::parse_args(const std::vector<String> &args,
                       const PropertiesDesc &desc, const PropertiesDesc *hidden,
                       const PositionalDesc *p, bool allow_unregistered) {
  HT_TRY("parsing arguments",

  Po::variables_map parser_map;
  Po::command_line_parser parser(args);
  parse(parser, desc, parser_map, hidden, p, allow_unregistered);
  for (const auto &kv : parser_map) {
    if(has(kv.first)&&kv.second.defaulted())
      continue;
    set(kv.first, kv.second.value(), kv.second.defaulted());
  }
  );
}


void Properties::alias(const String &primary, const String &secondary,
                       bool overwrite) {
  if (overwrite)
    m_alias_map[primary] = secondary;
  else
    m_alias_map.insert(std::make_pair(primary, secondary));

  m_need_alias_sync = true;
}

void Properties::sync_aliases() {
  if (!m_need_alias_sync)
    return;

  for (const auto &v : m_alias_map) {
    Map::iterator it1 = m_map.find(v.first);
    Map::iterator it2 = m_map.find(v.second);

    if (it1 != m_map.end()) {
      if (it2 == m_map.end())           // secondary missing
        add(v.second, (*it1).second);
      else if (!(*it1).second->defaulted())
        (*it2).second = (*it1).second;  // non-default primary trumps
      else if (!(*it2).second->defaulted())
        (*it1).second = (*it2).second;  // otherwise use non-default secondary
    }
    else if (it2 != m_map.end()) {      // primary missing
      add(v.first, (*it2).second);
    }
  }
  m_need_alias_sync = false;
}

// need to be updated when adding new option type
String Properties::to_str(const boost::any &v) {
  if (v.type() == typeid(String))
    return boost::any_cast<String>(v);

  if (v.type() == typeid(uint16_t))
    return format("%u", (unsigned)boost::any_cast<uint16_t>(v));

  if (v.type() == typeid(int32_t))
    return format("%d", boost::any_cast<int32_t>(v));

  if (v.type() == typeid(int64_t))
    return format("%llu", (Llu)boost::any_cast<int64_t>(v));

  if (v.type() == typeid(double))
    return format("%g", boost::any_cast<double>(v));

  if (v.type() == typeid(bool)) {
    bool bval = boost::any_cast<bool>(v);
    return bval ? "true" : "false";
  }
  if (v.type() == typeid(Strings)) {
    const Strings *strs = boost::any_cast<Strings>(&v);
    return format_list(*strs);
  }
  if (v.type() == typeid(Int64s)) {
    const Int64s *i64s = boost::any_cast<Int64s>(&v);
    return format_list(*i64s);
  }
  if (v.type() == typeid(Doubles)) {
    const Doubles *f64s = boost::any_cast<Doubles>(&v);
    return format_list(*f64s);
  }

  return "invalid option type";
}

void
Properties::print(std::ostream &out, bool include_default) {
  for (const auto &kv : m_map) {
    bool isdefault = kv.second->defaulted();

    if (include_default || !isdefault) {
      out << kv.first.c_str() << '=' << kv.second->str().c_str();

      if (isdefault)
        out << " (default)";
      out << std::endl;
    }
  }
}

void
Properties::append_to_string(String &out, bool include_default) {
  for (const auto &kv : m_map) {
    bool isdefault = kv.second->defaulted();

    if (include_default || !isdefault) {
      out.append(format("%s=%s", kv.first.c_str(), kv.second->str().c_str()));

      if (isdefault)
        out.append(" (default)");
      out.append("\n");
    }
  }
}

