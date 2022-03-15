# BT81X-Bunny-Pizza
A simple video game to be played on Riverdi screen connected with ESP32 microcontroller.


### Current Issues ###
```
-Font of 20 is glitching since this morning (not an issue last week. Glitch remains when loading previous versions of the program)
-Pizzas glitching in "avoid pizzas" version as the y range initialized is larger than in the "collect pizzas" version. Offscreen pizza appears to wrap around to the middle of the screen. 
```
### Features ###
```
-Gameduino library
-Various pages based on gameplay state
-Collision detection controls score and page (win vs lose scenario)
-Character animation responds to touch inputs
-Bitmaps that appear as falling randomly
-Colliding with special items scales character up or down, for 10 seconds. This scaling function can be interrupted by colliding with another special item.
-Special items fall randomly, and only a specified amount of special items fall throughout gameplay.
```
