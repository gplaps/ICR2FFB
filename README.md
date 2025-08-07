#FFB for ICR2 BETA 0.1 USE AT YOUR OWN RISK


This is a custom Force Feedback application for the classic racing simulator **IndyCar Racing II**. It reads telemetry directly from memory and tries to apply realistic force feedback to supported racing wheels.

This app works by reading memory from the game for speed and tire positions, and uses that to send direct input to wheels to approximate force feedback


PLEASE USE AT YOUR OWN RISK
	- I have tried to make sure it can't break your wheel or you, but no guarantees! I have been using it without any issues but I can't promise it will work on all systems/devices!

	Installation
	1. Download the app
	2. Open the ffb.ini and edit the following:
		a. "Device" - This should match exactly your device name as you see it in "Game Controllers" in windows
		b. "Game" - This can either by 'indycar' or 'cart' depending on what your exe is called (maybe other words too)
		c. "Version" - this can either be "REND32A" for rendition or "DOS4G" for regular dos (sorry no windy yet)
		d. "Force" - This controls the force scale, i have it set to 25% to start PLEASE BE CAREFUL I have tried to make my code to not send a massive input to the wheel but you can never be too careful
	3. Start the app and open the game, after a moment you should see the window start populating some telemetry. You may need to run the app in admin mode to get it to work since it needs to be able to scan the memory of Indycar.exe

If at any point you want to change a setting you can do so by closing the app, making the change and then reopening the program. It can be closed and reopened while ICR2 is running, just be careful since the effect will start after it starts collecting data (probably pause the game first)


#Version Stuff
Betas
#0.1 25-8-7
	- Sent to Git

Alphas
#0.7 25-8-7
	- Made the app detect Dosbox in general, should find all versions now
	- Redid slip calculations to give better sense of oversteer
	- Updated the display refresh so the app doesn't flicker as it runs (as much)
	- Redid all the error logging and created a "log.txt" which collects everything
#0.6 25-8-7
	- Added 'invert' toggle to the ini
	- Added fix for defining 'direction' logic in forces for hopeful compatibility in Moza wheels
#0.5 25-8-6
	- Massive code cleanup and notes
	- Increased polling rate to 60hz, this should improve the feel or responsiveness quite a lot
	- Added EXEs to make sure we're all using the same thing
#0.4 25-8-6
	- Made a ReadMe!
	- Moved the version history to here!
	- Installing Visual Studio is no longer required
	- The program should no longer apply force until Telemetry is being read from the game
#0.3 25-8-5 Better slip calculation to make more detail in the constant force
#0.2 25-8-5 Corrected using DOS4G
#0.1 25-8-5 first version