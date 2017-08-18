# Network Buffer - circular-like network buffer with raw access in C++

## Design
Network Buffer is a buffer implemntation to satisfy the following requirements:
- Conservative memory allocation
- Non-fragmentation guarantees for the caller
- Conservative memory copying
- Raw access to underlying buffer for C-like interfaces

These requirements arose from writing a Boost Asio-based server.

### Conservative memory allocation
By it's nature network buffer has to accept data of initially unknown size. 
In naive implementation this can be solved by allocating memory for each request
depending on that request's size requirements. This however is not a good solution
for a server application, since the performance will suffer from frequent memory
allocation/deallocation calls.

For FIFO pipeline, a circular buffer can be used. In this case memory is preallocated
before hand. 

### Non-fragmentation guarantees for the caller and conservative memory copying
In order to do packet parsing conviniently it is often nice to have the buffer to
be parsed as continuous chunk of memory. Then the data structure can be easily
described declaratively and doesn't require imperative parsing.
Circular buffer can lead to physically fragmented data if the data is located over
the underlying memory chunk boundary. A simple approach is just to move the data
within the physical buffer to defragment it. Another solution is to avoid receiving
the data past over the underlying buffer by providing the caller with the information
on maximum unfragmented size that can be received. Both approaches are taken in
this implementation, with a heavy bias against the first one. If desired by the programmer,
the memory move/copy can be avoided wholy by using the second approach. 

### Raw access
The implementation has both push/pop data interface as well as the ability to pass
the raw buffer for writing. This way it can be utilized with any basic interface
that requires a memory pointer to the buffer and it's size to write data.


