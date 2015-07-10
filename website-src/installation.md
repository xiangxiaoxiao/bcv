### Installation

The project is built with a single makefile, and there are relatively few dependencies.
To allow reading/writing images, make sure you have `libjpeg` and `libpng` installed.
On linuxmint/ubuntu systems, this is easy to do:

    $ sudo apt-get install libpng-dev libjpeg-dev

Optionally you can install [FFTW3](http://www.fftw.org/), which is necessary for some components.
This can be done with:

    $ sudo apt-get install libfftw3-dev

To be able to run examples, you need [gflags](http://gflags.github.io/gflags/).
    
<br>

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
    
<br>

The `Makefile` may need to be modified to point to appropriate include directories.
In addition it contains flags `HAVE_SSE` and `HAVE_FFTW`. The former is used
to enable SSE optimizations. The latter makes it possible to compile the library
without features that require the FFT.
