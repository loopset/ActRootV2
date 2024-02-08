# ActRoot
ActRoot provides the basic tools for analyzing and simulating ACTAR TPC experiments.

## Doxygen documentation
Uncompleted `Doxygen` documentation is available at [here](https://loopset.github.io/ActRootV2/). 
Major advances are still required to have all the code documented.

## Installation

Follow these steps to have ActRoot working on your computer
1. Clone this repo into your local computer

```bash
git clone https://github.com/loopset/ActRootV2.git
```
2. Create a `build` directory and move into it
```bash
mkdir build && cd build
```
3. Run CMake and `make`
```bash
cmake .. && make -jN install
```
This command will install all the libraries and headers in the created `install` folder inside the ActRoot parent directory.

Please, replace `N` with your number of cores (which you might find by executing `nproc` in a terminal)

4. Source the `thisActRoot.sh` file located in the ActRoot main folder.
   For this, just open your local `.bashrc` file and append at the end the line
```bash
source /home/youruser/ActRootV2/thisActRoot.sh
```
Now your system knows where the installation is located and you can use all the libraries inside a ROOT macro! Also, do not forget to include the headers in your 
macros when they are needed. They are all under `install/include`, so, for instance, you would add `#include "ActParticle.h"` for working with an `ActPhysics::Particle` object.

## Acknowledgements
This software is built using the [ROOT](https://root.cern.ch/) C++ libraries from CERN. For the multithreading service, `BS::thread_pool` from [bshoshany's Github](https://github.com/bshoshany/thread-pool) was employed. Thanks to both teams!
