### Installation

The project is built with a single makefile, and there are rather few dependencies.
To allow reading/writing images, make sure you have `libjpeg` and `libpng` installed.
Also install [gflags](gflags), as all examples heavily rely on it.

You can build the library and the examples simply with:

    $ make all
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