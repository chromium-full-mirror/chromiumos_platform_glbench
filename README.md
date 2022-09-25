GLBench runs OpenGL or OpenGL ES microbenchmarks and writes
performance numbers to stdout and resulting images to a directory for
verification.

For the test to pass the performance numbers have to be better than a predefined
threshold, while the resulting images have to be found in a repository of
reference images. As the image name encodes the raw pixel MD5 this can be
done as a simple file existence check. If we ever get too much pixel
variation using a tool like perceptualdiff to waive small differences
should be acceptable.

# Build for Linux

It might be easier to develop new tests under Linux:

    sudo apt-get install libgflags-dev
    sudo apt-get install libwaffle-dev
    sudo apt-get install waffle-utils (optional)
    make

# Executable Options

    ./glbench -notemp [-save [-outdir=<directory>]]


# Example

    ./glbench -save -outdir=img
    # board_id: NVIDIA Corporation - Quadro FX 380/PCI/SSE2
    swap_swap                    =   214.77 us           [swap_swap.pixmd5-20dbc406b95e214a799a6a7f9c700d2f.png]
    clear_color                  =  4448.28 mpixels_sec  [clear_color.pixmd5-e3609de1022a164fe240a562c69367de.png]
    clear_depth                  = 10199.76 mpixels_sec  [clear_depth.pixmd5-e3609de1022a164fe240a562c69367de.png]
    clear_colordepth             =  3250.57 mpixels_sec  [clear_colordepth.pixmd5-e3609de1022a164fe240a562c69367de.png]
    clear_depthstencil           = 26447.22 mpixels_sec  [clear_depthstencil.pixmd5-e3609de1022a164fe240a562c69367de.png]
    [...]

    ls img
    clear_color.pixmd5-e3609de122a164fe240a562c69367de.png
    clear_colordepth.pixmd5-e3609de122a164fe240a562c69367de.png
    clear_colordepthstencil.pixmd5-e3609de122a164fe240a562c69367de.png
    compositing.pixmd5-7d02a16a7ac15cd6cbbc5c786f1.png
    [...]


# Running from the autotest harness

Running the autotest *test_that $DUT graphics_GLBench* will \
1. run glbench -save \
2. first try to identify known buggy images by searching in *deps/glbench/glbench_knownbad_images.txt* \
3. then identify good images by searching in *deps/glbench/glbench_reference_images.txt* \
4. TODO(ihf): use perceptualdiff to do a fuzzy compare \
5. raise an error if the image is completely unknown \
6. report performance numbers back to the harness \


# Handling of reference images

Good reference images themselves are located at *./ref_images/glbench_reference_images/* \
Images that have outstanding defects and an open bug filed are at *./ref_images/glbench_knownbad_images/chromium-bug-NNNNN/* \
When that bug is closed the directory should be moved to *./ref_images/glbench_fixedbad_images/chromium-bug-NNNNN/* \

To push out new reference images place them in the appropriate directories
(create a new bug if needed) and run *./update_glbench_image_filelists.sh* to
update the image filelists.

# compositortest

CompositorTest is another binary generated in the glbench directory. It runs a
small OpenGL or OpenGL ES shader program and optionally sleeps for some time
before swapping buffers. The time between swaps is logged to a file and
processed for reporting in a tast test. We can see from this simiulated work
load how much time can be spent processing a frame before it gets dropped.

# example

./compositortest --fullscreen true --gpu_workload_ms 15
Platform OpenGL version: OpenGL ES 3.2 Mesa 22.1.4 (git-d3e5726da1)
Update num quads: 144863
calibrating: gpu_elapsed_time: 14.9717 ms
Finished calibrating workload.
[..]
frame 998: cpu_elapsed_time: 16.1118 ms
frame 998: gpu_elapsed_time: 14.9748 ms
frame 999: cpu_elapsed_time: 16.1824 ms
frame 999: gpu_elapsed_time: 14.9726 ms
Avg: 62 fps for 16 secs. 1000 frames rendered.