/**
   @file touchadaptor.h
   @brief TouchAdaptor

   <p>
   Copyright (C) 2009-2010 Nokia Corporation

   @author Ustun Ergenoglu <ext-ustun.ergenoglu@nokia.com>
   @author Timo Rongas <ext-timo.2.rongas@nokia.com>

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

#ifndef TOUCHADAPTOR_H
#define TOUCHADAPTOR_H

#include "inputdevadaptor.h"
#include "sensord/deviceadaptorringbuffer.h"
#include <sensord/filterproperty.h>
#include <QTime>
#include <QObject>
#include "touchdata.h"

/**
 * @brief Adaptor for device touchscreen.
 *
 * Provides input data from touchscreen input device.
 *
 * @todo <ul><li>Kernel update will change all the traffic into one handle,
 *           separated by sync messages. Change accordingly.</li>
 */
class TouchAdaptor : public InputDevAdaptor
{
    Q_OBJECT;
    Q_PROPERTY(TouchAdaptor::RangeInfo rangeinfo READ rangeInfo_)
public:
    /**
     * Factory method for gaining a new instance of TouchAdaptor class.
     * @param id Identifier for the adaptor.
     */
    static DeviceAdaptor* factoryMethod(const QString& id)
    {
        return new TouchAdaptor(id);
    }

protected:
    /**
     * Constructor.
     * @param id Identifier for the adaptor.
     */
    TouchAdaptor(const QString& id);
    ~TouchAdaptor();

private:

    /**
     * Holds values read from the driver.
     */
    struct TouchValues_ {
        int x;
        int y;
        int z;
        int volume;
        int toolWidth;
        TouchData::FingerState fingerState;
    };
    typedef struct TouchValues_ TouchValues;

    /**
     * Holds information related to screen properties.
     */
    struct RangeInfo_ {
        int xMin;   /**< Minimum value from the driver for X-axis */
        int yMin;   /**< Minimum value from the driver for Y-axis */
        int xRange; /**< Width of X-axis */
        int yRange; /**< Width of Y-axis */
    };
    typedef struct RangeInfo_ RangeInfo;

    /**
     * Verify whether the input device handle on given path is a touchscreen device.
     * @return True if yes, false if not.
     */
    bool checkInputDevice(QString path, QString matchString = "");

    /**
     * Interpret a read event and store the received value.
     * @param src Event source.
     * @param ev  Read event.
     */
    void interpretEvent(int src, struct input_event *ev);

    /**
     * Pushes recorded events into filterchain.
     * @param src Event source.
     */
    void commitOutput(int src);

    void interpretSync(int src);

    QTime time;
    DeviceAdaptorRingBuffer<TouchData>* outputBuffer_;
    TouchValues touchValues_[5];
    FilterProperty<RangeInfo> rangeInfo_;
};

#endif
