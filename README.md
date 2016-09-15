ipc
====

Tibia IP changer for Linux available as a command line tool and a Qt-based GUI.

Building (command line)
========

* ``mkdir build``
* ``g++ -std=c++14 -obuild/ipc src/common/common.cpp src/console/ipc.cpp -lX11``

Building (GUI)
========

* ``qmake``
* ``make``

License
=======

This project is released under the MIT license. You can find the details in the [license file](LICENSE.md).
