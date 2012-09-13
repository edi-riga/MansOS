#
# Components common for all MSP430 platforms
#

from component_hierarchy import *

serial = SerialOutput()
internalFlash = InternalFlashOutput()

printing = PrintAct()

analogIn = AnalogInputSensor()
digitalIn = DigitalInputSensor()

digitalout = DigitalOutputAct()
analogoput = AnalogOutputAct()

# all kind of pseudo sensors follow

nullSensor = SealSensor("Null") # hack: for virtual use, e.g. min of two sensors

command = CommandSensor()

constant = ConstantSensor()
counter = CounterSensor()
random = RandomSensor()

timecounter = TimeCounterSensor()
systime = SystemTimeSensor()
uptime = UptimeSensor()

squarewave = SquareWaveSensor()
trianglewave = TriangleWaveSensor()
sawwave = SawtoothWaveSensor()
sinewave = SineWaveSensor()

variables = VariableSensor()


#
# Tmote-specific components
#
led = LedAct()
yellowLed = YellowLedAct()