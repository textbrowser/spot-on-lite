#!/bin/sh
### BEGIN INIT INFO
# Provides:          spot-on-lite-daemon.sh
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start the Spot-On-Lite-Daemon.
# Description:       The cute and wonderful Spot-On-Lite-Daemon.
### END INIT INFO

case "$1" in
  start)
      echo -n "Starting Spot-On-Lite-Daemon... "
      su -c "cd /usr/local/spot-on-lite && exec ./Spot-On-Lite-Daemon --configuration-file /usr/local/spot-on-lite/spot-on-lite-daemon.conf" spot-on-lite-daemon
      ;;
  stop)
      echo -n "Stopping all Spot-On-Lite-Daemon processes owned by spot-on-lite-daemon... "
      killall -u spot-on-lite-daemon Spot-On-Lite-Daemon
      ;;
  *)
      echo "Usage: /etc/init.d/spot-on-lite-daemon.sh {start|stop}"
      exit 1
      ;;
esac

exit $?
