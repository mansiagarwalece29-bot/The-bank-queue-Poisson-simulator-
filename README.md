# Bank Queue (Poisson) Simulator

**Project**: Clash of Coders — Capstone  
**Language**: C  
**Author**: (your name)

## Overview
This program simulates a bank queue over an 8-hour day (480 minutes). Customer arrivals follow a Poisson distribution (user-specified λ = average arrivals per minute). The queue is implemented as a linked list (dynamic memory). One or more tellers serve customers; each service takes 2–3 minutes (random). The simulator records each customer's wait time and computes summary statistics: mean, median, mode, standard deviation, and longest wait.

## Files
- `bank_queue_simulator.c` — main C source file
- `.gitignore` — recommended to ignore compiled binaries

## How to compile
Make sure you have `gcc` installed.

```bash
gcc bank_queue_simulator.c -o bank_queue_simulator -lm
