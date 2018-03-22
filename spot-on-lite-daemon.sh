#!/bin/sh
### BEGIN INIT INFO
# Provides:          spot-on-lite-daemon.sh
# Required-Start:    $network $remote_fs $time
# Required-Stop:     $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start the Spot-On-Lite-Daemon.
# Description:       The cute and wonderful Spot-On-Lite-Daemon.
### END INIT INFO

. /lib/lsb/init-functions

case "$1" in
  start)
      su -c "cd /usr/local/spot-on-lite && exec ./Spot-On-Lite-Daemon --configuration-file /usr/local/spot-on-lite/spot-on-lite-daemon.conf" spot-on-lite-daemon
      ;;
  stop)
      killall -u spot-on-lite-daemon Spot-On-Lite-Daemon
      ;;
  *)
      echo "Usage: /etc/init.d/spot-on-lite-daemon.sh {start|stop}"
      exit 1
      ;;
esac

exit $?
