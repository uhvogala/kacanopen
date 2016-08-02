# Build Instructions

## Requirements

- a recent C++ compiler with C++14 support:
	- [GCC](https://gcc.gnu.org/) >= 4.9
	- [Clang](http://clang.llvm.org/) >= 3.6
	- others may work though not tested

- [CMake](https://cmake.org/) >= 3.2

- [Boost](http://www.boost.org/) >= 1.46.1
	
	If your system is missing a suitable boost installation, you can download and extract the latest Boost release from [http://www.boost.org/](http://www.boost.org/) and specify its path using the following catkin/CMake argument:

	`-DBOOST_ROOT=/path/to/boost`

- [ROS](http://www.ros.org/install/) >= Jade Turtle

	This is only needed for ROS Bridge. You can compile without ROS using the following CMake argument:

	`-DNO_ROS=On`

- [Doxygen](www.doxygen.org/) >= 1.8.7 and [Graphviz](www.graphviz.org/) >= 2.36.0
	
	This is only needed for compiling the documentation.

If your OS is Ubuntu 14.04 / Linux Mint 17.3 or newer, you can install all requirements with the following commands:

~~~bash
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -usc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver hkp://pool.sks-keyservers.net:80 --recv-key 0xB01FA116
sudo apt-add-repository ppa:george-edison55/cmake-3.x
sudo apt-get update
sudo apt-get install ros-jade-ros-base cmake clang-3.6 libboost-all-dev doxygen graphviz
sudo apt-get install cmake clang-3.6 libboost-system-dev libboost-filesystem-dev ros-jade-catkin ros-jade-roscpp ros-jade-std-msgs ros-jade-sensor-msgs ros-jade-message-runtime doxygen graphviz
~~~


## CMake/Catkin Arguments

The following CMake/Catkin arguments are available:

- `-DDRIVER=<name>`
	
	Specify the driver to compile and use. At the moment, the following drivers are available: lincan, peak_linux, serial, socket, virtual, dummy.

- `-DBUILD_DRIVERS=<;-separated-list of names>`

	Semicolon-separated list of additional CAN drivers to build. You can use the same values as for DRIVER.

- `-DBUILD_ALL_DRIVERS=On`

	Build all available drivers.

- `-DPCAN_PREFIX=<path>`

	Path to the [PCAN drivers](http://www.peak-system.com/fileadmin/media/linux/index.htm#download) needed by the peak_linux driver. See [Drivers section](#drivers).

- `-DEXHAUSTIVE_DEBUGGING=On`

	Enable exhaustive debugging.

- `-DNO_ROS=On`

	Exclude anything depending on ROS.

- `-DINSTALL_EXAMPLES=On`
	
	Install example programs when running make install.

- `-DSDO_RESPONSE_TIMEOUT_MS=<timeout>`

	Timeout in milliseconds when waiting for an SDO response.

- `-DCONSECUTIVE_SEND_PAUSE_MS=<timeout>`

	Pause between two consecutively sent CAN frames in milliseconds.


## Initialize ROS and create Catkin workspace

If your ROS installation is fresh you shoud first initialize it like described [here](http://wiki.ros.org/jade/Installation/Ubuntu).

~~~bash
sudo rosdep init
rosdep update
echo "source /opt/ros/jade/setup.bash" >> ~/.bashrc
source ~/.bashrc
~~~

You will need a Catkin workspace:

~~~bash
mkdir ~/arbitrary_path/catkin_ws
cd ~/arbitrary_path/catkin_ws
mkdir src
cd src
catkin_init_workspace
~~~

## Build process without ROS Bridge

KaCanOpen without the ROS part can be built easily using CMake:

~~~bash
git clone https://github.com/KITmedical/kacanopen.git
cd kacanopen
mkdir build
cd build
cmake -DDRIVER=<driver> -DNO_ROS=On ..
make
~~~

Replace `<driver>` by a CAN driver name (see section Drivers).

## Build process including ROS Bridge

Your KaCanOpen repository must reside or be symlinked inside src:

~~~bash
cd ~/arbitrary_path/catkin_ws/src
git clone https://github.com/KITmedical/kacanopen.git
~~~

Now you can build it:

~~~bash
catkin_make -DDRIVER=<driver>
~~~

Replace `<driver>` by a CAN driver name (see section [Drivers](#drivers)).

## Examples

There are several examples on how to use KaCanOpen. Source files are in the `examples/` dictionary.

When building with Catkin, you can excute example programs like that:

~~~bash
cd your_catkin_workspace
source devel/setup.bash
rosrun kacanopen kacanopen_example_motor_and_io_bridge # roscore needs to be running
~~~

Otherwise just run them from `build/examples/`:

~~~bash
./build/examples/kacanopen_example_listdevices
~~~

## Drivers

KaCanOpen provides several CAN drivers (currently only for Linux).

- socket (default)

	For use with [SocketCAN](https://en.wikipedia.org/wiki/SocketCAN), formerly known as LLCF. It's a CAN networking stack being part of the Linux kernel. It's probably the most popular CAN driver infrastructure and there are many devices supporting SocketCAN.

	One of them is [USBtin](http://www.fischl.de/usbtin/), which we use for development purposes. If you want to use USBtin with KaCanOpen just run dev/init_usbtin.sh.

	Use "slcan0" and "500K" as arguments for Core.start(busname,baudrate) / Master.start(busname,baudrate).

- serial

	Driver for use with serial character devices.

- virtual

	This driver can be used without any CAN hardware. It spans a virtual CAN network using POSIX pipes.

- peak_linux

	For use with CAN hardware by [PEAK-System](http://www.peak-system.com/fileadmin/media/linux/index.htm). You will need [PCAN drivers](http://www.peak-system.com/fileadmin/media/linux/index.htm#download) installed for successful compilation:

		wget http://www.peak-system.com/fileadmin/media/linux/files/peak-linux-driver-7.15.2.tar.gz
		tar -xzf peak-linux-driver-7.15.2.tar.gz
		mv peak-linux-driver-7.15.2 ~/peak
		cd ~/peak
		make

	Then build KaCanOpen with the CMake/Catkin flag `-DPCAN_PREFIX="~/peak"`. You can replace `~/peak` as you wish.

- lincan

	For use with [LinCan](http://ortcan.sourceforge.net/lincan/) kernel drivers.

You can also use any driver from the [CanFestival](http://www.canfestival.org/), as they are binary-compatible. They also have Windows drivers.
