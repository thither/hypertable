/*
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3 of the
 * License, or any later version.
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

#include <Common/Compat.h>

#include <Tools/htck/MetadataDumpReader.h>
#include <Tools/htck/MetadataRepairWriter.h>
#include <Tools/htck/RsmlReader.h>
#include <Tools/htck/TableIdentifierMap.h>

#include <Hypertable/RangeServer/MetaLogEntityRange.h>
#include <Hypertable/RangeServer/MetaLogDefinitionRangeServer.h>

#include <Hypertable/Master/MetaLogDefinitionMaster.h>

#include <Hypertable/Lib/Key.h>
#include <Hypertable/Lib/LoadDataEscape.h>
#include <Hypertable/Lib/MetaLog.h>
#include <Hypertable/Lib/MetaLogReader.h>

#include <Hyperspace/Session.h>

#include <FsBroker/Lib/Config.h>
#include <FsBroker/Lib/Client.h>

#include <AsyncComm/Comm.h>
#include <AsyncComm/ConnectionManager.h>

#include <Common/Error.h>
#include <Common/FileUtils.h>
#include <Common/FlyweightString.h>
#include <Common/InetAddr.h>
#include <Common/Logger.h>
#include <Common/Init.h>
#include <Common/ScopeGuard.h>
#include <Common/System.h>
#include <Common/Time.h>
#include <Common/Usage.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <set>

using namespace Hypertable;
using namespace Hyperspace;
using namespace Config;
using namespace std;

namespace {

  Hyperspace::SessionPtr hyperspace;
  ConnectionManagerPtr conn_manager;
  String toplevel_dir;
  StringSet valid_locations;
  StringSet valid_tids;
  LocationEntitiesMapT entities_map;
  FsBroker::Lib::Client *dfs = 0;
  bool gVerbose = false;

  const char *usage =
    R"(
Usage: ht htck <mode> <metadata-file> [options]

The htck program is used to check the integrity (metadata consistency) of a
Hypertable database.  If an integrity problem is detected, the tool can also be
used to repair it.

PREPARING TO RUN

  You must perform the following steps prior to running htck:

  1. Stop all client activity (esp. insert activity).

  2. Run `ht wait-until-quiesced` to wait for Hypertable to get into an idle
     state.  For example:

    $ ht wait-until-quiesced
    16d27cbad8dd528dd6745fef491dfb4f metadata.tsv
    16d27cbad8dd528dd6745fef491dfb4f metadata.tsv

    As a side-effect, the `ht wait-until-quiesced` command will generate a
    'metadata.tsv' file in the current working directory that contains a dump
    of the sys/METADATA after the system enters the idle state.  This file will
    be used as input to the htck program.

  3. Stop Hypertable

    $ ht cluster stop

  Once Hypertable has stopped completely, the next step is to restart Hyperspace
  and start an FS broker on the machine from which you plan to run htck.  For
  example:

    $ ht cluster start_hyperspace
    $ ht start-fsbroker hadoop  # or local, ...

CHECKING INTEGRITY

  To check for integrity problems, run htck as follows, using the 'metadata.tsv'
  file that was generated by the `ht wait-until-quiesced` command.

    $ ht htck check metadata.tsv

  If htck detects no integrity problems, it will generate output that looks
  something like the following (entry counts may differ):

    METADATA entries: 100977
    RSML range entities: 100977
    RSML dangling entities: 0
    METADATA overlaps:  0
    METADATA gaps:  0
    METADATA incomplete:  0
    RSML dangling fix-ups: 0
    METADATA shrunk ranges:  0
    Unassigned locations: 0
    Unassigned start row: 0
    Unassigned entities: 0
    Load acknowledge fix-ups: 0

  If htck does detect an integrity problem, it will produce output that
  describes the problems that it encountered.  For example:

    METADATA entries: 100976
    RSML range entities: 100977
    RSML dangling entities: 1
    Error: GAP in metadata range found
    last end row:   2/81:ur7H6vYc7tekOLBCte+aPsGA==_388659_2097371132442
    next start row: 2/81:ur7HdsMGe7Z/wApbmVezq28g_502360009_2604183617513
    METADATA overlaps:  0
    METADATA gaps:  1
    METADATA incomplete:  0
    RSML dangling fix-ups: 0
    METADATA shrunk ranges:  0
    RSML dangling entries:
    2/81[ur7H6vYc7tekOLBCte+aPsGA==_388659_2097371132442..ur7HdsMGe7Z/wApbmVezq28g_502360009_2604183617513] at rs211
    Unassigned locations: 0
    Unassigned start row: 0
    Unassigned entities: 0
    Load acknowledge fix-ups: 0

REPAIRING INTEGRITY

  To repair integrity problems that were discovered with `htck check`, you can
  run htck again, but replace the word "check" with "repair".  For example:

    $ ht htck repair metadata.tsv

  The above command will generate a file 'metadata.repair.tsv' in the current
  working directory.  This file will need to be loaded via LOAD DATA INFILE as
  soon as the system is brought back up.  For example:

    $ ht cluster start

  After a few seconds, the system should have loaded the sys/METADATA ranges, at
  which point you can load the 'metadata.repair.tsv' file (e.g. in another
  terminal window):

    $ echo "USE sys; LOAD DATA INFILE 'metadata.repair.tsv' INTO TABLE METADATA;" | ht shell --batch

  At this point, once the system comes up fully, it should be in a consistent
  state.  To double-check, rerun the `ht wait-until-quiesced` command followed
  by `htck check` and verify that no integrity problems are detected.

Options)";
  struct AppPolicy : Policy {
    static void init_options() {
      cmdline_desc(usage).add_options()
        ("all", "Display all entities in log (not just latest state)")
        ("metadata-tsv", "For each Range, dump StartRow and Location .tsv lines")
        ("location", "Used with --metadata-tsv to specify location proxy")
        ("show-version", "Display log version number and exit")
        ("verify-dfs", "Verifies DFS cellstores")
        ;
      cmdline_hidden_desc().add_options()
        ("mode", str(), "'check' or 'repair'")
        ("mode", 1)
        ("metadata-file", str(), "metadata dump file")
        ("metadata-file", 1)
        ("hadoop-file", str(), "dump file of `hadoop -lsr /hypertable/tables`")
      ;
    }
    static void init() {
      if (!has("mode")) {
        cout << "ERROR: <mode> required, try --help for usage" << endl;
        exit(1);
      }
      else if (!has("metadata-file")) {
        cout << "<metadata-file> required, try --help for usage" << endl;
        exit(1);
      }
      if (has("verbose"))
	      gVerbose = get<gBool>("verbose");
    }
  };

  typedef Meta::list<AppPolicy, FsClientPolicy, DefaultCommPolicy> Policies;

#if 0
  void write_metadata_repair(FILE *fp, const char *table_id, 
          const char *end_row, size_t end_row_len, 
          const char *column, const char *value, size_t value_len) {
    LoadDataEscape escaper;
    const char *escaped_str;
    size_t escaped_len;

    fwrite(table_id, strlen(table_id), 1, fp);
    fwrite(":", 1, 1, fp);
    escaper.escape(end_row, end_row_len, &escaped_str, &escaped_len);
    fwrite(escaped_str, escaped_len, 1, fp);
    fwrite("\t", 1, 1, fp);
    fwrite(column, strlen(column), 1, fp);
    fwrite("\t", 1, 1, fp);
    escaper.escape(value, value_len, &escaped_str, &escaped_len);
    fwrite(escaped_str, escaped_len, 1, fp);
    fwrite("\n", 1, 1, fp);
  }
#endif

} // local namespace

std::set<String> read_hadoop(const String &fname)
{
  std::set<String> ret;
  char *buffer = new char [10*1024*1024];
  FILE *fp;

  if ((fp = fopen(fname.c_str(), "r")) == 0) {
    String msg = format("Unable to open '%s' for reading", fname.c_str());
    perror(msg.c_str());
    exit(1);
  }

  unsigned lineno = 1;
  while (fgets(buffer, 10*1024*1024, fp)) {
    // skip directories
    if (buffer[0] == 'd')
      continue;

    char *p = &buffer[0] + strlen(&buffer[0]) - 1;
    *p = '\0'; // strip \n
    while (*p != ' ' && *p != '\t')
      --p;
    // skip "/hypertable/tables/"
    p += 20;

    // skip hints files
    if (strstr(p, "/hints"))
      continue;

    ret.insert(p);
    // cout << p << endl;

    lineno++;
  }

  delete[] buffer;
  fclose(fp);
  return ret;
}

void read_metadata_dump(FlyweightString &flyweight, const String &metadata,
                        RangeInfoSet &riset_all,
			MetadataRepairWriter &repair_writer) {
  MetadataDumpReader mdreader(metadata, valid_locations, valid_tids,
			      flyweight, repair_writer);
  mdreader.swap_riset(riset_all);

  /**
   * Read ROOT range info from Hyperspace and add to riset_all
   */
  {
    DynamicBuffer value(0);
    uint64_t root_handle = hyperspace->open(toplevel_dir + "/root", 
            OPEN_FLAG_READ);
    hyperspace->attr_get(root_handle, "Location", value);
    RangeInfo *rinfo = new RangeInfo();
    rinfo->table_id = "0/0";
    rinfo->set_start_row("", 0);
    rinfo->set_end_row(Key::END_ROOT_ROW, strlen(Key::END_ROOT_ROW));
    rinfo->location = flyweight.get((const char *)value.base);
    rinfo->got_location = true;
    hyperspace->close(root_handle);
    riset_all.insert(rinfo);
  }
}

String get_cellstore_directory(const String &path)
{
  String dir = path;
  while (dir[dir.size() - 1] != '/')
    dir.resize(dir.size() - 1);
  return dir;
}

void handle_verifydfs()
{
  RangeInfoSet riset_all;
  FlyweightString flyweight;
  String mode = get_str("mode");
  String metadata_dump_fname = get_str("metadata-file");
  String hadoopls = get_str("hadoop-file");

  cout << "verify dfs: " << mode << ", " << metadata_dump_fname << ", "
       << hadoopls << endl;

  // first read the hadoop file
  std::set<String> dfscs = read_hadoop(hadoopls);

  HT_ASSERT(!"Needs to be fixed to handle access groups!");

  MetadataRepairWriter repair_writer;

  // then read the metadata dump
  read_metadata_dump(flyweight, metadata_dump_fname, riset_all, repair_writer);

  // for each cellstore in the metadata: remove it from the hadoop dump
  for (RangeInfoSet::iterator it = riset_all.begin(); it != riset_all.end();
          ++it) {
    if (!(*it)->got_start_row || !(*it)->got_location) {
      cout << "RangeInfo is missing row or location - please repair " 
          << endl << "METADATA with htck before proceeding." << endl;
      exit(-1);
    }
    // (*it)->println();

    for (const auto &file : (*it)->files)
      dfscs.erase(file);

  }

  // all remaining cellstores in the hadoop dump will now be written to
  // the 'repair' file
  if (mode == "check") {
    cout << "Number of cellstores missing in METADATA: " << dfscs.size() 
         << endl;
    vector<String> warn;
    for (std::set<String>::iterator it = dfscs.begin(); it != dfscs.end(); 
            ++it) {
      if ((*it).c_str() == strstr((*it).c_str(), "0/0/"))
        warn.push_back(*it);
      else
        cout << "Missing cellstore: " << *it << endl;
    }
    if (warn.size()) {
      cout << endl << "  WARNING WARNING WARNING" << endl
           << "The following cellstores were skipped b/c they are metadata:" 
           << endl;
      for (const auto &s : warn)
        cout << s << endl;
      cout << endl << warn.size() << " warning(s)" << endl;
    }
  }
  else {
    int warnings = 0;
    int written = 0;
    RangeInfoSet repaired;

    // for each CellStore that is NOT in the METADATA
    for (std::set<String>::iterator csit = dfscs.begin(); csit != dfscs.end(); 
            ++csit) {
      if ((*csit).c_str() == strstr((*csit).c_str(), "0/0/")) {
        warnings++;
        continue;
      }
      if (!strstr((*csit).c_str(), "default")) {
        cout << "Error: CellStores in different access groups are not yet "
             << "handled: " << *csit << endl;
        exit(-1);
      }

      // get the directory of this cellstore, i.e. "0/0/default/asdfhasdjhf"
      // and find the existing ri-entry; if this entry does not exist then
      // there's a gap in the metadata and the normal htck must be run first;
      // otherwise add this cellstore to the file list of this entry and
      // store the entry in 'repaired'
      String dir = get_cellstore_directory(*csit);
      bool found = false;
      for (RangeInfoSet::iterator rit = riset_all.begin(); 
              rit != riset_all.end(); ++rit) {
        if ((*rit)->files.empty())
          continue;
        if (dir == get_cellstore_directory((*rit)->files[0])) { 
          (*rit)->files.push_back(*csit);
          repaired.insert(*rit);
          if (found)
            cout << "CellStore " << *csit << " used by multiple ranges" << endl;
          found = true;
          // continue - the same cellstore can be used by several Ranges
        }
      }
      if (!found) {
        cout << "Unable to find METADATA entry for cellstore " << *csit << endl;
        // exit(1);
      }
    }

    // finally write all RangeInfos in the 'repaired' vector; all cellstores
    // are concatenated with ";\\\n"
    for (RangeInfoSet::iterator rit = repaired.begin(); 
            rit != repaired.end(); ++rit) {
      String cs;
      //cout << "Repairing " << (*rit)->to_str() << endl;
      for (const auto &s : (*rit)->files) {
        cs += s;
        cs += ";\n"; // actually it's '\\' 'n' but write_metadata_repair will
                     // escape this correctly
      }

      // fixme !!!
      //write_metadata_repair(repair_fp, (*rit)->table_id, 
      //(*rit)->end_row, (*rit)->end_row_len, 
      //"Files:default", cs.c_str(), cs.size());

      written++;
    }

    cout << "Wrote " << written << " cellstores" << endl; 
    if (warnings) {
      cout << "Finished with " << warnings << " warnings; run in 'check' mode "
           << "to see why" << endl;
    }
  }
}


void handle_default()
{
  String mode = get_str("mode");
  String metadata_dump_fname = get_str("metadata-file");
  RangeInfo rkey;
  RangeInfo *rinfo;
  RangeInfoSet riset_all;
  RangeInfoSet riset_all_tmp;
  RangeInfoSetEndRow riset_all_endrow;
  RangeInfoSetEndRow metadata_gaps;
  RangeInfoSetEndRow riset_incomplete;
  RangeInfoSet metadata_overlaps;
  RangeInfoSet dangling_rsml;
  RangeInfoSet::iterator all_iter;
  RangeInfoSetEndRow::iterator dangling_iter, incomplete_iter;
  pair<RangeInfoSet::iterator, bool> ret;
  TableIdentifierMap tidmap;
  int64_t split_size = properties->get_i64("Hypertable.RangeServer.Range.SplitSize");
  int64_t timestamp = get_ts64();
  FlyweightString flyweight;
  int64_t dangling_rsml_repair_count = 0;
  int64_t shrunk_ranges = 0;
  int64_t metadata_incomplete_count = 0;
  size_t rsml_count = 0;

  MetadataRepairWriter repair_writer;

  FilesystemPtr fs(dfs);

  // read the metalog dump
  read_metadata_dump(flyweight, metadata_dump_fname, riset_all, repair_writer);

  cout << "\nMETADATA entries: " << riset_all.size() << endl;

  // - insert incomplete records into riset_incomplete
  // - remove incomplete entries from riset_all
  {
    auto iter = riset_all.begin();
    while (iter != riset_all.end()) {
      if (!(*iter)->got_start_row || !(*iter)->got_location) {
        riset_incomplete.insert(*iter);
        auto rm_iter = iter++;
        riset_all.erase(*rm_iter);
      }
      else
        ++iter;
    }
  }

  RsmlReader rsml_reader(fs, valid_locations, valid_tids, tidmap, riset_all, entities_map, &rsml_count);

  cout << "RSML range entities: " << rsml_count << endl;

  //
  // "Dangling RSML Entity" [DEFINITION]
  //  : An entity discovered in an RSML file that has no matching row in the
  //    METADATA table.
  //
  // For each dangling RSML entity, see if there is an entry in the METADATA
  // incomplete set that matches on the following attributes:
  //
  //   - Table ID
  //   - End row
  //
  // If an incomplete METADATA entry that is found, fix the entry by setting the
  // StartRow and/or Location to the correspponding values from the dangling
  // RSML.  Then move the completed entry back into riset_all and remove the
  // dangling entity
  //
  {
    RangeInfoSetEndRow::iterator rm_iter;

    rsml_reader.swap_dangling_riset(dangling_rsml);
    cout << "RSML dangling entities: " << dangling_rsml.size() << endl;
    dangling_iter = dangling_rsml.begin();
    while (dangling_iter != dangling_rsml.end()) {

      rkey.table_id = (*dangling_iter)->table_id;
      rkey.end_row = (char *)(*dangling_iter)->end_row;
      rkey.end_row_len = (*dangling_iter)->end_row_len;

      if ((incomplete_iter = riset_incomplete.find(&rkey)) != riset_incomplete.end()) {
        rinfo = 0;

        if (!(*incomplete_iter)->got_location) {
	  repair_writer.write_location(*dangling_iter);
          if (!(*incomplete_iter)->got_start_row)
	    repair_writer.write_start_row(*dangling_iter);
          rinfo = *incomplete_iter;
          (*incomplete_iter)->entity = (*dangling_iter)->entity;
          rinfo->location = flyweight.get((*dangling_iter)->location);
          rinfo->got_location = true;
        }
        else if (!(*incomplete_iter)->got_start_row) {
	  repair_writer.write_start_row(*dangling_iter);
	  rinfo = *incomplete_iter;
          (*incomplete_iter)->entity = (*dangling_iter)->entity;
          rinfo->set_start_row((*dangling_iter)->start_row, (*dangling_iter)->start_row_len);
        }
	else
	  HT_FATALF("METADATA entry encountered (%s:%s) with no StartRow or Location",
		    rkey.table_id, rkey.end_row);

	dangling_rsml_repair_count++;
	rm_iter = dangling_iter++;
	dangling_rsml.erase(*rm_iter);

	// Move back to good set
	riset_all.insert(*incomplete_iter);
	riset_incomplete.erase(incomplete_iter);

      }
      else
        ++dangling_iter;
    }
  }

  //
  // Move remaining incomplete entries back into riset_all
  //
  metadata_incomplete_count = riset_incomplete.size();
  for (auto ri : riset_incomplete)
    riset_all.insert(ri);

  //
  // Remove overlaps
  //
  // An overlap is defined as a range whose start row is less than the end row
  // of the previous range.  For example:
  //
  // 2:baz	StartRow	bar
  // 2:foo	StartRow	bar
  //
  // Overlapping ranges are removed from riset_all and inserted into
  // metadata_overlaps.
  //
  RangeInfo *last = 0;
  RangeInfoSet::iterator iter = riset_all.begin();
  while (iter != riset_all.end()) {
    if (last && (*iter)->got_start_row &&
        !strcmp((*iter)->table_id, last->table_id)) {
      int cmp = memcmp((*iter)->start_row, last->end_row,
                          min((*iter)->start_row_len, last->end_row_len));
      if (cmp < 0 || (cmp == 0 && (*iter)->start_row_len < last->end_row_len)) {
        ret = metadata_overlaps.insert(*iter);
        if (ret.second == false)
          cout << "Error duplicate OVERLAP metadata range found" << endl;
        RangeInfoSet::iterator rm_iter = iter++;
        riset_all.erase(rm_iter);
	continue;
      }
    }
    last = *iter;
    iter++;
  }

  //
  // Identify gaps
  //
  // A gap is defined as the area between two consecutive ranges in the METADATA
  // table where the end row of the first range is less than the start row of
  // the second range.  For example:
  //
  // 2:banana	StartRow	apple
  // [gap]
  // 2:orange	StartRow	cherry
  //
  // A new RangeInfo object is created for each gap and inserted into the
  // metadata_gaps set.
  //
  last = 0;
  for (auto ri : riset_all) {
    if (last && ri->got_start_row && !strcmp(ri->table_id, last->table_id)) {
      int cmp = memcmp(ri->start_row, last->end_row,
                          min(ri->start_row_len, last->end_row_len));
      if (cmp > 0 || (cmp == 0 && ri->start_row_len > last->end_row_len)) {
        cout << "Error: GAP in metadata range found" << endl;
        cout << "  last end row:   " << last->table_id << ":" << last->end_row << endl;
        cout << "  next start row: " << ri->table_id << ":" << ri->start_row << endl;
        RangeInfo *gap = new RangeInfo();
        gap->set_start_row(last->end_row, last->end_row_len);
        gap->set_end_row(ri->start_row, ri->start_row_len);
        gap->table_id = last->table_id;
        ret = metadata_gaps.insert(gap);
        if (ret.second == false)
          cout << "Error duplicate GAP metadata range found" << endl;
      }
    }
    last = ri;
  }

  cout << "METADATA overlaps:  " << metadata_overlaps.size() << endl;
  if (gVerbose) {
    for (auto ri : metadata_overlaps)
      cout << "  " << ri->to_str() << "\n";
  }

  cout << "METADATA gaps:  " << metadata_gaps.size() << endl;
  if (gVerbose) {
    for (auto ri : metadata_gaps)
      cout << "  " << ri->to_str() << "\n";
  }

  cout << "METADATA incomplete:  " << metadata_incomplete_count << endl;
  if (gVerbose) {
    for (auto ri : riset_incomplete)
      cout << "  " << ri->to_str() << "\n";      
  }

  cout << "RSML dangling fix-ups: " << dangling_rsml_repair_count << endl;

  //
  // Shrink overlaps
  //
  // Since there is only one start row for each METADATA entry, overlaps
  // must share an end row with a gap.  For each overlapping range,
  // reset the start row to be that of the corresponding gap.  This is
  // accomplished by writing a StartRow entry in metadata.repair.tsv.
  //
  // For each such overlap, the corresponding entries are removed from
  // metadata_overlaps and metadata_gaps.
  //
  for (auto overlap : metadata_overlaps) {

    rkey.table_id = overlap->table_id;
    rkey.end_row = (char *)overlap->end_row;
    rkey.end_row_len = overlap->end_row_len;

    auto gap_iter = metadata_gaps.find(&rkey);

    if (gap_iter != metadata_gaps.end()) {

      shrunk_ranges++;

      // The following lines have the effect of shrinking the overlapping range
      // in the METADATA table such that it exactly covers the gap.
      overlap->set_start_row((*gap_iter)->start_row, (*gap_iter)->start_row_len);
      repair_writer.write_start_row(overlap);

      if (overlap->entity)
	overlap->entity->set_start_row((*gap_iter)->start_row);

      riset_all.insert(overlap);
      metadata_gaps.erase(gap_iter);
    }
    else
      repair_writer.delete_row(overlap->table_id, overlap->end_row, false);
  }
  metadata_overlaps.clear();

  cout << "METADATA shrunk ranges:  " << shrunk_ranges << endl;

  // Print out the dangling RSML entries
  if (!dangling_rsml.empty()) {
    cout << "RSML dangling entries:\n";
    for (auto ri : dangling_rsml)
      cout << "  " << ri->to_str() << "\n";
  }

  //
  // Fix gaps
  //
  // Checks to see if there is a matching dangling entity for the gap and if so,
  // sets the gap's entity and location from the dangling entity.  If there is
  // matching dangling entity then a random location is chosen from the set of
  // valid locations (round-robin).  then a METADATA entry is created for the
  // gap by writing StartRow and Location lines to "metadata.repair.tsv".  The
  // gap's RangeInfo object is then inserted into riset_all.
  //
  {
    RangeInfoSetRange dangling_by_range;

    for (auto dangling : dangling_rsml)
      dangling_by_range.insert(dangling);

    auto vli = valid_locations.begin();
    for (auto gap : metadata_gaps) {

      if (dangling_by_range.count(gap) > 0) {
	auto dangling_iter = dangling_by_range.find(gap);
	TableIdentifier table;
	RangeSpecManaged range_spec;
	RangeState range_state;
	range_spec.set_start_row( String(gap->start_row, gap->start_row_len) );
	range_spec.set_end_row( String(gap->end_row, gap->end_row_len) );
	{
	  RangeStateManaged range_state_managed;
	  (*dangling_iter)->entity->get_range_state(range_state_managed);
	  range_state.timestamp = range_state_managed.timestamp;
	  range_state.soft_limit = range_state_managed.soft_limit;
	}
	(*dangling_iter)->entity->get_table_identifier(table);
	gap->entity = make_shared<MetaLogEntityRange>(table, range_spec, range_state, false);
	gap->entity->set_load_acknowledged(true);
	gap->location = (*dangling_iter)->location;
      }
      else {
	gap->location = vli->c_str();
	if (++vli == valid_locations.end())
	  vli = valid_locations.begin();
      }
      gap->got_location = true;
      repair_writer.write_location(gap);
      repair_writer.write_start_row(gap);
      riset_all.insert(gap);
    }
  }

  std::vector<String> unassigned_location;
  std::vector<String> unassigned_start_row;
  std::vector<String> unassigned_entity;

  for (auto ri : riset_all) {
    if (!ri->got_location)
      unassigned_location.push_back(ri->to_str());
    if (!ri->got_start_row)
      unassigned_start_row.push_back(ri->to_str());
    if (ri->entity == 0)
      unassigned_entity.push_back(ri->to_str());
  }

  cout << "Unassigned locations: " << unassigned_location.size() << endl;
  if (gVerbose) {
    for (const auto &s : unassigned_location)
      cout << "  " << s << "\n";
  }  

  cout << "Unassigned start row: " << unassigned_start_row.size() << endl;
  if (gVerbose) {
    for (const auto &s : unassigned_start_row)
      cout << "  " << s << "\n";
  }  

  cout << "Unassigned entities: " << unassigned_entity.size() << endl;
  if (gVerbose) {
    for (const auto &s : unassigned_entity)
      cout << "  " << s << "\n";
  }

  //
  // Assign a LOCATION to each entry that doesn't have one
  //
  StringSet::iterator vl_iter = valid_locations.begin();
  RangeInfoSet riset_tmp;
  for (auto ri : riset_all) {
    if (!ri->got_location) {
      ri->location = flyweight.get(vl_iter->c_str());
      ri->got_location = true;
      if (++vl_iter == valid_locations.end())
	vl_iter = valid_locations.begin();
      repair_writer.write_location(ri);
    }
    riset_tmp.insert(ri);
  }
  riset_tmp.swap(riset_all);

  //
  // Create RSML entity for each riset_all entry that doesn't have one
  //
  // (use info from partially matching dangling entries)
  //
  {
    RangeInfoSetRange dangling_by_range;

    for (auto dangling : dangling_rsml)
      dangling_by_range.insert(dangling);

    for (auto ri : riset_all) {
      if (ri->entity == 0 && ri->got_location) {
	TableIdentifier table;
	RangeSpecManaged range_spec;
	RangeState range_state;
	range_spec.set_start_row( String(ri->start_row, ri->start_row_len) );
	range_spec.set_end_row( String(ri->end_row, ri->end_row_len) );

	if (dangling_by_range.count(ri) > 0) {
	  auto dangling_iter = dangling_by_range.find(ri);
	  {
	    RangeStateManaged range_state_managed;
	    (*dangling_iter)->entity->get_range_state(range_state_managed);
	    range_state.timestamp = range_state_managed.timestamp;
	    range_state.soft_limit = range_state_managed.soft_limit;
	  }
	  (*dangling_iter)->entity->get_table_identifier(table);
	  ri->entity = make_shared<MetaLogEntityRange>(table, range_spec, range_state, false);
	  ri->entity->set_load_acknowledged(true);
	  printf("Creating new MetaLog entity from pre-split entity for %s\n", ri->to_str().c_str());
	}
	else {
	  TableIdentifier dummy_tid;
	  const TableIdentifier *tid = tidmap.get(ri->location, ri->table_id);
	  range_state.timestamp = timestamp;
	  range_state.soft_limit = split_size;
	  if (tid == 0) {
	    if ((tid = tidmap.get_latest(ri->table_id)) == 0) {
	      HT_INFOF("TableIdentifier for %s not found, auto-generating...", ri->table_id);
	      dummy_tid.id = ri->table_id;
	      tid = &dummy_tid;
	    }
	  }
	  ri->entity = make_shared<MetaLogEntityRange>(*tid, range_spec, range_state, false);
	  ri->entity->set_load_acknowledged(true);
	  printf("Creating new MetaLog entity for %s\n", ri->to_str().c_str());
	}
	ri->got_start_row = true;
      }
    }
  }

  int unassigned_entity_count = 0;
  int gap_count = 0;
  int load_acknowledged_fixups = 0;

  last = 0;
  for (auto ri : riset_all) {
    if (last) {
      if (!strcmp(ri->table_id, last->table_id) &&
          (ri->start_row_len != last->end_row_len ||
           memcmp(last->end_row, ri->start_row, last->end_row_len))) {
        printf("error: plan contains non-contiguous ranges %s followed by %s\n",
               last->to_str().c_str(), ri->to_str().c_str());
        gap_count++;
      }
    }
    if (!ri->entity)
      unassigned_entity_count++;
    else if (!ri->entity->get_load_acknowledged()) {
      ri->entity->set_load_acknowledged(true);
      load_acknowledged_fixups++;
    }

    last = ri;
  }

  cout << "Load acknowledge fix-ups: " << load_acknowledged_fixups << endl;

  if (repair_writer.lines_written() > 0) {
    cout << "\n" << repair_writer.lines_written()
	 << " lines written to 'metadata.repair.tsv'.  "
            "Load with the following command:\n\n";
    cout << " echo \"use sys; load data infile 'metadata.repair.tsv' into table"
            " METADATA;\" | /opt/hypertable/current/bin/ht shell --batch\n";
  }

  cout << endl;

  if (unassigned_entity_count > 0 || gap_count > 0) {
    cout << "Exiting because unassigned entity count and/or gap count is non-zero ..." << endl;
    exit(1);
  }

  if (mode == "repair") {
    LocationEntitiesMapT::iterator entities_map_iter;
    MetaLog::WriterPtr rsml_writer;
    MetaLog::DefinitionPtr rsml_definition;

    for (auto ri : riset_all) {
      HT_ASSERT(entities_map.find(ri->location) != entities_map.end());
      entities_map[ri->location]->push_back(ri->entity);
    }

    for (entities_map_iter = entities_map.begin(); 
         entities_map_iter != entities_map.end(); ++entities_map_iter) {

      rsml_definition = make_shared<MetaLog::DefinitionRangeServer>(entities_map_iter->first.c_str());

      printf("Repairing RSML %s, writing %d entities\n", entities_map_iter->first.c_str(),
             (int)entities_map_iter->second->size());

      rsml_writer = make_shared<MetaLog::Writer>(fs, rsml_definition,
                                        toplevel_dir + "/servers/" + entities_map_iter->first + "/log/" + rsml_definition->name(),
                                        *(entities_map_iter->second));
      rsml_writer->close();
      rsml_writer = 0;
    }
  }
}

void load_valid_tids(const String &toplevel_dir, const String &subdir) {
  std::vector<DirEntry> listing;
  String dir = subdir.empty() ? toplevel_dir : toplevel_dir + "/" + subdir;
  uint64_t handle = 0;

  HT_ON_SCOPE_EXIT(&Hyperspace::close_handle_ptr, hyperspace, &handle);

  handle = hyperspace->open(dir, Hyperspace::OPEN_FLAG_READ);

  hyperspace->readdir(handle, listing);

  for (const auto &entry : listing) {
    String path = subdir.empty() ? entry.name : subdir + "/" + entry.name;
    if (entry.is_dir)
      load_valid_tids(toplevel_dir, path);
    else
      valid_tids.insert(path);
  }
}

int main(int argc, char **argv) {
  init_with_policies<Policies>(argc, argv);

  try {
    toplevel_dir = properties->get_str("Hypertable.Directory");
    boost::trim_if(toplevel_dir, boost::is_any_of("/"));
    toplevel_dir = String("/") + toplevel_dir;
    conn_manager = make_shared<ConnectionManager>();
    hyperspace = make_shared<Hyperspace::Session>(conn_manager->get_comm(),
					 properties);

    if (!hyperspace->wait_for_connection(10000)) {
      HT_ERROR("Unable to connect to Hyperspace, exiting...");
      exit(1);
    }

    dfs = new FsBroker::Lib::Client(conn_manager, properties);

    if (!dfs->wait_for_connection(10000)) {
      HT_ERROR("Unable to connect to DFS Broker, exiting...");
      exit(1);
    }

    std::vector<Filesystem::Dirent> listing;  
    String toplevel = properties->get_str("Hypertable.Directory");
    boost::trim_if(toplevel, boost::is_any_of("/"));
    toplevel = String("/") + toplevel;

    dfs->readdir(toplevel + "/servers", listing);

    for (size_t i=0; i<listing.size(); i++) {

      if (listing[i].name == "master")
        continue;

      // Check for removal
      String fname = toplevel + "/servers/" + listing[i].name;
      uint64_t handle = hyperspace->open(fname, Hyperspace::OPEN_FLAG_READ);

      if (hyperspace->attr_exists(handle, "removed")) {
        std::cout << "Range server " << listing[i].name 
                  << " has been marked \"removed\" in hyperspace, skipping..."
                  << std::endl;
        hyperspace->close(handle);
        continue;
      }
      hyperspace->close(handle);
      valid_locations.insert(listing[i].name);
    }

    if (valid_locations.empty()) {
      cout << "Unable to proceed because no valid locations found." << endl;
      _exit(1);
    }

    load_valid_tids("/hypertable/namemap/ids", "");

    if (has("verify-dfs"))
      handle_verifydfs();
    else
      handle_default();
  }
  catch (Exception &e) {
    HT_ERROR_OUT << e << HT_END;
    return 1;
  }
  return 0;
}
