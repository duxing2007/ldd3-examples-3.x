ldd3-examples-3.x
=================

port ldd3 source code examples to linux 3.x

backgroud
=================
ldd3 is Linux Device Drivers, Third Edition.
A great book about how to write linux device drivers.
You can get this book and its source code examples free
from http://lwn.net/Kernel/LDD3/.

But souce code examples in this code are based on linux 2.6.10,
which was released in 2005. Basically all original examples won't
compiled in currently linux 3.x. I've port examples to all longterm
stable branch after linux 3.0, including:
linux 3.0       first released in July 2011
linux 3.2       first released in January 2012
linux 3.4       first released in May 2012

The key differnt between this project and other porting is:
All examples not only compiled on new kernel version,
But also *RUN* on new kernel version!

how to port
=================

Make it compiled
-----------------
a. clone linux-stable repo from
    http://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
b. Using git log to find out who in when changed what causing it not compiled,
   then changed ldd3 examples according to those commits.

   Supposed compiling errors are XXX symbol not found, find root caused commits by:
        $git log -p <XXX symbol's file path in original linux version> |\
          grep <XXX symbol>

c. Using git bisect when git log not work

Make it run
-----------------
Firstly we should understanding original examples,
Secondly we should understanding related commits in new linux version.
If we still can't fix the bug, debug it using below methods:
a. compared similar driver code in linux-stable.
   For example: debuging snull by refer to loopback.c,
       debug snull by refer to loop.c...
b. printk
c. gdb/qemu.
   compile busybox+linux running in qemu, load buggy module, using gdb to debug it.
   Embedded Linux From Scratch is useful when doing such tasks, refer to
   http://free-electrons.com/docs/elfs/.

