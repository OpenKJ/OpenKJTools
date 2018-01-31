#
# Regular cron jobs for the openkjtools package
#
0 4	* * *	root	[ -x /usr/bin/openkjtools_maintenance ] && /usr/bin/openkjtools_maintenance
