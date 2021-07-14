gdbpid=$(ps ax -o pid,cmd | grep Logic | grep renderer |   awk '{print $1}')
echo PID: $gdbpid
gdb -p $gdbpid

