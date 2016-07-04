# railway-simulator
A multithreaded freight carriage railway system simulator 

**Problem Statement**

On a planet in a Galaxy far far away, there are 8 train stations with a single rail track connecting the train stations. This track forms a big circle. 

There are 4 freight trains, each traveling at a different but constant speed. All trains travel in the same direction. For safety reasons, a trainmay not enter the section of the track between 2 stations if another train is already on that section of the track. In such a case the train needs to wait at the train station until the other train has reached the next train station. At the train stations there are sufficient parallel tracks to allow the trains to overtake one another. 

At the train stations, people from a nearby city will (at random times) deliver shipment that needs to be delivered to another train station. Each piece of shipment will be addressed to a specific train station. 

Trains unload the shipment meant for a train station when they reach that station. Unloading the shipment takes a bit of time for each piece of shipment. Once unloaded, the shipment disappears (it will be taken to a nearby city which is outside the scope of this simulation).  Once all shipment for that station has been unloaded, a train will load all available shipment that needs to be transported to another station. This loading of shipment also takes a bit of time for each piece of shipment. If there are multiple trains at the same station, they may each load some of the available shipment. Trains have a limited shipment capacity, so the loading of shipment will stop when the train is full. A train will continue its journey once all available shipment has been loaded or when the train is full.  

Create a program that simulates the scenario described above using threads. Print all interesting events (arriving at a station, departing, loading, unloading, etc…) to the screen. Design your program so that the number of stations, the number of trains and their shipment capacity can be easily modified. 


**Assumptions**

1. Each train "i" has speed of "i+2", random shipment capacity, random start station, random loading/unloading times
2. Loading/Unloading time for any shipment is equal to (size of shipment*LOADUNLOAD_TIME) in milliseconds. The LOADUNLOAD_TIME variable is defined in the beginning of main.cpp
3. The system does not stop at any point of time
4. Shipments are generated at random intervals of time at random station such that the shipment capacity of the station is not overlaoded
5. Any shipment's source and destination station will always be different

**Compilation**

` make run `

**Execution**

`./a.out`
