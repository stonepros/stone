=====================
 Installing Oprofile
=====================

The easiest way to profile Stone's CPU consumption is to use the `oprofile`_
system-wide profiler.

.. _oprofile: http://oprofile.sourceforge.net/about/

Installation
============

If you are using a Debian/Ubuntu distribution, you can install ``oprofile`` by
executing the following::

	sudo apt-get install oprofile oprofile-gui
	

Compiling Stone for Profiling
============================

To compile Stone for profiling, first clean everything. :: 

	make distclean
	
Then, export the following settings so that you can see callgraph output. :: 

	export CFLAGS="-fno-omit-frame-pointer -O2 -g"

Finally, compile Stone. :: 

	./autogen.sh
	./configure
	make

You can use ``make -j`` to execute multiple jobs depending upon your system. For
example::

	make -j4


Stone Configuration 
==================

Ensure that you disable ``lockdep``. Consider setting logging to 
levels appropriate for a production cluster. See `Stone Logging and Debugging`_ 
for details.

.. _Stone Logging and Debugging: ../../rados/troubleshooting/log-and-debug

See the `CPU Profiling`_ section of the RADOS Troubleshooting documentation for details on using Oprofile.


.. _CPU Profiling: ../../rados/troubleshooting/cpu-profiling