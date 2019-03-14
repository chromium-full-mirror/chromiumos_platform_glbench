# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#!/bin/bash

cd ref_images/glbench_reference_images
ls *.png | sort > ../../glbench_reference_images.txt
ls *.png | sort > index.html

TYPE=(glbench_knownbad_images glbench_fixedbad_images)

for images in ${TYPE[*]}
do
  cd ../../ref_images/${images}
  ls */*.png 2>/dev/null | sort > ../../${images}.txt
  ls */*.png 2>/dev/null | sort > index.html
done
