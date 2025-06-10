# Concurrent Producer-Consumer System in C

This project was developed as part of coursework for my **Operating Systems** course. It demonstrates key concepts such as process management, interprocess communication, and synchronization.

## Overview

The program simulates a factory where two separate producers generate products of different types and send them through a pipe to a parent consumer process. The consumer process distributes the products into two bounded circular buffers based on product type. Multiple consumer threads consume products from these buffers concurrently.

Key features include:

- **Process creation with `fork()`** to run separate producers.
- **Interprocess communication using pipes** to transfer data between producers and the consumer.
- **Multi-threading with POSIX threads (`pthreads`)** for concurrent consumption.
- **Synchronization mechanisms** including mutexes and condition variables to safely manage shared buffers.
- **Bounded circular buffers** for each product type to implement the producer-consumer pattern.
- **Thread-safe logging** of consumption events to a file.
