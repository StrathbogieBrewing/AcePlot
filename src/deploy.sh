#!/bin/sh -xe

# ssh target info
REMOTEHOST=pi@raspberrypi.local

# upload source code to target
ssh $REMOTEHOST mkdir -p aceplot
rsync -raL --exclude 'cgi-bin' ./ $REMOTEHOST:aceplot/src

ssh $REMOTEHOST /bin/bash <<'EOT'
set -x
cd aceplot/src
make

EOT

# ssh $REMOTEHOST /bin/bash <<'EOT'
# set -x
# pkill logger || true
# pkill busybox || true
# cd aceplot/src
# nohup ./logger -s /dev/ttyUSB0 </dev/null >/dev/null 2>&1 &
# nohup busybox httpd -p 8000 -f
#
# EOT

#
# #rsync -a -e "ssh -p $REMOTEPORT" bin/jackal $REMOTEHOST:bin
# # log on to target and start buslogger
# ssh -p $REMOTEPORT $REMOTEHOST /bin/bash <<'EOT'
# set -x
# # clean and build application
# cd jackal
# make clean
# make all
# # kill any existing instance of jackal
# pkill jackal || true
# # start jackal
# IP=`getent hosts mail.plasmatronics.com.au | awk '{ print $1 }'`
# #bin/jackal /dev/ttyUSB0 $IP $REMOTENAME
# nohup bin/jackal /dev/ttyUSB0 $IP jkl-sb </dev/null >/dev/null 2>&1 &
# sleep 1
# EOT
#
# # ssh target info
# REMOTEHOST=logger@mail.plasmatronics.com.au
# REMOTEPORT="1085"
#
# # clean and build application
# make clean
# make all
#
# # upload to target
# ssh -p $REMOTEPORT $REMOTEHOST mkdir -p www/cgi-bin
# rsync -a -e "ssh -p $REMOTEPORT" bin/busplot $REMOTEHOST:www/cgi-bin
#
#
# #!/bin/sh -xe
#
# # ssh target info
# REMOTEHOST=logger@mail.plasmatronics.com.au
# REMOTEPORT="1085"
#
# # clean and build application
# make clean
# make all
#
# # upload new firmware to target
# ssh -p $REMOTEPORT $REMOTEHOST mkdir -p bin
# rsync -a -e "ssh -p $REMOTEPORT" bin/buslogger $REMOTEHOST:bin
# # log on to target and start buslogger
# ssh -p $REMOTEPORT $REMOTEHOST /bin/bash <<'EOT'
# # set path to log
# LOGPATH="/home/"$USER"/log"
# # kill any existing instance of logger
# pkill buslogger
# # make sure logging directory exists
# mkdir -p $LOGPATH
# # start logger
# nohup /home/"$USER"/bin/buslogger $LOGPATH </dev/null >/dev/null 2>&1 &
# EOT
