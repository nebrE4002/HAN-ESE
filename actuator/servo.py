
# We imports the GPIO module
import RPi.GPIO as GPIO
# We import the command sleep from time
from time import sleep

GPIO.cleanup() # Clean up all the ports we've used.

# Stops all warnings from appearing
GPIO.setwarnings(False)

# We name all the pins on BOARD mode
GPIO.setmode(GPIO.BOARD)
# Set an output for the PWM Signal
GPIO.setup(16, GPIO.OUT)


pwm = GPIO.PWM(16, 50)
# Set up the PWM on pin #16 at 50Hz
while True:
	pwm.start(0) # Start the servo with 0 duty cycle ( at 0 deg position )
	pwm.ChangeDutyCycle(5) # Tells the servo to turn to the left ( -90 deg position )
	sleep(0.5) # Tells the servo to Delay for 5sec
	pwm.ChangeDutyCycle(10) # Tells the servo to turn to the right ( +90 deg position )
	sleep(0.5) # Tells the servo to Delay for 5sec
	
