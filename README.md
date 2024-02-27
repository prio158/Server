HTTP Server 

[Crash Debug] 
>gdb /bin/TestFiberCrash \
>b main \
>r \
>c   
>bt  \
>f 1  \
>f 2  \
>l \
>p t_thread_fiber \
>$1 = std::shared_ptr<Server::Fiber> (empty) = {get() = 0x0}


