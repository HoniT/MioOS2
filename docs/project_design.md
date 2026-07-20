# MioOS project design
This document describes the project's design, how I've decided to structure and develop it, alongside the design patterns and abstraction strategies I'm using.

## CPU architecture specific code

**MioOS_2 is a x86_64 specific OS kernel and doesn't support other CPU architectures; because of this the codebase is way simpler**. Arch-independent subsystems can directly call the method they need from arch-dependent (x86_64 specific components like the GDT, IDT ...) code without needing a Hardware Abstraction Layer (HAL). I've placed x86_64 specific components in the [`arch`](/src/kernel/arch/) folder of the kernel to keep the project structure tidy.


## OOP

Most subsystems like memory managers, the GDT and drivers use classes. Most of these are just singletons or have only static methods/attributes. For the classes that actually need to have objects (e.g. We may need multiple framebuffer driver objects for multiple displays/monitors), we also need to store those objects somewhere in our code to be easily accessed. For this I've added [**subsystem managers**](/src/kernel/include/subsystems/) to this project, which serves about the same purpose as a DI. These subsystems only job is to store objects that we'll need for a certain task. A good example of this is the [Output Subsystem](/src/kernel/include/subsystems/output_subsystem.hpp) which holds driver objects dedicated for output (the COM serial driver object and an array of framebuffers). 