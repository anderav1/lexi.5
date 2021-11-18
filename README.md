# Project 4: Resource Management

CS 4760: Operating Systems  
Lexi Anderson  


## Overview
This program simulates resource management inside a system. The main program 
forks user processes and controls their primary activities--resource requests, 
resource releases, and terminations. The program uses deadlock detection to 
determine how it will allocate resources.

## Running the Program
Invoke the program using the command:
```
oss
```

The command has no options or arguments.

## Interprocess Communication
To facilitate communication between the main program and the user processes, 
I used a message queue to carry data back and forth between them. The user processes 
must send a message to `oss` to be allocated resources, to release their allocated 
resources, and to terminate.

## Difficulties
I worked off of the queue struct I built in the previous project to create a 
queue of active processes for this program. My queue has an underlying int array structure, 
so the head and tail are accessed via their respective indices, and they must be updated 
by incrementing those indices. When a process is removed from the queue, I represent 
that by assigning a value of 0 to the corresponding index.  
This implementation got a bit tricky to work with when trying to read and write the 
matrices for resource allocation, maximum resources needed, etc.
