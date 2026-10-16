# LW3. Topic: Design Patterns for Parallel Algorithms

## Task Statement
Implement algorithms from the proposed ones. When implementing, it is allowed to use frameworks that provide templates for parallel algorithms (in this case, the choice of framework must be justified during the defense).

3.1 Parallel "fast" matrix multiplication (Strassen's algorithm).  
Perform testing of the correctness of calculations and comparison of performance with single-threaded implementation.

3.2 Parallel implementation of a sorting algorithm.  
Choose the sorting algorithm yourself, the algorithm complexity must be no worse than $O(N log N)$.  
Testing of the correctness of sorting results and comparison with a single-threaded implementation of the same algorithm must be conducted.


## From Author
The library will provide the required reusable parts for parallel computing. The exact usages and/or configurations will
be stored in the `executables` directory.

3.1 For the matrix multiplication the basis will be implemented to investigate and utilize the Strassen's algorithm;

3.2 For the sorting algorithms, the ones with $O(N log N)$ will be performed
as it is easier to split them into independent parts for future parallel computation (merge sort / heap sort);
