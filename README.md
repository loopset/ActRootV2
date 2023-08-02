# ActRoot
ActRoot provides the basic tools for analyzing and simulating ACTAR TPC experiments.

## Installation

Follow these steps to have ActRoot working on your computer
1. Clone this repo to your local computer

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
This command will install all the libraries and headers in the created `install` dir inside the ActRoot folder.

Please, replace `N` with your number of cores (which you might find by executing `nproc` in a terminal)

4. Source the `thisActRoot.sh` file located in the ActRoot main folder.
   For this, just open your local `.bashrc` file and append at the end the line
```bash
source /home/youruser/ActRootV2/thisActRoot.sh
```
Now your system knows where the installation is located and you can use all the libraries inside a ROOT macro! Also, do not forget to include the headers in your macros when they are needed. They are all under `instal/include`, so for instance you would add `#include "Particle.h"` for working with a `ActPhysics::Particle` object.