ldd3-examples-3.x
=================

port of the ldd3 source code examples to linux 3.x

backgroud
=================
ldd3 is Linux Device Drivers, Third Edition.
This is a great book about how to write linux device drivers.
You can get this book and its source code examples free
from http://lwn.net/Kernel/LDD3/.

But, the souce code examples in the book are based on linux 2.6.10,
which was released in 2005. This means that all the original examples 
won't compile in the current linux 3.x branch. I've ported all examples 
to the longterm stable branch after linux 3.0, including:

|branch          |original release date|
|----------------|:--------------------:|
|linux 3.0       |July 2011 |
|linux 3.2       |January 2012 |
|linux 3.4       |May 2012 |

The key difference between this project and other porting attemps is that
all these examples not only compile on the modern kernel version,
but they also **RUN** on the modern kernel!

how to port
=================

Make it compile.
-----------------
a. clone linux-stable repo from
    http://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git

b. Use git log to find out who changed what when. Then determine what is causing 
   it to not compile. then change ldd3 examples according to those commits.

   If  compiling errors are 'XXX symbol not found', find root causing commits by:
```
        $git log -p <XXX symbol's file path in original linux version> |\
          grep <XXX symbol>
```

c. Use git bisect when git log fails to work.

Make it run.
-----------------
First you should understand the original examples,

Second you should understand the related commits in the modern linux version.

If you still can't fix the bug, debug it using below methods:

a. compare similar driver code in linux-stable.
   For example: debuging snull by refering to loopback.c,
       debug snull by refering to loop.c...

b. printk

c. gdb/qemu.
   compile busybox+linux running in qemu, load buggy module, use gdb to debug it.
   Embedded Linux From Scratch is useful when doing such tasks, refer to
   http://free-electrons.com/docs/elfs/.

