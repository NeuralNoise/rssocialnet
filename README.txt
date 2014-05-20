This is a Retroshare plugin designed to provide an instant Web UI to retroshare,
based on Wt, the Qt-style library for web UIs. 

This will provide users an immediate web app that is easy to configure and run.

COMPILATION on Ubuntu
=====================
	- install packages 
			> sudo apt-get install witty-dev libwt-dev

	In the later, assume that the RSWapp Plugin code is in ~/Code/retroshare/trunk/plugins/RSWebUI/
	This is required to get the retroshare_plugin.pri file in ../common/

	- compilation
			> cd ~/Code/retroshare/trunk/plugins/RSWebUI/
			> qmake CONFIG=debug
			> make

		This should create the following files:

          libWebUI.so -> libWebUI.so.1.0.0
          libWebUI.so.1 -> libWebUI.so.1.0.0
          libWebUI.so.1.0 -> libWebUI.so.1.0.0
          libWebUI.so.1.0.0
			
	- create a link to the plugin in retroshare plugin directory
			> cd ~/.retroshare/extensions
			> ln -s ~/Code/RSWebUI/libWebUI.so.1.0.0 libWebUI.so

	- run Retroshare, or retroshare-nogui, and enable the RSWebUI plugin

	- In Web browser (e.g. chromium-browser) go to localhost:9090

