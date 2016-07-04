#include <iostream>
#include <map>
#include <string>
#include <cctype>
#include <set>
#include <memory>
#include <csignal>
#include <random>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
using namespace std;

/* include Semaphore, Atomic Logger and Random number generator (uniformly distributed) */
#include "Semaphore.h"
#include "AtomicLogger.h"
#include "Random.h"

/* constants for easily modifying count of RAILWAYSTATIONS and TRAINS */
const unsigned int MAX_RAILWAYSTATIONS = 8;
const unsigned int MAX_TRAINS = 4;
const unsigned int MAX_TRAINSHIPMENTCAPACITY = 15;
const unsigned int MAX_STATIONSHIPMENTCAPACITY = 20;
const unsigned int MAX_LOADUNLOADTIME = 1000;

/* function to generate unifromly distributed random shipmentCapacity of each train based on its trainId */
int getRandomShipmentCapacity(int trainId)
{
	Random randomShipmentCapacity(MAX_TRAINSHIPMENTCAPACITY);
	return randomShipmentCapacity.generate() + 1;
}
/* function to get start station for each train */
int getRandomTrainStartStation()
{
	Random randomTrainStation(MAX_RAILWAYSTATIONS);
	return randomTrainStation.generate() - 1;
}
/* function to get random capacity of shipments at a station */
int getRandomStationCapacity()
{
	Random randomStationCapacity(MAX_STATIONSHIPMENTCAPACITY);
	return randomStationCapacity.generate();
}
/* function to generate loading unloading time for a train (in milliseconds) */
int getRandomLoadingUnloadingTime()
{
	Random randomLoadUnloadTime(MAX_LOADUNLOADTIME);
	return randomLoadUnloadTime.generate();
}

namespace
{
	volatile sig_atomic_t gSignalStatus = 0;
}

/* Catches the Ctrl+C (SIGINT) signal */
void signal_handler(int signal)
{
	gSignalStatus = signal;
}

/*
	Shipment class
	Each shipment has 
	- id (unique to each shipment)
	- size (size of shipment)
	- destinationId (id of destination station)
*/ 
class Shipment
{
public:
	Shipment() : Shipment(0, 0, 0) {}
	Shipment(unsigned int id_, unsigned int size_, unsigned int destinationId_) : id(id_), size(size_), destinationId(destinationId_) {}
	unsigned int id;
	unsigned int size;
	unsigned int destinationId;
};

/* Structure to compare sizes of two Shipment objects */
struct ShipmentComparator 
{
	bool operator() (const Shipment& ship1, const Shipment& ship2) const
	{
		return ship1.size < ship2.size;
	}
};

/*
	Station class
	Each station has
	- an id (unique id to identify stations)
	- ShipmentCapacityLeft (shipment capacity left at station)
	- ShipmentList (a list of different shipments that are to be loaded to arriving trains)
	- ShipmentMutex (a mutex)
	- isNexSectionFree (a semaphore)
*/
class Station
{
	unsigned int id;
	unsigned int ShipmentCapacityLeft;
	mutex ShipmentMutex;
	set<Shipment, ShipmentComparator> ShipmentList;
	Semaphore isNextSectionFree;
	
public:
	Station() : id() {}
	void init(unsigned int id_, unsigned int ShipmentCapacityLeft_)
	{
		id = id_;
		ShipmentCapacityLeft = ShipmentCapacityLeft_;
	}
	void WaitForNextSection()
	{
		isNextSectionFree.Wait();
	}
	void SetNextSectionFree()
	{
		isNextSectionFree.Notify();
	}
	/* function that returns a shipment that has size <= available shipment capacity of train */
	Shipment TakeShipmentSmallerThan(unsigned int size)
	{
		unique_lock<mutex> lock(ShipmentMutex);
		if (ShipmentList.empty())
			return Shipment(0, 0, 0);

		if (ShipmentList.begin()->size <= size)
		{
			Shipment Shipment = *ShipmentList.begin();
			ShipmentList.erase(ShipmentList.begin());
			return Shipment;
		}
		return Shipment(0, 0, 0);
	}

	void addShipment(Shipment item)
	{
		unique_lock<mutex> lock(ShipmentMutex);
		ShipmentList.insert(item);
		ShipmentCapacityLeft -= item.size;
	}
	unsigned int getShipmentCapacityLeft()
	{
		return ShipmentCapacityLeft;
	}
	string getInfo()
	{
		return "Station " + to_string(id) + " has shipment capacity " + to_string(ShipmentCapacityLeft) + " left.";
	}
};

static mutex stationListMutex;
Station stationList[MAX_RAILWAYSTATIONS];

/*
	Train class
	Each train has
	- id (an id unique to each train)
	- speed (speed of train which is constant throughout)
	- LoadingTime (time in milliseconds for train to load shipment per weight unit
	- UnloadingTime (similar as loading time but used for unloading )
	- ShipmentCapacityLeft (remaining number of shipments train can load)
	- destinationIdShipment 
	- lastStationId (id of last station visited by train)
	- trainState {STATION, SECTION} (a train may be either at a station or at a section i.e. area between 2 stations)
	- logger (an atomic logger to log events of train)
	
	Each train is a thread that goes to stations
	
	All trains start at some station
*/
class Train
{
	enum state { STATION, SECTION };
	unsigned int id;
	unsigned int speed;
	unsigned int LoadingTime, UnloadingTime;

	unsigned int ShipmentCapacityLeft;
	multimap<unsigned int, Shipment> destinationIdShipment;

	unsigned int lastStationId;
	state trainState;
	AtomicLoggerOstreamPtr logger;

	string getInfo()
	{
		return "Train=" + to_string(id) + ", ShipmentCapacityLeft=" + to_string(ShipmentCapacityLeft) + ": ";
	}

public:

	Train(unsigned int id_, unsigned int speed_, unsigned int ShipmentCapacityLeft_, unsigned int lastStationId_, unsigned int LoadingTime_, unsigned int UnloadingTime_, AtomicLoggerOstreamPtr logger_) :
		id(id_), speed(speed_), ShipmentCapacityLeft(ShipmentCapacityLeft_), lastStationId(lastStationId_), LoadingTime(LoadingTime_), UnloadingTime(UnloadingTime_), trainState(Train::STATION), logger(logger_)
	{}

	void TrainDepart()
	{
		Station& currentStation = stationList[lastStationId];
		logger->log("****" + getInfo() + " wants to leave station " + to_string(lastStationId));
		currentStation.WaitForNextSection();
		logger->log("****" + getInfo() + " is departing for station: " + to_string((lastStationId + 1) % MAX_RAILWAYSTATIONS));
	}

	void TrainArrive()
	{
		int stationIdToUnlock = lastStationId;
		if (stationIdToUnlock == 0)
			stationIdToUnlock = MAX_RAILWAYSTATIONS;
		Station& previousStation = stationList[stationIdToUnlock - 1];

		previousStation.SetNextSectionFree();
		logger->log("****" + getInfo() + " arrived to " + to_string(lastStationId));
	}

	void LoadShipmentAtStation()
	{
		Station& currentStation = stationList[lastStationId];
		
		Shipment ShipmentItem;
		while ((ShipmentItem = currentStation.TakeShipmentSmallerThan(ShipmentCapacityLeft)).size > 0)
		{
			logger->log("****" + getInfo() + " loading Shipment of size: " + to_string(ShipmentItem.size));
			ShipmentCapacityLeft -= ShipmentItem.size;

			destinationIdShipment.insert(make_pair(ShipmentItem.destinationId,ShipmentItem));

			/* loading shipment takes time, so making currrent train thread sleep for "ShipmentItem.size*LoadingTime" milliseconds */
			this_thread::sleep_for(chrono::milliseconds(ShipmentItem.size*LoadingTime));
			logger->log("****" + getInfo() + " loaded Shipment of size: " + to_string(ShipmentItem.size) + " for station: " + to_string(ShipmentItem.destinationId));
		}

		logger->log(getInfo() + " no (more) Shipment for this train.");
	}

	void UnloadShipmentAtStation()
	{
		multimap<unsigned int, Shipment>::iterator ShipmentIterator;
		while ((ShipmentIterator = destinationIdShipment.find(lastStationId)) != destinationIdShipment.end())
		{
			unsigned int ShipmentSize = ShipmentIterator->second.size;
			logger->log("****" + getInfo() + " unloading Shipment of size: " + to_string(ShipmentSize));
			ShipmentCapacityLeft += ShipmentSize;
			/* unloading shipment takes time, so making currrent train thread sleep for "ShipmentSize*UnloadingTime" milliseconds */
			this_thread::sleep_for(chrono::milliseconds(ShipmentSize*UnloadingTime));
			logger->log("****" + getInfo() + " unloaded Shipment of size: " + to_string(ShipmentSize) + " for station: " + to_string(ShipmentIterator->second.destinationId));
			destinationIdShipment.erase(ShipmentIterator);
		}

	}


	void run()
	{
		while (gSignalStatus == 0) 
		{
			if (trainState == STATION)
			{
				UnloadShipmentAtStation();
				LoadShipmentAtStation();
				TrainDepart();

				trainState = SECTION;
				this_thread::sleep_for(chrono::seconds(speed));
			}
			if (trainState == SECTION)
			{
				lastStationId = (lastStationId + 1) % MAX_RAILWAYSTATIONS;
				TrainArrive();
				trainState = STATION;
				this_thread::sleep_for(chrono::seconds(speed));
			}
		}
	}
};

/*
	ShipmentCreator Class
	Each ShipmentCreator class has
	- logger (an atomic logger to log the creation the new shipments)
	
	Every ShipmentCreator class object is a thread that creates shipments at random interval of time
*/
class ShipmentCreator
{
	AtomicLoggerOstreamPtr logger;
public:
	explicit ShipmentCreator(AtomicLoggerOstreamPtr logger_) : logger(logger_) {}
	void run()
	{
		/* maximum number of shipment size to be generated */
		const unsigned int MAX_Shipment = 10;	
		/* maximum time shipmentCreator thread sleeps before generating new shipment */
		const unsigned int MAX_SLEEP = 2;

		Random randomSleepSize(MAX_SLEEP);
		Random randomStationSize(MAX_RAILWAYSTATIONS);

		unsigned int ShipmentId = 0;
		while (gSignalStatus == 0)
		{
			/* make the shipmentCreator sleep for a random amount of time */
			this_thread::sleep_for(chrono::seconds(randomSleepSize.generate()));
			unsigned int fromStation = randomStationSize.generate() - 1;
			unsigned int toStation = randomStationSize.generate() - 1;
			/* get a random station that has some shipment capacity left */
			while(stationList[fromStation].getShipmentCapacityLeft() == 0)
				fromStation = randomStationSize.generate() - 1;
			/* both the fromStation and toStation should be different */
			if(toStation == fromStation)
				toStation = (toStation + 1) % MAX_RAILWAYSTATIONS;				
			/* generate a shipment of size <= available shipment capacity at fromStation */
			Random randomShipmentSize(stationList[fromStation].getShipmentCapacityLeft());
			unsigned int ShipmentSize = randomShipmentSize.generate();

			stationList[fromStation].addShipment(Shipment(ShipmentId++, ShipmentSize, toStation));
			logger->log("Generating Shipment at station " + to_string(fromStation) + " of size: " + to_string(ShipmentSize) + " for station: " + to_string(toStation));
			logger->log(stationList[fromStation].getInfo());
		}
	}
};

int main()
{
	/* Install a signal handler */
	signal(SIGINT, signal_handler);
	AtomicLoggerOstreamPtr p_logger(new AtomicLoggerOstream(cout));
	
	for (unsigned int i = 0; i < MAX_RAILWAYSTATIONS; ++i)
	{
		unsigned int stationCapacity = getRandomStationCapacity();
		stationList[i].init(i, stationCapacity);
		cout << stationList[i].getInfo() << endl;
	}


	thread t[MAX_TRAINS];
	/* Each train is a thread that goes to railway stations */
	for (unsigned int i = 0; i < MAX_TRAINS; ++i)
	{
		/* arguments for Train: id, speed, ShipmentCapacity, start station id, loadingTime, unloadingTime, logger) */
		t[i] = thread(&Train::run, 
						Train(	i, 
							(i + 2) , /* for speeds to be distinct, speed is function of trainId */
							getRandomShipmentCapacity(i) , 
							getRandomTrainStartStation(), 
							getRandomLoadingUnloadingTime(), 
							getRandomLoadingUnloadingTime(), 
							p_logger)
				);
	}


	/* Create a ShipmentCreator thread that generates shipment */
	thread ShipmentThread = thread(&ShipmentCreator::run, 
					ShipmentCreator(p_logger)
				);

	ShipmentThread.join();

	for (unsigned int i = 0; i < MAX_TRAINS; ++i)
		t[i].join();

	return 0;
}
