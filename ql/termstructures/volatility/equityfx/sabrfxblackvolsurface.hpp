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

/*! \file sabrefxblackvolsurface.hpp
    \brief Noarb SABR FX Black volatility surface
*/

#ifndef sabr_fx_black_vol_surface_hpp
#define sabr_fx_black_vol_surface_hpp

#include <ql/termstructures/volatility/equityfx/fxblackvolsurface.hpp>

namespace QuantLib {

    //! No-arb SABR FX Black volatility surface
    //! FX Black volatility surface using SABR interpolation in strike
    class SabrFxBlackVolatilitySurface : public FxBlackVolatilitySurface {
      public:
        SabrFxBlackVolatilitySurface(
                    const delta_vol_matrix& deltaVolMatrix,
                    const Handle<Quote>& fxSpot, 
                    const std::vector<Period>& optionTenors, 
                    const Handle<YieldTermStructure>& domesticTermStructure,
                    const Handle<YieldTermStructure>& foreignTermStructure,
                    Natural fxFixingDays,
                    const Calendar& advanceCalendar,
                    const Calendar& adjustCalendar = Calendar(), 
                    const Calendar& fxFixingCalendar = WeekendsOnly(),
                    BusinessDayConvention bdc = Following,
                    const DayCounter& dc = Actual365Fixed(),
                    bool cubicTimeInterpolation = false,
                    Real gamma = 1.0);
        //! \name LazyObject interface
        //@{
        virtual void update();
        //|

    protected:
        //! \name FxBlackVolatilitySurface interface
        //@{
        virtual ext::shared_ptr<SmileSection> smileSectionImpl(Time t) const;
        //@}
    private:
        //! \name FxBlackVolatilitySurface interface
        //@{
        virtual void convertQuotes() const;
        //@}
        mutable SmileCache smileCache_;
        Real gamma_;
    };

} // namespace QuantLib
#endif