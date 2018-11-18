/**
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

#include "Common/Compat.h"
#include "Common/Init.h"

using namespace Hypertable;

namespace {

struct AppPolicy : Config::Policy {
  static void init_options() {
	Config::cmdline_desc("Usage: %s [Options] [args]\nOptions").add_options()
      ("i16", Config::i16(), "16-bit integer")
      ("i32", Config::i32(), "32-bit integer")
      ("i64", Config::i64(), "64-bit integer")
      ("int64s", Config::i64s(), "a list of 64-bit integers")
      ("boo", Config::boo()->zero_tokens()->default_value(false), "a boolean arg")
      ("string", Config::str(), "a string arg")
      ("strs", Config::strs(), "a list of strings")
      ("float64", Config::f64(), "a double arg")
      ("f64s", Config::f64s(), "a list of doubles")
      ;
	Config::cmdline_hidden_desc().add_options()("args", Config::strs(), "arguments");
	Config::cmdline_positional_desc().add("args", -1);
  }
};

typedef Meta::list<AppPolicy, Config::DefaultPolicy> Policies;

} // local namespace

int main(int argc, char *argv[]) {
  Config::init_with_policies<Policies>(argc, argv);
  Config::properties->print(std::cout);
}
