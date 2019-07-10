// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _NZOS_DISPLAY_UTIL_H_
#define _NZOS_DISPLAY_UTIL_H_

#include <vector>

#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"

namespace base {
class FilePath;
}

namespace display {
class DisplayMode;
class DisplaySnapshot;
}

namespace ui {

// Conforms to the std::UnaryPredicate interface such that it can be used to
// find a display with |display_id| in std:: containers (ie: std::vector).
class FindDisplayById {
 public:
  explicit FindDisplayById(int64_t display_id);

  bool operator()(const DisplaySnapshot_Params& display) const;

 private:
  int64_t display_id_;
};

// DisplayMode_Params GetDisplayModeParams(const display::DisplayMode& mode);
DisplaySnapshot_Params GetDisplaySnapshotParams(const display::DisplaySnapshot& display);

// Create a display using the Ozone command line parameters.
// Return false if the command line flags are not specified.
DisplayMode_Params GetDisplayModeParams(const display::DisplayMode& mode);
bool CreateSnapshotFromCommandLine(DisplaySnapshot_Params* snapshot_out);

}  // namespace ui

#endif  // _NZOS_DISPLAY_UTIL_H_
