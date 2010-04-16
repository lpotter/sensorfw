/**
   @file bin.cpp
   @brief Bin

   <p>
   Copyright (C) 2009-2010 Nokia Corporation

   @author Semi Malinen <semi.malinen@nokia.com
   @author Joep van Gassel <joep.van.gassel@nokia.com>
   @author Timo Rongas <ext-timo.2.rongas@nokia.com>
   @author Ustun Ergenoglu <ext-ustun.ergenoglu@nokia.com>

   This file is part of Sensord.

   Sensord is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   Sensord is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Sensord.  If not, see <http://www.gnu.org/licenses/>.
   </p>
 */

#include "bin.h"
#include "pusher.h"
#include "consumer.h"
#include "filter.h"
#include "source.h"
#include "sink.h"
#include "callback.h"
#include "ringbuffer.h"
#include "logging.h"


void PrivateThread::run()
{
    thread_.run();
}


class Bin::Command
{
public:
    virtual ~Command() {}

    virtual void execute(Bin& t) = 0;
    // TODO
};


class StopCommand : public Bin::Command
{
public:
    void execute(Bin& t)
    {
        t.running_ = false;
    }
};


class Bin::CommandReader : public RingBufferReader<Command*>
{
public:
    CommandReader(Bin& t) :
        thread_(t)
    {}

    void pushNewData()
    {
        Bin::Command* command;
        while (read(1, &command)) {
            command->execute(thread_);
        }
    }

private:
    Bin& thread_;
};


Bin::Bin() :
    thread_(*this),
    commands_(new RingBuffer<Command*>(256)),
    commandReader_(new CommandReader(*this)),
    signalNewEvent_(this, &Bin::signalNewEvent),
    running_(false)
{
    commandReader_->setReadyCallback(&signalNewEvent_);
    commands_->join(commandReader_);
}

Bin::~Bin()
{
    stop();
    delete commands_;
    delete commandReader_;
}

void Bin::start()
{
    running_ = true;
    thread_.start();
}

void Bin::stop()
{
    if (thread_.isRunning()) {
        // TODO: lure the thread to stop
        static StopCommand s;
        Command* c = &s;
        SinkBase* commandSinkBase = commands_->sink("sink");
        SinkTyped<Command*>* commandSink;
        commandSink = dynamic_cast<SinkTyped<Command*>*>(commandSinkBase);
        commandSink->collect(1, &c);
        thread_.wait(); // TODO: set the timeout
    }
}

void Bin::add(Pusher* pusher, const QString& name)
{
    pusher->setReadyCallback(&signalNewEvent_);

    // TODO: check for same name in producers_ and filters_
    pushers_.insert(name, pusher);
}

void Bin::add(Consumer* consumer, const QString& name)
{
    // TODO: check for same name in consumers_ and filters_
    consumers_.insert(name, consumer);
}

void Bin::add(FilterBase* filter, const QString& name)
{
    // TODO: check for same name in filters_ and producers_
    filters_.insert(name, filter);
}

bool Bin::join(const QString& producerName,
                  const QString& sourceName,
                  const QString& consumerName,
                  const QString& sinkName)
{
    bool joined = false;

    SourceBase* src = source(producerName, sourceName);
    SinkBase*   snk = sink(consumerName, sinkName);

    if (src && snk) {
        if (src->join(snk)) {

            joined = true;

        } else {
            sensordLogT() << "source"
                          << producerName << "/" << sourceName
                          << "and sink"
                          << consumerName << "/" << sinkName
                          << "are of incompatible types";
        }
    } else {
        if (!src) {
            sensordLogT() << "source"
                     << producerName << "/" << sourceName
                     << "not found";
        }
        if (!snk) {
            sensordLogT() << "sink"
                     << consumerName << "/" << sinkName
                     << "not found";
        }
    }

    return joined;
}

bool Bin::unjoin(const QString& producerName,
                  const QString& sourceName,
                  const QString& consumerName,
                  const QString& sinkName)
{
    SourceBase* src = source(producerName, sourceName);
    SinkBase*   snk = sink(consumerName, sinkName);

    bool unjoined = false;

    if (src && snk) {
        if (src->unjoin(snk)) {

            unjoined = true;

        } else {
            sensordLogT() << "Cannot unjoin sink & source. Possibly, they are not connected.";
        }
    } else {
        if (!src) {
            sensordLogT() << "source"
                     << producerName << "/" << sourceName
                     << "not found";
        }
        if (!snk) {
            sensordLogT() << "sink"
                     << consumerName << "/" << sinkName
                     << "not found";
        }
    }

    return unjoined;
}

SourceBase* Bin::source(const QString& producerName,
                           const QString& sourceName)
{
    SourceBase* source = 0;

    if (Producer* p = producer(producerName)) {
        source = p->source(sourceName);
    }

    return source;
}

SinkBase* Bin::sink(const QString& consumerName, const QString& sinkName)
{
    SinkBase* sink = 0;

    if (Consumer* c = consumer(consumerName)) {
        sink = c->sink(sinkName);
    }

    return sink;
}

Producer* Bin::producer(const QString& name)
{
    Producer* p;

    if (pushers_.contains(name)) {
        p = pushers_.value(name);
    } else if (filters_.contains(name)) {
        p = filters_.value(name);
    } else {
        p = 0;
    }

    return p;
}

Consumer* Bin::consumer(const QString& name)
{
    Consumer* c;

    if (consumers_.contains(name)) {
        c = consumers_.value(name);
    } else if (filters_.contains(name)) {
        c = filters_.value(name);
    } else {
        c = 0;
    }

    return c;
}


void Bin::run()
{
    while (running_) {
        waitForNewEvents();
        handleEvents();
    }
}

// TODO: use eventfd(2) instead of mutex and condvar
void Bin::signalNewEvent()
{
    if (!isNewEvent_.fetchAndStoreRelaxed(true)) {
        mutex_.lock();
        newEvent_.wakeAll();
        mutex_.unlock();
    }
}

void Bin::waitForNewEvents()
{
    if (!isNewEvent_.fetchAndStoreRelaxed(false)) {
        mutex_.lock();
        if (!isNewEvent_.fetchAndStoreRelaxed(false)) {
            newEvent_.wait(&mutex_);
            isNewEvent_ = false;
        }
        mutex_.unlock();
    }
}

void Bin::handleEvents()
{
    // execute new commands
    commandReader_->pushNewData();

    // filter new data
    Pusher* pusher;
    foreach (pusher, pushers_) {
        pusher->pushNewData();
    }
}