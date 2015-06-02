# TeaTime

Please refer to the blog post <https://stealthy.io/blog/201506.html> about the
description of this proof-of-concept.

## PRE-REQUISITES

On Debian 7.0 Linux or its derivatives you need the following installed:

    $ sudo apt-get install build-essential gcc libglew-dev freeglut3-dev
        libglu1-mesa-dev libgl1-mesa-dev
   
You will also need OpenGL 3.0 or better installed using the AMD/ATI `fglrx`
driver or NVIDIA's proprietary driver.

    $ sudo apt-get fglrx-driver libgl1-fglrx-glx
    ## ... OR ...
    $ sudo apt-get nvidia-driver libgl1-nvidia-glx


## BUILD and TEST

To build the software, you just need to run the `make` command and it will
generate an executable called `teatime`.

    $ make
    ## ... now run the executable ...
    $ ./teatime


## COPYRIGHT

&copy; 2015. Stealthy Labs LLC. All Rights Reserved.

## LICENSE

MIT License.
