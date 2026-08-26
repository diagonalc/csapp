# Concurrency Programing

### Process

fork whenever there's a new connection

### IO multiplexing

creating a read_set that stores the fds, use select function to find a active connection (for example, after adding STDIN_FILENO into the read_set, it's corresponding set bit remains 0 when there is no input command, and it toggles to 1 whenever there is input and `select()` can be used to update the FD_SET)

`select() ` is used to update the FD_SET

to archieve multiplexing, we can create a while(1) loop to continually and rapidly to services a small chunk of work (like echoing a single line) for the clients and immediately yields control back to the `select()`

### Threads

similar to using process, create a threads whenever there's a new connection, or prethread...
