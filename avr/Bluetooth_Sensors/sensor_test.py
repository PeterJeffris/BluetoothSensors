# Written by Peter Jeffris for the University of Colorado Boulder Engineering Department
# Usb based datalogging script, detach bluetooth pins to use
import serial,time,curses,pprint
sensors = serial.Serial(port='/dev/ttyACM0',baudrate=115200,timeout=1)
window = curses.initscr()
time.sleep(2)
gyro = [0,0,0]
acc = [0,0,0]
baro = [0,0]
delta = 0

def tc_eval():
    val = int(ord(sensors.read())<<8)+ord(sensors.read())
    if val >= (1<<15):
        val = val-(1<<16)
    return val

acc_scale = 9.81*2*2/(1<<12) 
gyro_scale = 250.0*2/(1<<16)

try:
    sensors.write('A')
    while (True):
        window.erase()
        for i in range(3):
            acc[i] = tc_eval()*acc_scale
        results = "Linear Acceleration\nX: " + str(acc[0]) + "\nY: " + str(acc[1]) + "\nZ: " + str(acc[2]) + "\n"
        for i in range(3):
            gyro[i] = tc_eval()*gyro_scale
        results = results +  "Angular Velocity\nX: " + str(gyro[0]) + "\nY: " + str(gyro[1]) + "\nZ: " + str(gyro[2]) + "\n"
        baro[0] = float((ord(sensors.read())<<8)+ord(sensors.read()))+(float(ord(sensors.read()))/16)
        baro[1] = float(ord(sensors.read()))+(float(ord(sensors.read()))/16)
        results = results + "Altitude: " + str(baro[0]) + " m\nTemperature: " + str(baro[1]) + " C\n"
        delta = float((ord(sensors.read())<<8)+(ord(sensors.read())))/10000000
        frequency = 1/delta
        results = results + "Frequency: " + frequency + " Hz\n"
        window.addstr(results)
        window.refresh()
finally:
    curses.endwin()




