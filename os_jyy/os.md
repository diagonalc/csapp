# Operational System Note
### handle
A handle is like a namespace, there might be many functions with a same name, so we need to specify which dictionary we are using when we are trying to look for a particular one. Therefore, we need both a handle and the function's name to find the address of the function by using `dlsym`.\
By providing the .so (shared object, like a dynamic link library) file path, `dlopen` will return a `void *` type handle. \
In addition, the handle also manages the library's lifetime: as long as it's not closed via `dlclose`, the library stays loaded and the returned address remains valid.