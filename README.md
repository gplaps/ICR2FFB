# FFB for ICR2 – BETA 0.6  
**USE AT YOUR OWN RISK**

This is a custom Force Feedback application for the classic racing simulator **IndyCar Racing II** by Papyrus.

This app works by reading memory from the game (speed, tire loads, etc.) and uses that to send DirectInput force effects to your wheel.

Its all a lot of guesswork and workarounds bandaided together but it feels ok!

This is my first ever big program, so I am sure it could be done better in almost every aspect, let me know if theres an obvious mistake!

Big thanks to SK and Eric H for all their help in bringing this to life.

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

## Version History

### Betas

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