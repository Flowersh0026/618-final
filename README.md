## Summary

We plan to start with implementing a concurrent dynamic sized queue in 4 different synchronization methods and evaluating their performance. The four versions include: coarse-grained locked queue, fine-grained locked queue, lock-free queue using CAS instructions, lock-free queue using transactional memory. Then we would further develop the concurrent queues into distributed versions, and do further performance evaluation there.

## Background

Dynamic sized queue is a commonly used basic data structure. A concurrent multiple producer multiple consumer queue can be useful in some parallel scenarios such as work distribution queue. We want to simulate a worker queue in this project.

There are three base operations for a FIFO queue including push, pop and front. All of the base operations can be done in O(1). Push and pop would modify the queue by inserting/removing elements to/from tail/head, while top will only do a read operation on the head. Since we are simulating a worker queue, in addition to the normal base operations, we will have another blocking pop. When calling a blocking pop, thread will be blocked on an empty queue.

In addition to a centralized queue, under scenarios that the queue needs to serve large amount of works, distributed queue could help a lot when single machine could not hold all the data and could also accelarate performance of locked version queues. Therefore, we also want to evalute the performance of distributed queues and possibly explore different load balancing policies.

## Challenge

It can be inherently challenging to implement concurrent data structures due to the data races issues. Although the locked versions can be relatively easy to accomplicash, the lock-free versions need some efforts to maintain correctness of the algorithm. For transactional memory, TODO

Distributed version can be inherently difficult as we need to maintain the data consistency and the correct ordering among different machines. When an operation taken place, the metadata on each machine, as well as where the head points to, need to be modified accordingly. Additionally, we also need to explore the load balancing policies under various work loads to let it maintain a reasonable performance.

## Resources

We will write code from scratch but using some paper as refereces, as listed below:

## Goals and Deliverables
## Platform Choice
## Schedule

Markdown is a lightweight and easy-to-use syntax for styling your writing. It includes conventions for

```markdown
Syntax highlighted code block

# Header 1
## Header 2
### Header 3

- Bulleted
- List

1. Numbered
2. List

**Bold** and _Italic_ and `Code` text

[Link](url) and ![Image](src)
```

