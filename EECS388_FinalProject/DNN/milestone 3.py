import serial
import time
ser1 = serial.Serial('/dev/ttyAMA1', 115200)
ser1.write(b"0123456789123456789")

time.sleep(1.0)

file1 = open("text.txt" , "r")
for line in file1:
	thislist = line.split()
	input_speed = thislist[0]
	input_angle = thislist[1]
	
	if int(input_angle) < 0 and int(input_angle) > -10 :
		sign = "-0"
		input_angle = str(abs(int(input_angle)))
	elif int(input_angle) > 0 and int(input_angle) < 10 :
		sign = "+0"
	elif int(input_angle) == 0:
		sign = "00"
	elif int(input_angle) > 0:
		sign = "+"
	else:
		sign = ""

	Command = "Speed: " + input_speed + " Angle: " + sign + input_angle + " "
	ser1.write(bytes(Command))
	print(Command)
	time.sleep(float(thislist[2]))

file1.close()
ser1.close()
