 //========================================================================
// EventManager.h : implements a multi-listener multi-sender event system
//
// Part of the GameEngine Application
//
// GameEngine is the sample application that encapsulates much of the source code
// discussed in "Game Coding Complete - 4th Edition" by Mike McShaffry and David
// "Rez" Graham, published by Charles River Media. 
// ISBN-10: 1133776574 | ISBN-13: 978-1133776574
//
// If this source code has found it's way to you, and you think it has helped you
// in any way, do the authors a favor and buy a new copy of the book - there are 
// detailed explanations in it that compliment this code well. Buy a copy at Amazon.com
// by clicking here: 
//    http://www.amazon.com/gp/product/1133776574/ref=olp_product_details?ie=UTF8&me=&seller=
//
// There's a companion web site at http://www.mcshaffry.com/GameCode/
// 
// The source code is managed and maintained through Google Code: 
//    http://code.google.com/p/GameEngine/
//
// (c) Copyright 2012 Michael L. McShaffry and David Graham
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser GPL v3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See 
// http://www.gnu.org/licenses/lgpl-3.0.txt for more details.
//
// You should have received a copy of the GNU Lesser GPL v3
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//========================================================================


#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "GameEngineStd.h"

#include "Core/Process/CriticalSection.h"
#include "Core/Threading/ThreadSafeQueue.h"

#include <strstream>

/*
	Game Events is the glue that holds the entire game logic and game view architecture together.
	Whenever some authoritative system, such as the game logic, makes changes in the game. The
	game must notify all the appropriate subsystems that the event has occurred so they can handle
	the event in their own way. Game views, such as human view, are subsystems that consumes events
	in order to produce any change according to the recieved event. The game logic makes game
	events to happen which are sent into the systems that know how to distribute the event to any 
	subsystem that wants to listen. In a well-designed game, each subsystem should be responsible for 
	subscribing to and handling game events as they pass through the system. 
	The game event system allows design complex systems that are decoupled from each other while still
	it is possible to communicate to one another. This decoupling allows the systems to grow and change
	organically without affecting any of the other systems they are attached to, as long as they still 
	send and respond to the same events as before. It is global to the application and therefore makes 
	a good candidate to sit in the application layer. 
	It manages all communications going on between the game logic and game views. If the game logic
	makes a change, an event is sent, and all the game views will receive it. If a game view wants to
	send a command to the game logic, it does so through the event system.
	The game event system is organized into three basic parts:
	- Events and event data
	- Event handler delegates
	- EventManager

	Events and event data are generated by authoritative systems when an action of any significance
	occurs, and they are sent into the Event Manager, sometimes also called a listener registry. The
	Event Manager matches each event with all the subsystems that have subscribed to the event and calls
	each event listener delegate function in turn so it can handle the event in its own way.
*/

/*
	Event listener delegates. Events and event data need to go somewhere, and they always go to event listener
	delegate functions. A delegate function is basically a function pointer that can be coupled with an object
	pointer and used as callback. All event listener delegate function must conform a function prototype. To
	declare this function signature as delegate it is used the fast delegate template which is the help class
	used to bind the runtime object (it it exists) with the appropiate function pointer. The "1" means that
	there is one parameter and so on. This is required mostly for compiler compatibility. The template parameter
	is the parameter type to pass into the function. There is an optional additional parameter for the return
	value, which it is'nt used here. EventListenerDelegate is now a typedef of the delegate type. To us the 
	delegate, you instantiate it and call the bind() method. This overloaded method will bind your function
	(or object pointer and function pair) to the delegate object. If you are binding C++ functions, the easiest
	thing to do is to use the global MakeDelegate() function. This function takes in the object pointer and 
	member function pointer and returns a newly constructed delegate object.
*/

//---------------------------------------------------------------------------------------------------------------------
// Forward declaration & typedefs
//---------------------------------------------------------------------------------------------------------------------
class BaseEventData;

typedef unsigned long BaseEventType;
typedef eastl::shared_ptr<BaseEventData> BaseEventDataPtr;
typedef fastdelegate::FastDelegate1<BaseEventDataPtr> EventListenerDelegate;
typedef ConcurrentQueue<BaseEventDataPtr> ThreadSafeEventQueue;


//---------------------------------------------------------------------------------------------------------------------
// Macro for event registration
//---------------------------------------------------------------------------------------------------------------------
extern GenericObjectFactory<BaseEventData, BaseEventType> mEventFactory;
#define REGISTER_EVENT(eventClass) mEventFactory.Register<eventClass>(eventClass::skEventType)
#define CREATE_EVENT(eventType) mEventFactory.Create(eventType)


//---------------------------------------------------------------------------------------------------------------------
// EventData                               - Chapter 11, page 310
// Base type for event object hierarchy, may be used itself for simplest event notifications such as those that do 
// not carry additional payload data. If any event needs to propagate with payload data it must be defined separately.
//---------------------------------------------------------------------------------------------------------------------
class BaseEventData 
{
public:
	virtual ~BaseEventData(void) {}
	virtual const BaseEventType& GetEventType(void) const = 0;
	virtual float GetTimeStamp(void) const = 0;
	virtual void Serialize(std::ostrstream& out) const = 0;
    virtual void Deserialize(std::istrstream& in) = 0;
	virtual BaseEventDataPtr Copy(void) const = 0;
    virtual const char* GetName(void) const = 0;
};


//---------------------------------------------------------------------------------------------------------------------
// class BaseEventData		- Chapter 11, page 311
//---------------------------------------------------------------------------------------------------------------------
class EventData : public BaseEventData
{
    const float mTimeStamp;

public:
	explicit EventData(const float timeStamp = 0.0f) : mTimeStamp(timeStamp) { }

	// Returns the type of the event
	virtual const BaseEventType& GetEventType(void) const = 0;

	float GetTimeStamp(void) const { return mTimeStamp; }

	// Serializing for network input / output
	virtual void Serialize(std::ostrstream &out) const	{ }
    virtual void Deserialize(std::istrstream& in) { }
};


//---------------------------------------------------------------------------------------------------------------------
// BaseEventManager Description                        Chapter 11, page 314
//
// This is the object which maintains the list of registered events and their listeners.
//
// This is a many-to-many relationship, as both one listener can be configured to process multiple event types and 
// of course multiple listeners can be registered to each event type.
//
// The interface to this construct uses smart pointer wrapped objects, the purpose being to ensure that no object 
// that the registry is referring to is destroyed before it is removed from the registry AND to allow for the registry 
// to be the only place where this list is kept ... the application code does not need to maintain a second list.
//
// Simply tearing down the registry (e.g.: destroying it) will automatically clean up all pointed-to objects (so long 
// as there are no other outstanding references, of course).
//---------------------------------------------------------------------------------------------------------------------
class BaseEventManager
{
public:

	enum eConstants { CONS_INFINITE = 0xffffffff };

	explicit BaseEventManager(const char* name, bool setAsGlobal);
	virtual ~BaseEventManager(void);

    // Registers a delegate function that will get called when the event type is triggered.  Returns true if 
    // successful, false if not.
    virtual bool AddListener(const EventListenerDelegate& eventDelegate, const BaseEventType& type) = 0;

	// Removes a delegate / event type pairing from the internal tables.  Returns false if the pairing was not found.
	virtual bool RemoveListener(const EventListenerDelegate& eventDelegate, const BaseEventType& type) = 0;

	// Fire off event NOW.  This bypasses the queue entirely and immediately calls all delegate functions registered 
    // for the event.
	virtual bool TriggerEvent(const BaseEventDataPtr& event) const = 0;

	// Fire off event.  This uses the queue and will call the delegate function on the next call to VTick(), assuming
    // there's enough time.
	virtual bool QueueEvent(const BaseEventDataPtr& event) = 0;
	virtual bool ThreadSafeQueueEvent(const BaseEventDataPtr& event) = 0;

	// Find the next-available instance of the named event type and remove it from the processing queue.  This 
    // may be done up to the point that it is actively being processed ...  e.g.: is safe to happen during event
	// processing itself.
	//
	// if allOfType is true, then all events of that type are cleared from the input queue.
	//
	// returns true if the event was found and removed, false otherwise
	virtual bool AbortEvent(const BaseEventType& type, bool allOfType = false) = 0;

	// Allow for processing of any queued messages, optionally specify a processing time limit so that the event 
    // processing does not take too long. Note the danger of using this artificial limiter is that all messages 
    // may not in fact get processed.
	//
	// returns true if all messages ready for processing were completed, false otherwise (e.g. timeout )
	virtual bool Update(unsigned long maxTime = CONS_INFINITE) = 0;

    // Getter for the main global event manager. This is the event manager that is used by the majority of the 
    // engine, though you are free to define your own as long as you instantiate it with setAsGlobal set to false.
    // It is not valid to have more than one global event manager.
	static BaseEventManager* Get(void);

protected:

	static BaseEventManager* mEventMgr;
};

const unsigned int EVENTMANAGER_NUM_QUEUES = 2;

/*
	The implementation of EventManager manages two sets of objects: event data and listener delegates. As events
	are processed by the system, the EventManager matches them up with subscribed listener delegate functions
	and calls each one with events they care about. There are two ways to send events, by queue and by trigger.
	By queue means the event will sit in line with other events until the game processes EventManager::Update().
	By trigger means the event will be sent immediately by calling each delegate function directly.
*/
class EventManager : public BaseEventManager
{
	/*
		The defined data structure are used to register listener delegate functions. Each event has a list of
		delegates to call when the event is triggered. The EventQueue defines a list of smart pointers to
		BaseEventData objects.
	*/
	typedef eastl::list<EventListenerDelegate> EventListenerList;
	typedef eastl::map<BaseEventType, EventListenerList> EventListenerMap;
	typedef eastl::list<BaseEventDataPtr> EventQueue;

	/*
		There are two event queues here so that delegate methods can safely queue up new events. It is necessary
		to controll the processed queues and also to point the currently active queue
	*/
	EventListenerMap mEventListeners;
	EventQueue mQueues[EVENTMANAGER_NUM_QUEUES];
	int mActiveQueue;  // index of actively processing queue; events enque to the opposing queue

	ThreadSafeEventQueue mRealtimeEventQueue;

public:
	explicit EventManager(const char* name, bool setAsGlobal);
	virtual ~EventManager(void);

	virtual bool AddListener(const EventListenerDelegate& eventDelegate, const BaseEventType& type);
	virtual bool RemoveListener(const EventListenerDelegate& eventDelegate, const BaseEventType& type);

	virtual bool TriggerEvent(const BaseEventDataPtr& event) const;
	virtual bool QueueEvent(const BaseEventDataPtr& event);
	virtual bool ThreadSafeQueueEvent(const BaseEventDataPtr& event);
	virtual bool AbortEvent(const BaseEventType& type, bool allOfType = false);

	virtual bool Update(unsigned long maxTime = CONS_INFINITE);
};


#endif