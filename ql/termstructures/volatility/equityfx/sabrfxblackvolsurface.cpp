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

#include <ql/termstructures/volatility/equityfx/sabrfxblackvolsurface.hpp>
#include <ql/termstructures/volatility/sabrinterpolatedsmilesection.hpp>
#include <ql/experimental/volatility/zabrinterpolatedsmilesection.hpp>

namespace QuantLib {

    SabrFxBlackVolatilitySurface::SabrFxBlackVolatilitySurface(
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
                const DayCounter& dc,
                bool cubicTimeInterpolation,
                Real gamma)
        : FxBlackVolatilitySurface(deltaVolMatrix, fxSpot, optionTenors, 
        domesticTermStructure, foreignTermStructure, fxFixingDays,
        advanceCalendar, adjustCalendar, fxFixingCalendar, bdc, dc,
        cubicTimeInterpolation), gamma_(gamma) {
    }

    void SabrFxBlackVolatilitySurface::convertQuotes() const {

        std::vector<Volatility> vols(quotesPerSmile_);
        DeltaVolQuote::DeltaType deltaType;
        DeltaVolQuote::AtmType atmType;
        for(Size i=0; i<optionDates().size(); i++) {

            // infer quotation conventions at this tenor
            for(Size j=0; j<quotesPerSmile_; j++) {
                if (deltaVolMatrix_[i][j]->atmType() != DeltaVolQuote::AtmNull) {
                    atmType = deltaVolMatrix_[i][j]->atmType();
                } else {
                    deltaType = deltaVolMatrix_[i][j]->deltaType();
                }
                vols[j] = deltaVolMatrix_[i][j]->value();
            }

            // if necessary, convert to common conventions
            if (deltaType != DeltaVolQuote::Fwd 
                    && atmType != DeltaVolQuote::AtmDeltaNeutral) {
                    
                // set up SABR smile section
                Time optionTime = timeFromReference(optionDates()[i]);
                std::vector<Rate> currentStrikes = 
                                    strikesFromVols(optionTime, vols, 
                                                    deltaType, atmType);   
           
                Real fxFwd = forwardValue(optionTime);
                Real beta = 0.5;
                Real alpha = vols[2] * std::pow(fxFwd, 0.5);
                ext::shared_ptr<SmileSection> p(new 
                        ZabrInterpolatedSmileSection
                                <ZabrShortMaturityNormal>(
                            optionTime, fxFwd, 
                            currentStrikes, false,
                            Null<Volatility>(), vols,
                            alpha, beta, Null<Real>(), Null<Real>(),
                            gamma_, false, false, false, false, true));
               
                // compute vols at required strike levels
                std::vector<Rate> requiredStrikes = 
                                    strikesFromVols(optionTime, vols, 
                                                    DeltaVolQuote::Fwd, 
                                                    DeltaVolQuote::AtmDeltaNeutral);
                for(Size j=0; j<quotesPerSmile_; j++) {
                    vols[j] = p->volatility(requiredStrikes[j]);
                }
            }
            for(Size j=0; j<quotesPerSmile_; j++) {
                volMatrix_[i][j] = vols[j];
            }
        } 
    }

    ext::shared_ptr<SmileSection> 
    SabrFxBlackVolatilitySurface::smileSectionImpl(Time t) const {
        // check for existing smile section in cache--this boosts
        // performance, as setting up the interpolation can be expensive
        ext::shared_ptr<SmileSection> smile(smileCache_.fetchSmile(t));
        if (smile)
            return smile;

        // interpolate vols in time (any strike will do)
        std::vector<Volatility> vols;
        for (Size j=0; j < quotesPerSmile_; ++j) { 
            vols.push_back(volCurves_[j]->blackVol(t, 0));
        }
        // find strikes at interpolated vols
        std::vector<Rate> strikes = strikesFromVols(
                            t, vols, DeltaVolQuote::Fwd,
                            DeltaVolQuote::AtmDeltaNeutral);
        // return interpolated SABR smile section
        Rate fxFwd = forwardValue(t);

        Real beta = 0.5;
        Real alpha = vols[2] * std::pow(fxFwd, 0.5);
        ext::shared_ptr<SmileSection> p(new 
            ZabrInterpolatedSmileSection
                    <ZabrShortMaturityLognormal>(
                t, fxFwd, strikes, false,
                Null<Volatility>(), vols,
                alpha, beta, Null<Real>(), Null<Real>(),
                gamma_, false, false, false, false, true));
        smileCache_.addSmile(t, p);
        return p;
    }

    void SabrFxBlackVolatilitySurface::update() {
        smileCache_.clear();
        FxBlackVolatilitySurface::update();
    }

} // namespace QuantLib