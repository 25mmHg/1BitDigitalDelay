# 1BitDigitalDelay
Delay for Trigger

For researches about synchronisation we need a delay for drum triggering with 1ms resolution and delays of 10 seconds or more.
This is the first working example of our researches. It uses serial communication for adjustment of delay. 
The code is great for very long delays but not so for many trigger events. The prozessing time for values is about 20us without code optimization.
We handle now a queue of 512 trigger events in a 2k ringbuffer.
TODO:
substitute the 512x4Byte Ringbuffer to 1024x2Byte (handle overflow from 50days to 1minute)
better debounce of INPUTs (max input frequency is now 5Hz - 50Hz are better)
