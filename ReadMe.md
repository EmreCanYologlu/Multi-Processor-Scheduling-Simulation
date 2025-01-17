# Multi-Processor Scheduling Simulation

## Overview

This project implements a multi-threaded simulation of multi-processor scheduling in C/Linux. The program simulates two scheduling approaches:

1. **Single-Queue Approach:** All processors share a common ready queue.
2. **Multi-Queue Approach:** Each processor has its own ready queue.

The simulation supports two scheduling algorithms:
- **FCFS (First-Come, First-Served):** Processes are executed in the order they arrive.
- **SJF (Shortest Job First - Non-preemptive):** Processes with shorter burst times are executed first.

---

## Features

- **Scheduling Approaches:**
  - Single-Queue.
  - Multi-Queue with two queue selection methods:
    - **Round-Robin Method (RM):** Bursts are distributed cyclically across queues.
    - **Load-Balancing Method (LM):** Bursts are added to the queue with the least load.

- **Simulation Algorithms:**
  - FCFS.
  - SJF.

- **Random Workload Generation:** Generate burst lengths and interarrival times if no input file is provided.

- **Output Modes:**
  - **Mode 1:** Minimal output.
  - **Mode 2:** Log picked bursts.
  - **Mode 3:** Detailed logging of all key events.

- **Metrics Collected:**
  - Turnaround Time.
  - Waiting Time.
  - Average Turnaround Time.

---

## Requirements

- **Operating System:** Linux (tested on Ubuntu 24.04 64-bit).
- **Compiler:** gcc.
- **Libraries:** POSIX pthreads for threading and synchronization.

---

## Usage

### Compilation

Run the following command to compile the project:
```bash
make
```
This will generate the executable `mschedule`.

### Running the Program

The general syntax for running the program is:
```bash
./mschedule -n N -a SAP QS -s ALG [-i INFILE | -r T T1 T2 L L1 L2 PC] [-m OUTMODE] [-o OUTFILE]
```

#### Parameters:
- `-n N`: Number of processors (1 to 64).
- `-a SAP QS`: Scheduling approach and queue selection method.
  - `SAP`: `S` for Single-Queue, `M` for Multi-Queue.
  - `QS`: `NA` (for Single-Queue), `RM` (Round-Robin), `LM` (Load-Balancing).
- `-s ALG`: Scheduling algorithm (`FCFS` or `SJF`).
- `-i INFILE`: Input file containing process lengths and interarrival times.
- `-r T T1 T2 L L1 L2 PC`: Generate workload randomly.
  - `T`: Mean interarrival time.
  - `T1`, `T2`: Minimum and maximum interarrival times.
  - `L`: Mean burst length.
  - `L1`, `L2`: Minimum and maximum burst lengths.
  - `PC`: Number of processes.
- `-m OUTMODE`: Output mode (1, 2, or 3; default is 1).
- `-o OUTFILE`: File to save the simulation output.

#### Examples:
1. Using an input file:
   ```bash
   ./mschedule -n 4 -a M LM -s SJF -i input.txt -m 2 -o output.txt
   ```
2. Generating random workload:
   ```bash
   ./mschedule -n 8 -a S NA -s FCFS -r 200 50 600 100 10 400 50 -m 3 -o result.txt
   ```

---

## Input and Output Formats

### Input File Format
An input file contains alternating lines of process lengths (`PL`) and interarrival times (`IAT`):
```
PL 100
IAT 50
PL 200
IAT 30
...
```

### Output Format
The program outputs the following metrics for each process, sorted by process ID:
```
pid   cpu  burstlen   arv   finish  waitingtime turnaround
1      1    1000       0    1000        0       1000
2      2     500     200    1500      800       1300
...
average turnaround time: 1150 ms
```

---

## Experimentation

### Performance Experiments
Conduct experiments with different:
- Number of processors (`-n`).
- Scheduling approaches (`-a`).
- Algorithms (`-s`).
- Workloads (random vs. input files).

### Metrics
Analyze:
- Average turnaround time.
- Effect of queue selection methods on load distribution.

### Report
Save experimental results and analysis in `report.pdf`.

---

## File Structure

- **mschedule.c:** Source code for the simulation program.
- **Makefile:** Build automation.
- **input.txt:** Example input file for testing.
- **report.pdf:** Performance analysis and conclusions.

---

## Notes

- Always use proper locks to prevent race conditions and ensure thread safety.
- The maximum number of simultaneous processors is 64.
- Use the `-m 2` or `-m 3` output modes for debugging and detailed logging.

---

## References

- [POSIX Threads](https://man7.org/linux/man-pages/man7/pthreads.7.html)

---

## Author
- Name: Emre Can Yologlu
- Course: CS342 - Operating Systems, Bilkent University

