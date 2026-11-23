import spidev
#IntEnum instead of Enum to make sure Speed.STOP == 1515 and int() casting is possible
from enum import IntEnum 
import struct
import time

#Spi setup, all communication will be 4 byte floats
spi = spidev.SpiDev()
spi.open(0, 0)             # Pi SPI0, CE0
spi.mode = 0
spi.max_speed_hz = 100_000

#Same values understood by the STM32
class Direction(IntEnum):
    RIGHT_MAX = 1999
    CENTER = 1555
    LEFT_MAX = 1111

class Speed(IntEnum):
    FORWARD_MED = 1545
    FORWARD_MAX = 1605
    STOP = 1515
    REVERSE = 1425

#Get user input
def speed_select(user: str) -> Speed:
    match user.strip().lower():
        case "stop":
            return Speed.STOP
        case "forward":
            return Speed.FORWARD_MED
        case "max":
            return Speed.FORWARD_MAX
        case "reverse":
            return Speed.REVERSE
        case _:
            raise ValueError("Invalid speed command")

#Talking with the STM
def talk(value: float) -> float:
    tx = list(struct.pack("<f", value))
    rx = spi.xfer2(tx)
    return struct.unpack("<f", bytes(rx))[0]

def read_only() -> float:
    return talk(0.0)

#Initial values/examples
user_str = "stop"
user_sp = Speed.STOP
d_target:float = 1515 
d_current:float = read_only()

print(f"Starting speed: {d_current:.2f}")

try:
    while True:
        count = 0
        user_str = input("What should be the new speed?\n")
        try:
            user_sp = speed_select(user_str)
        except ValueError as e:
            print(e)
            continue
        d_target = float(user_sp.value) 
        print(f"{d_target}")
        d_current = talk(d_target)
        
        #Loading
        while abs(d_current - d_target) > 0.01:
            time.sleep(0.02) #1 PWM cycle passes
            d_current = talk(d_target)
            print(f"Current speed: {d_current:.2f}")
            count+=1
            if (count > 5):
                print("Over 5")
                count = 0
        print(f"Target met: {d_current:.2f}")

except KeyboardInterrupt:
    print("\nExiting...") 

finally:
    spi.close()
        


