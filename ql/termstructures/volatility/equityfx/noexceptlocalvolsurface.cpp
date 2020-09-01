/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2020 Alex Winter

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/termstructures/volatility/equityfx/noexceptlocalvolsurface.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/quotes/simplequote.hpp>

namespace QuantLib {


    Volatility NoExceptLocalVolSurface::localVolImpl(Time t, Real underlyingLevel)
                                                                     const {
        DiscountFactor dr = riskFreeYield()->discount(t, true);
        DiscountFactor dq = dividendYield()->discount(t, true);
        Real forwardValue = underlying()->value()*dq/dr;
        
        // strike derivatives
        Real strike, y, dy, strikep, strikem;
        Real w, wp, wm, dwdy, d2wdy2;
        strike = underlyingLevel;
        y = std::log(strike/forwardValue);
        dy = ((std::fabs(y) > 0.001) ? y*0.0001 : 0.000001);
        strikep=strike*std::exp(dy);
        strikem=strike/std::exp(dy);
        w  = volSurface()->blackVariance(t, strike,  true);
        wp = volSurface()->blackVariance(t, strikep, true);
        wm = volSurface()->blackVariance(t, strikem, true);
        dwdy = (wp-wm)/(2.0*dy);
        d2wdy2 = (wp-2.0*w+wm)/(dy*dy);

        // time derivative
        Real dt, wpt, wmt, dwdt;
        if (t==0.0) {
            dt = 0.0001;
            DiscountFactor drpt = riskFreeYield()->discount(t+dt, true);
            DiscountFactor dqpt = dividendYield()->discount(t+dt, true);           
            Real strikept = strike*dr*dqpt/(drpt*dq);
        
            wpt = volSurface()->blackVariance(t+dt, strikept, true);
            if (wpt<w)
                return illegalLocalVolOverwrite_;

            dwdt = (wpt-w)/dt;
        } else {
            dt = std::min<Time>(0.0001, t/2.0);
            DiscountFactor drpt = riskFreeYield()->discount(t+dt, true);
            DiscountFactor drmt = riskFreeYield()->discount(t-dt, true);
            DiscountFactor dqpt = dividendYield()->discount(t+dt, true);
            DiscountFactor dqmt = dividendYield()->discount(t-dt, true);
            
            Real strikept = strike*dr*dqpt/(drpt*dq);
            Real strikemt = strike*dr*dqmt/(drmt*dq);
            
            wpt = volSurface()->blackVariance(t+dt, strikept, true);
            wmt = volSurface()->blackVariance(t-dt, strikemt, true);

            if (wpt<w || w<wmt)
                return illegalLocalVolOverwrite_;
         
            dwdt = (wpt-wmt)/(2.0*dt);
        }

        if (dwdy==0.0 && d2wdy2==0.0) { // avoid /w where w might be 0.0
            return std::sqrt(dwdt);
        } else {
            Real den1 = 1.0 - y/w*dwdy;
            Real den2 = 0.25*(-0.25 - 1.0/w + y*y/w/w)*dwdy*dwdy;
            Real den3 = 0.5*d2wdy2;
            Real den = den1+den2+den3;
            Real result = dwdt / den;

            if (result < 0.0)
                return illegalLocalVolOverwrite_;

            return std::sqrt(result);
        }
    }

} // namespace QuantLib