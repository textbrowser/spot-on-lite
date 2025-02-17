#!/usr/bin/env sh
### BEGIN INIT INFO
# Provides:          spot-on-lite-daemon.debian.sh
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
      su -c "cd /opt/spot-on-lite && exec ./Spot-On-Lite-Daemon --configuration-file /opt/spot-on-lite/spot-on-lite-daemon.conf" spot-on-lite-daemon
      ;;
  stop)
      pkill -U spot-on-lite-daemon -f Spot-On-Lite-Daemon
      ;;
  *)
      echo "Usage: /etc/init.d/spot-on-lite-daemon.debian.sh {start|stop}"
      exit 1
      ;;
esac

exit $?
