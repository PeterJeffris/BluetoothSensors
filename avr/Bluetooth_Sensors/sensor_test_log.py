# Usb based datalogging script, detach bluetooth pins to use
import serial,time,curses,pprint
sensors = serial.Serial(port='/dev/ttyACM0',baudrate=115200,timeout=1)

time.sleep(2)
gyro = [0,0,0]
acc = [0,0,0]
baro = [0,0]

def tc_eval():
    val = int(ord(sensors.read())<<8)+ord(sensors.read())
    if val >= (1<<15):
        val = val-(1<<16)
    return val

acc_scale = 9.81*2*2/(1<<12) 
gyro_scale = 250.0*2/(1<<16)

f = open('datalog.csv', 'w')
intial_time = time.time();

while (True):
    sample_time = time.time() - intial_time
    f.write(str(sample_time))
    sensors.write('A')
    for i in range(3):
        acc[i] = tc_eval()*acc_scale
    f.write("," + str(acc[0]) + ", " + str(acc[1]) + "," + str(acc[2]))
    sensors.write('G')
    for i in range(3):
        gyro[i] = tc_eval()*gyro_scale
    f.write("," + str(gyro[0]) + ", " + str(gyro[1]) + "," + str(gyro[2]))
    sensors.write('B')
    baro[0] = float((ord(sensors.read())<<8)+ord(sensors.read()))+(float(ord(sensors.read()))/16)
    baro[1] = float(ord(sensors.read()))+(float(ord(sensors.read()))/16)
    f.write("," + str(baro[0]) + ", " + str(baro[1]) + "\n")





