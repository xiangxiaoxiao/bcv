### What this is

bcv is a small library of algorithms that i (personally) find interesting. 

### Examples

* [test_slic](test_slic.html) - SLIC superpixels
* [test_tvsegment](test_tvsegment.html) - TV-regularized segmentation

### Installation

Make sure you have libjpeg and libpng installed. Install [gflags](gflags) if you
plan to use examples. Then run makefile

    $ make all
You may wish to symlink the library to `/usr/local/lib`, in which case you should
probably run

	$ sudo make install  
    $ sudo ld config  
Alternatively you can modify your `LD_LIBRARY_PATH` to include the lib directory.
Open your `.bashrc` file with your favorite text editor:

	$ vim ~/.bashrc
Add a line:

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/bcv/lib/
And reload it:

	$ source ~/.bashrc 

### Contact

[email me](mailto:karasev00@gmail.com)