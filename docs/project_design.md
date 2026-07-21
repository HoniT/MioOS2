# MioOS Project Architecture and Design

This document outlines the architectural design, structural methodologies, and abstraction strategies employed in the development of the MioOS project.

## CPU Architecture Specificity

**MioOS_2 is a dedicated x86_64 operating system kernel.** By intentionally omitting cross-architecture support, the overall codebase maintains a streamlined and simplified structure.

Because of this targeted focus, architecture-independent subsystems can interface directly with architecture-dependent components (such as the GDT, IDT, and other x86_64-specific elements) without the overhead of a Hardware Abstraction Layer (HAL). To ensure a clean and organized project structure, all x86_64-specific components are localized within the [`arch`](/src/kernel/arch/) directory.

## Object-Oriented Programming (OOP) Paradigms

Core subsystems- including memory managers, the GDT, and device drivers- are implemented using object-oriented classes. The majority of these utilize the Singleton design pattern or rely strictly on static methods and attributes.

For components that require multiple instantiations (e.g., maintaining multiple framebuffer driver objects to support multi-monitor setups), effective state management is essential to ensure these objects are easily accessible throughout the codebase.

To address this, [**subsystem registries**](/src/kernel/include/registry/) have been introduced, functioning similarly to Dependency Injection (DI) containers. The primary responsibility of these managers is to act as centralized repositories for task-specific objects. A prime example of this architecture is the [Output Registry](/src/kernel/include/registry/output_registry.hpp), which seamlessly encapsulates driver instances dedicated to system output, such as the COM serial driver object and an array of framebuffers.