---
title: "The resource container"
---

The resource container has been created to make sure the lifetime of dependant resources outlive the lifetime of the consumer.

For example a command buffer must ensure that all the resources it's using don't die until it finished its execution.


```c++
class resource_container
{
public:
    template<_t>
    void add_resource(std::shared_ptr<_t> resource);

    // (...)
}
```
