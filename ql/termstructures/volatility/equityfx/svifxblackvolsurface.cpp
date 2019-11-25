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
 FOR A PARTICULAR PURPOSE. See the license for more details.
*/

#include <termstructures/volatility/equityfx/svifxblackvolsurface.hpp>
#include <ql/experimental/volatility/sviinterpolatedsmilesection.hpp>

namespace QuantLib {

    SviFxBlackVolatilitySurface::SviFxBlackVolatilitySurface(
                const delta_vol_matrix& deltaVolMatrix,
                const Handle<Quote>& fxSpot, 
                const std::vector<Period>& optionTenors, 
                const Handle<YieldTermStructure>& domesticTermStructure,
                const Handle<YieldTermStructure>& foreignTermStructure,
                Natural fxFixingDays,
                const Calendar& advanceCalendar,
                const Calendar& adjustCalendar, 
                const Calendar& fxFixingCalendar,
                BusinessDayConvention bdc,
                const DayCounter& dc)
        : FxBlackVolatilitySurface(deltaVolMatrix, fxSpot, optionTenors, 
          domesticTermStructure, foreignTermStructure, fxFixingDays,
          advanceCalendar, adjustCalendar, fxFixingCalendar, bdc, dc) {
    }

    void SviFxBlackVolatilitySurface::convertQuotes() const {

        std::vector<Volatility> vols;
        DeltaVolQuote::DeltaType deltaType;
        DeltaVolQuote::AtmType atmType;
        for(Size i=0; i<optionDates().size(); i++) {

            // determine quotation conventions at this tenor
            vols.clear();
            for(Size j=0; j<quotesPerSmile_; j++) {
                if (deltaVolMatrix_[i][j]->atmType() != DeltaVolQuote::AtmNull) {
                    atmType = deltaVolMatrix_[i][j]->atmType();
                } else {
                    deltaType = deltaVolMatrix_[i][j]->deltaType();
                }
                vols.push_back(deltaVolMatrix_[i][j]->value());
            }

            // if necessary, convert to common conventions
            if (deltaType != DeltaVolQuote::Fwd 
                    && atmType != DeltaVolQuote::AtmDeltaNeutral) {
                    
                // set up SVI smile section
                Time optionTime = timeFromReference(optionDates()[i]);
                std::vector<Rate> currentStrikes = 
                                    strikesFromVols(optionTime, vols, 
                                                    deltaType, atmType);                 
                SviInterpolatedSmileSection smileSection(optionTime, 
                                                         fxForward(optionTime), 
                                                         currentStrikes, false,
                                                         Null<Volatility>(), vols);

                // compute vols at required strike levels
                std::vector<Rate> requiredStrikes = 
                                    strikesFromVols(optionTime, vols, 
                                                    DeltaVolQuote::Fwd, 
                                                    DeltaVolQuote::AtmDeltaNeutral);
                for(Size j=0; j<quotesPerSmile_; j++) {
                    vols[j] = smileSection.volatility(requiredStrikes[j]);
                }
            }
            for(Size j=0; j<quotesPerSmile_; j++) {
                volMatrix_[i][j] = vols[j];
            }
        } 
    }

    ext::shared_ptr<SmileSection> 
    SviFxBlackVolatilitySurface::smileSectionImpl(Time t) const {

        // interpolate vols in time
        std::vector<Volatility> vols;
        for (Size j=0; j < quotesPerSmile_; ++j) { 
            vols.push_back(volCurves_[j].blackVol(t, 0));
        }
        // find strikes at interpolated vols
        std::vector<Rate> strikes = strikesFromVols(
                                            t, vols, DeltaVolQuote::Fwd,
                                            DeltaVolQuote::AtmDeltaNeutral);
        // return interpolated SVI smile section
        Rate fxFwd = fxForward(t);
        return ext::shared_ptr<SmileSection>(new 
                        SviInterpolatedSmileSection(t, fxFwd, strikes, false,
                                                    Null<Volatility>(), vols));
    }

} // namespace QuantLib