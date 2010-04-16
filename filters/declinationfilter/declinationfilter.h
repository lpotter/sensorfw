/**
   @file declinationfilter.h
   @brief DeclinationFilter

   <p>
   Copyright (C) 2009-2010 Nokia Corporation

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

#ifndef DECLINATIONFILTER_H
#define DECLINATIONFILTER_H

#include <QObject>
#include "orientationdata.h"
#include "sensord/filter.h"
#include "sensord/filterproperty.h"

/**
 * @brief Filter for calculating declination correction for Compass data..
 *
 */
class DeclinationFilter : public QObject, public Filter<CompassData, DeclinationFilter, CompassData>, public PropertyTracker
{
    Q_OBJECT;
    /**
     * Holds the declination correction amount applied in the calculation.
     * The value is read from GConf key \c /foo/bar.
     */
    Q_PROPERTY(int declinationCorrection READ declinationCorrection_);

public:
    /**
     * Factory method.
     * @return New DeclinationFilter instance as FilterBase*.
     */
    static FilterBase* factoryMethod()
    {
        return new DeclinationFilter();
    }

private:
    DeclinationFilter();

    void correct(unsigned, const CompassData*);
    void loadSettings();
    CompassData orientation;
    CompassData newOrientation;
    FilterProperty<int> declinationCorrection_;
};

#endif // DECLINATIONFILTER_H