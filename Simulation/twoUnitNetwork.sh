#!/bin/bash
#
# Start services and create open devices
#
# -s FILE -- settings file

#         socat -x -v -d -d pty,raw,nonblock,ispeed=9600,echo=0 pty,raw,nonblock,ispeed=9600,echo=0

case "$1" in
    init)
        echo "Initialize services for testing"

        # For RS485 network
        socat -x -d  UDP-RECV:2000\!\!UDP-SEND:localhost:2001 pty,raw,ispeed=9600,link=/dev/ttys_d00 &
        echo $! > /tmp/sim_2000_socat.pid
        socat -x -d  UDP-RECV:2002\!\!UDP-SEND:localhost:2003 pty,raw,ispeed=9600,link=/dev/ttys_d02 &
        echo $! > /tmp/sim_2002_socat.pid

        socat -x -d  UDP-RECV:2004 pty,raw,ispeed=9600,link=/dev/ttys_d04 &
        echo $! > /tmp/sim_2004_socat.pid

        # For Application connections to coordinators
        socat -x -v -d -d pty,raw,nonblock,ispeed=9600,echo=0,link=/dev/ttys_a00 pty,raw,nonblock,ispeed=9600,echo=0,link=/dev/ttys_a01 &
        echo $! > /tmp/sim_a00.pid
        socat -x -v -d -d pty,raw,nonblock,ispeed=9600,echo=0,link=/dev/ttys_a02 pty,raw,nonblock,ispeed=9600,echo=0,link=/dev/ttys_a03 &
        echo $! > /tmp/sim_a02.pid

        node-red &
        echo $! > /tmp/sim_0000_nodeRed.pid
        ;;
    deinit)
        echo "Stop and deallocate services"
        for i in $(ls /tmp/sim_*); do
            PID=$(<$i)
            kill -9 $PID
            rm $i
        done
        rm /dev/ttys_*
        ;;
    *) echo "usage: $0 [start|stop]"; exit 1
esac
shift
