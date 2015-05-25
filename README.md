# PINE
Pine Is Not an Emulator

PINE is a windows implementation of the pebble c apis. Some of them so far anyway. All of them soon, hopefully.

# Why
Mainly just for fun for me. But, it should eventually be an extremely handy development and debugging tool for 
pebble watchfaces and apps.
Because it is running everything locally in visual studio you get all the power and convenience of a debugger 
to help find and fix all your bugs or try changes out quickly on the fly.

# Building
I've done all of it using the free Visual Studio Express tools so you should be able to use it with the same.
Just download, run create_project.py <appdir>/ and open PINE.sln and you'll be running and debugging right away.
To build, run and debug your own watchfaces and app then you'll need to copy your own watchface into a similar subdir and rerun create_project.py with your own dirname.

# What's supported now
Basically only enough to get the included watch faces working.

# What's not supported yet
Using your own font.  not really sure how to yet or if i want to bother.  And a ton of other APIs but i'll be working my way through them all...hopefully.
