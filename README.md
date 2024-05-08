
# CPE-301 Final project
 By Hunter Bickel

This project is a recreation of a swamp cooler, using a water level sensor, a fan, a humidity and temperature sensor, a motor to control the vent, an LCD screen, and a real time clock module.

The project is set up into 4 different "states", so it can technically be considered a state machine. At first, the project is in the "DISABLED" state, meaning that's off. You press the start button to set it to the "IDLE" state, which is when it waits for something to happen. In this state, and in the next states, the project checks if the vent position is being changed, the temp/humidity from the sensor, and the water level. If the temperature is too high in IDLE, it goes to "RUNNING", which turns the fan on, and then back to "IDLE" when the temperature is good enough, turning the fan off. If the water level ever gets too low, it turns to "ERROR", which will only be brought back to "IDLE" if the water level is alright again and the reset button has been pressed.

The temperature to trigger the "RUNNING" state is any temp above 22*C. This is mainly because it is currently cold in my room and I needed an easy way to test the temperature, without using a high enough temperature. The water level trigger is anything below 500 from the sensor. I believe the sensor has been contaminated from being in the water for so long and that's why it needs a high threshold.

The power requirement for this project is just 5v.

Arduino Pinout Sheet: https://www.electronicshub.org/wp-content/uploads/2021/01/Arduino-Mega-Pinout.jpg

Link to video: https://www.dropbox.com/scl/fi/f42ni1jy1qwhkeutx8b0c/IMG_4824.MOV?rlkey=pvkci4yona2chjl5zl7brg1y3&st=rmhsg09t&dl=0