cmd_/home/sqy0819/linux/tools/labs/skels/./kernel_api/4-list/modules.order := {   echo /home/sqy0819/linux/tools/labs/skels/./kernel_api/4-list/list.ko; :; } | awk '!x[$$0]++' - > /home/sqy0819/linux/tools/labs/skels/./kernel_api/4-list/modules.order
