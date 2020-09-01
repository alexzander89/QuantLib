/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2019 Alex Winter

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

/*! \file fixedlocalvoladapter.hpp
    \brief Builds a FixedLocalVolSurface from an existing LocalVolTermStructure
*/

#ifndef fixed_local_vol_adapter_hpp
#define fixed_local_vol_adapter_hpp

#include <ql/termstructures/volatility/equityfx/fixedlocalvolsurface.hpp>
#include <ql/termstructures/volatility/equityfx/localvolsurface.hpp>

namespace QuantLib {

    class FixedLocalVolSurfaceAdapter : public LocalVolTermStructure {
      public:
        explicit FixedLocalVolSurfaceAdapter(
            const Handle<LocalVolSurface>& localVol,
            Real xMax,
            Real xMin = 1E-3,
            Size tGrid = 100, 
            Size xGrid = 100);
        //! \name TermStructure interface
        //@{
        const Date& referenceDate() const;
        DayCounter dayCounter() const;
        Time maxTime() const;
        Date maxDate() const;
        //@}
        //! \name VolatilityTermStructure interface
        //@{
        Real minStrike() const;
        Real maxStrike() const;
        //@}
      protected:
        Volatility localVolImpl(Time, Real) const;
      private:
        ext::shared_ptr<FixedLocalVolSurface> fixedLocalVol_;
    };

    inline const Date& 
    FixedLocalVolSurfaceAdapter::referenceDate() const {
        return fixedLocalVol_->referenceDate();
    }
    inline DayCounter 
    FixedLocalVolSurfaceAdapter::dayCounter() const {
        return fixedLocalVol_->dayCounter();
    }
    inline Time
    FixedLocalVolSurfaceAdapter::maxTime() const {
        return fixedLocalVol_->maxTime();
        
    }
    inline Date 
    FixedLocalVolSurfaceAdapter::maxDate() const {
        return fixedLocalVol_->maxDate();
    }
    inline Real
    FixedLocalVolSurfaceAdapter::minStrike() const {
        return fixedLocalVol_->minStrike();
    }
    inline Real
    FixedLocalVolSurfaceAdapter::maxStrike() const {
        return fixedLocalVol_->maxStrike();
    }
    inline Volatility 
    FixedLocalVolSurfaceAdapter::localVolImpl(Time t, Real s) const {
        return fixedLocalVol_->localVol(t, s);
    }

} // namespace QuantLib

#endif