#!/bin/bash
# Copyright 2019 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TYPE=(
  "glbench_reference_images"
  "glbench_knownbad_images"
  "glbench_fixedbad_images"
)

for images in "${TYPE[@]}"; do
  dir="ref_images/${images}"
  [[ -d "${dir}" ]] || echo "NOTICE: ${dir} does not exist."

  find "${dir}" -name "*.png" -printf "%P\n" 2>/dev/null | sort > "${images}.txt"
done
