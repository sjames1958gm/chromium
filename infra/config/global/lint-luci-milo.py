#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Checks that the main console and subconsole configs are consistent."""

import collections
import difflib
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.join(THIS_DIR, '..', '..', '..')
sys.path.insert(1, os.path.join(
    SRC_ROOT, "third_party", "protobuf", "python"))

import google.protobuf.text_format
import project_pb2


def compare_builders(name, main_builders, sub_builders):
  # Checks that the builders on a subwaterfall on the main waterfall
  # are consistent with the builders on that subwaterfall's main page.
  # For example, checks that the builders on the "chromium.win" section
  # are the same as on the dedicated standalone chromium.win waterfall.
  main_names = [', '.join(builder.name) for builder in main_builders]
  sub_names = [', '.join(builder.name) for builder in sub_builders]

  if main_names != sub_names:
    print ('bot name lists different between main waterfall ' +
           'and stand-alone %s waterfall:' % name)
    print '\n'.join(difflib.unified_diff(main_names, sub_names,
                                         fromfile='main', tofile=name,
                                         lineterm=''))
    print
    return False
  return True


def main():
  project = project_pb2.Project()
  with open(os.path.join(THIS_DIR, 'luci-milo.cfg'), 'rb') as f:
    google.protobuf.text_format.Parse(f.read(), project)

  # Maps subwaterfall name to list of builders on that subwaterfall
  # on the main waterfall.
  subwaterfalls = collections.defaultdict(list)
  for console in project.consoles:
    if console.id == 'main':
      # Chromium main waterfall console.
      for builder in console.builders:
        subwaterfall = builder.category.split('|', 1)[0]
        subwaterfalls[subwaterfall].append(builder)

  all_good = True
  for console in project.consoles:
    if console.id in subwaterfalls:
      if not compare_builders(console.id, subwaterfalls[console.id],
                              console.builders):
        all_good = False
  return 0 if all_good else 1


if __name__ == '__main__':
  sys.exit(main())
