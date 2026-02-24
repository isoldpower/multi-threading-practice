# LW2. Topic: Development of thread-safe data structures
## Task Statement
Develop a thread-safe data structure that supports potentially simultaneous access by multiple threads (readers and writers). The data structure is to be chosen from the provided list.  
Develop a pipeline for testing the correctness and performance of the thread-safe data structure. Investigate the performance of the thread-safe data structure. As a baseline implementation for comparison, take the same structure with coarse-grained locking. Compare the performance for different numbers of threads interacting with the data structure, analyze the dependency on the ratio of reader and writer threads. Present performance metrics averaged over runs, and include graphs in the report.

2.1 Fine-grained locking data structure

2.2 Lock-free data structure

## Data structures (choose one)

**unbounded queue;**  
The queue must provide pop and push methods. The pop method must be implemented both in a blocking variant (wait_pop) for the case of an empty queue, and in a non-blocking variant (try_pop).  

**bounded queue;**  
The queue must have an upper size limit (the size is defined during initialization). Pop and push methods must have blocking and non-blocking variants.  
The 2.2 implementation may be done with preallocated memory and based on an array.  

**singly linked list;**  
Must provide insert, delete, find methods 

# From Author
As the task is completed not at the deadline and for learning purposes only, an implementation of **ALL 3** structures 
will be present here. The mechanism for running pipeline will be held in the `{root}/{project_name}/utilities` library
as it will be shared across all the Laboratory works and used only in the `executables` itself. 
