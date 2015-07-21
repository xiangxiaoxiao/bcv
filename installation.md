---
layout: default
title: Installation
---
## Installation

The project is built with a single makefile, and there are somewhat few dependencies.


### Dependencies

To allow reading/writing images, make sure you have `libjpeg` and `libpng` installed.
On linuxmint/ubuntu systems, this is easy to do:

    $ sudo apt-get install libpng-dev libjpeg-dev

*Optionally* you can install [FFTW3](http://www.fftw.org/), which is necessary for some components.
This can be done with:

    $ sudo apt-get install libfftw3-dev

*Optionally*, to be able to read and write videos, you need avconv/ffmpeg libraries.
You may be able to get it from repositories:

    $ sudo apt-get install libav-tools

However, my preferred method is to [build it from source](https://www.ffmpeg.org/). 
The current version of BCV is made to work with ffmpeg 2.7.1.

Finally, to be able to run examples, you need [gflags](http://gflags.github.io/gflags/).
The library itself can be build without this.
    
### Build commands

You can build the library and the examples with:

    $ make all

(To build the library alone -- `make bcv`).
You may wish to symlink the library to `/usr/local/lib`, in which case you should
probably run

	$ sudo make install  
    $ sudo ld config  

Alternatively you can modify your `LD_LIBRARY_PATH` to include the lib directory.
To do so, open your `.bashrc` file with your favorite text editor:

	$ vim ~/.bashrc
Add a line:

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/bcv/lib/
And reload it:

	$ source ~/.bashrc 

At this point you can navigate to `bin/` and try the examples.
    
### Makefile configuration

The `Makefile` may need to be modified to point to appropriate include directories.
In addition it contains several flags:

* `HAVE_SSE` - enables SSE optimization.
* `HAVE_FFTW` - is necessary for convolutions in TV deblurring code.
* `HAVE_FFMPEG` - is necessary for reading/writing videos

All these are optional in principle -- if you do not have the ability to 
run these things on your platform, you may turn them off and still compile the
library (albeit with somewhat limited functionality).

### Documentation

In addition to the few tutorials listed on this website, the code contains
limited doxygen annotations, which can be build with:

    $ make documentation

After which you can check out `doc/html/index.html`
