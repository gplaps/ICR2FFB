# FFB for ICR2 – BETA 1.0.2
**USE AT YOUR OWN RISK**

This is a custom Force Feedback application for the classic racing simulator **IndyCar Racing II** by Papyrus.

This app works by reading memory from the game (speed, tire loads, etc.) and uses that to send DirectInput force effects to your wheel.

Its all a lot of guesswork and workarounds bandaided together but it feels ok!

This is my first ever big program, so I am sure it could be done better in almost every aspect, let me know if theres an obvious mistake!

Big thanks to SK, Eric H, Niels Heusinkveld and Hatcher for all their help in bringing this to life.

---

## DISCLAIMER – USE AT YOUR OWN RISK

I have tried my best to make sure this app can't hurt you or your wheel but **no guarantees**.  
I’ve been using it without issues on my own hardware, but your mileage may vary.

---

## Installation

1. **Download** the app. Get the latest version from Releases
2. **Open `ffb.ini`** and edit the following:
    - `Device` — Must match your device name **exactly** as seen in Windows "Game Controllers".
    - `Game` — Set to either `indycar` or `cart` depending on your executable name.
    - `Version` — Set to `REND32A` for Rendition builds, or `DOS4G` for standard DOS builds.  
      _(Windy not supported yet.)_
    - `Force` — Controls force scale (default is 25%).  
      **Be careful** — although the code limits input, always test with low force first.

3. **Run the app**, then launch the game.  
   After a moment, the window should begin showing telemetry.

> You may need to run the app in **Admin mode** so it can access IndyCar II’s memory.

---

## Changing Settings

You can close the app, edit `ffb.ini`, and reopen it while ICR2 is still running.  
To avoid sudden force application, **pause the game first** before restarting the app.

---

## Build from source 

0. Prerequisites: CMake and a version of MSVC compiler (2013 and 2019 tested) or MinGW GCC / MinGW Clang
1. `mkdir build && cd build`
2. MSVC/Default compiler: `cmake ..` or MinGW: `cmake -DCMAKE_TOOLCHAIN_FILE=cmake/MinGW.cmake -DTOOLCHAIN_PREFIX=<path-to-mingw>/bin/x86_64-w64-mingw32 -DCMAKE_INSTALL_PREFIX=<path-is-your-choice e.g. ../installed> ..`
3. `cmake --build . -j4 --target install`
4. Start program as usual

---

## Version History

### Betas
**1.0.2 (2025-09-05)** 
- Now supports the Windows version (WINDY). Use 'ICR2WND' in the config. Big thanks to hatcher for help in figuring it out, this specifically supports the "second wind" version which you can find here: https://grandprix2.racing/file/misc/view/windy-gets-a-second-wind.

**1.0.1 (2025-08-27)** 
- Some large structural changes to begin adding support (hopefully) for additional games in the future
- Changed Game definition in the ffb.ini. It now uses a simplified 'Game:' setting which can be set to 'ICR2DOS' for the Dos version or 'ICR2REND' for the rendition version. 
- Redid FFB curve to give better center feeling
- Output display will include game version now
- Removed some Legacy code for calculations (slip/lateral load)

**0.9.1 (2025-08-20)**
- Code structure improved
- Log file is opened and closed only once
- Reduce change of deadlocks
- Improve game detection by excluding certain keywords
- Ini layout with sections
- Write a defaulted ini file if non exists beside binary

**0.9.0 (2025-08-18)** 
- Added longitudinal tire forces (we think) to the constant force calculation. Now braking or accelerating values can have an effect on the force feedback. Although you cannot lock a tire in ICR2, you can still feel better now if you have a potential weight shift under braking! 
- Added "Braking Scale:" option to the config. This can be used to tune the longitudinal forces proportional to the existing latitude forces on the wheel
- Added Raw Longitudinal forces to output telemetry
- Redid app timing with a timed game loop rather than sleep timer to avoid windows timing issues (thank Hatcher)

**0.8.9 (2025-08-18)** 
- Updated ffb.ini to support using a joystick index or the name. This will be easier for some with strange characters in the joystick name
- Removed some debug junk from the display
- Removed the sleep timer from the main compute thread

**0.8.8 (2025-08-14)** 
- Actually fixed the asymetrical error. All versions up until this had it! WOW!

**0.8.7 (2025-08-14)** 
- Fixed an error in how i was reading some game data that resulted in the output being asymetrical.

**0.8.5 (2025-08-14)** 
- Redid force curve, again, but even better this time. Niels has supplied some knowledge and the main constant force curve is now being calculated on front tire load alone. This removes the separate 'constant' vs 'weight' settings in the INI, but the results are much much better!

**0.8 (2025-08-13)** 
- Redid force curve to be MUCH stronger overall. This is to give the most detail possible in normal driving conditions. You may need to lower your overall FFB %. I also found lowering the "Weight" is a good idea, I've set the default for that to 75%. It should now be possible to 'max out' your wheel
- Added some force which is applied by speed. This should help smooth out oscillation in a straight line (fingers crossed)
- Added "Deadzone" option to ini. I do not recommend setting this unless you have issues with the direction swapping too eratically (also try the limiter in that case)
- Added some new logging
- Added License stuff

**0.78 (2025-08-12)** 
- Trying to fix compatibility and oscillation issues with T500RS and other belt wheels. Added option for "Limit" which when set to true tries to limit the frequency the FFB updates are sent to the wheel. This can help wheels which can't process 60hz refresh rates, or where frequent FFB changes make them go crazy. I have tested without this on a G27 and it works fine, so most older wheels should be ok, but belt wheels like the T500RS may need it.

**0.75 (2025-08-11)** 
- Fixed app from crashing if the window is clicked

**0.7 (2025-08-11)** 
- Added more options for configuration. In the INI file there are now independant scale options and toggles for each force.
- I have also added toggles/force scale for the new 'weight' force which gives the surface and camber change feeling. Its now possible to turn this part of the force off, or reduce its scale compared to the other effects.

**0.65 (2025-08-10)** 
- Fixed an issue where the new weight effects did not scale based on the INI value

**0.6 (2025-08-10)** 
- You can now feel surface (grass vs dirt vs tarmac) and camber changes in the road!
- Added new code to use the force split between left and right-side which has allowed for a ton of detail to be added which helps remove the 'smooth' feeling of previous versions.
- Updated the display to show the actual force being delivered to the wheel (0 - 10000 units)

**0.5 (2025-08-10)** 
- A big update based on some new tire telemetry found (thanks Eric!). These new tire values are thought to be the amount of friction each tires has and show much more varied data in different conditions. To take advantage of these I rewrote the entire calculations and forces pipeline to be cleaner and better represent the forces. The forces are approximated into real-life data and force feedback calculations are based on this. The results are more predictable than feedback was based before for oversteer/understeer conditions. Overall the feedback is still missing detail for changes in the road or distinct feeling for going off into the dirt/grass. Hopefully we find more physics values which could be brought in to bring this more to life.
- At this time 'slip' does not directly factor into the feedback, but because the tires themselves report loss of grip the feeling of oversteer/understeer is still quite present.
- Fixed pausing/unpausing logic, it should remove force now
- Fixed a lot of display data to make sure its not flickering/dispalys cleanly
- Log now tracks more things
- Code cleanup is still needed to remove old slip and lateral calculation code, it is no longer used in constant force

**0.4 (2025-08-08)** 
- Updated to use magnitude to determine force direction instead of a direction parameter. This may fix effects only working in one direction for Moza wheels
- Rewrote main display to clean up some of the telemetry weirdness
- App now asks for admin permissions which are needed anyway to look at the memory of ICR2

**0.3 (2025-08-08)**  
- Fixes to make forces correct for non-Fanatec wheels, may still be in progress
- Reduced the rate of sending updates to the wheel with a filter to only send updates when they're "meaningful". It still sends updates fast but previously where i sent an update every 16ms no matter what, this will slow them down to try not to overwhelm older wheels.
- Logging updates, preventing the program from crashing if it cannot find the controller

**0.2 (2025-08-08)**  
- Fixed issues with Thrustmaster wheels not working with constant force due to pausing effects

**0.1 (2025-08-07)**  
- Initial beta pushed to Git

---

### Alphas

**0.7 (2025-08-07)**  
- App now detects all DOSBox versions, not just specific titles  
- Improved slip calculations for oversteer detection  
- Reduced display flicker while running  
- Full error logging to `log.txt`

**0.6 (2025-08-07)**  
- Added `invert` toggle to `ffb.ini`  
- Improved force direction compatibility (e.g. for Moza wheels)

**0.5 (2025-08-06)**  
- Major code cleanup and comments  
- Increased polling rate to 60 Hz  
- Added example EXEs to unify setups

**0.4 (2025-08-06)**  
- Created this README  
- Moved version history here  
- No longer requires Visual Studio  
- Forces only apply after telemetry starts

**0.3 (2025-08-05)**  
- Improved slip calculation for more detailed constant force

**0.2 (2025-08-05)**  
- Corrected support for `DOS4G` executables

**0.1 (2025-08-05)**  
- First working version

---