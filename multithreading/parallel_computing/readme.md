# LW3. Topic: Design Patterns for Parallel Algorithms & GPGPU Algorithms

## Task Statement
Deadline: December 15 (inclusive).

Implement algorithms from the proposed ones. Each algorithm must be implemented in two versions: a parallel CPU version and a GPU version.
For the CPU version, it is allowed to use frameworks that provide templates for parallel algorithms (in this case, the choice of framework must be justified during the defense).
The GPU version must be implemented in OpenCL/CUDA (the framework is chosen based on the available hardware).

3.1 Parallel "fast" matrix multiplication (Strassen's algorithm).  
Implement the algorithm for parallel execution on the CPU and for execution on the GPU.  
Perform testing of the correctness of calculations and performance analysis. Compare the performance of both versions with the single-threaded implementation and with each other, and estimate the speedup compared to LW1.  
Points: 0–8

3.2 Parallel implementation of a sorting algorithm.  
Choose the sorting algorithm yourself, the algorithm complexity must be no worse than $O(N log N)$ (e.g. merge sort).  
Implement the same algorithm for parallel execution on the CPU and for execution on the GPU.  
Testing of the correctness of sorting results and performance analysis must be conducted. Compare both versions with a single-threaded implementation of the same algorithm and with each other.  
Points: 0–10

Bonus points:  
On-time defense: 2


## From Author
The library will provide the required reusable parts for parallel computing. The exact usages and/or configurations will
be stored in the `executables` directory.

3.1 For the matrix multiplication the basis will be implemented to investigate and utilize the Strassen's algorithm;

3.2 For the sorting algorithms, the ones with $O(N log N)$ will be performed
as it is easier to split them into independent parts for future parallel computation (merge sort / heap sort);
