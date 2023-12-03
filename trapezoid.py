import math as m

MAX_SPEED = 230     #steps/sec
ACCEL = 1000        #steps/sec*sec
FIRST_DELAY = 1200
TPS = 100000         #clock ticks per second

LUT = []
speed = TPS/FIRST_DELAY


while (speed < MAX_SPEED):
    #append to LUT
    delay = TPS / speed
    LUT.append(m.ceil(delay))

    #set next speed
    speed = speed + (ACCEL * delay/TPS)
    print(speed)

#append max speed
LUT.append(m.ceil(TPS / MAX_SPEED))

print ()

le = len(LUT)
print(le)
print(LUT)

print("90 deg turn time:")
print(LUT[le-1] * (50 - 2 * le) + 2 * sum(LUT))


print("180 deg turn time:")
print(LUT[le-1] * (100 - 2 * le) +  2 * sum(LUT))












