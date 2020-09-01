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

#include <ql/termstructures/volatility/equityfx/kahalefxblackvolsurface.hpp>
#include <ql/termstructures/volatility/kahalesmilesection.hpp>

#include <ql/termstructures/volatility/interpolatedsmilesection.hpp>
#include <ql/experimental/volatility/sviinterpolatedsmilesection.hpp>

namespace QuantLib {

    KahaleFxBlackVolatilitySurface::KahaleFxBlackVolatilitySurface(
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
                bool interpolate, 
                bool exponentialExtrapolation,
                bool deleteArbitragePoints)
        : FxBlackVolatilitySurface(deltaVolMatrix, fxSpot, optionTenors, 
        domesticTermStructure, foreignTermStructure, fxFixingDays,
        advanceCalendar, adjustCalendar, fxFixingCalendar, bdc, dc,
        cubicTimeInterpolation), interpolate_(interpolate),
        exponentialExtrapolation_(exponentialExtrapolation),
        deleteArbitragePoints_(deleteArbitragePoints) {
    }

    void KahaleFxBlackVolatilitySurface::convertQuotes() const {

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
                    
                // set up smile section
                Time optionTime = timeFromReference(optionDates()[i]);
                std::vector<Rate> currentStrikes = 
                                    strikesFromVols(optionTime, vols, 
                                                    deltaType, atmType);   
           
                Real fxFwd = forwardValue(optionTime);
                std::vector<Real> money;
                //std::vector<Real> stdDevs;
                for (Size i = 0; i < currentStrikes.size(); i++) {
                    money.push_back(currentStrikes[i] / fxFwd);
                    //stdDevs.push_back(vols[i]*std::sqrt(optionTime));
                }
                ext::shared_ptr<SmileSection> sec1(
                    new SviInterpolatedSmileSection(
                                            optionTime, fxFwd, currentStrikes, false,
                                            Null<Volatility>(), vols));
                    //new InterpolatedSmileSection<Linear>(
                    //    optionTime, currentStrikes, stdDevs, fxFwd));
                ext::shared_ptr<KahaleSmileSection> p(
                    new KahaleSmileSection(
                        sec1, fxFwd, interpolate_, exponentialExtrapolation_, 
                        deleteArbitragePoints_, money));
               
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
    KahaleFxBlackVolatilitySurface::smileSectionImpl(Time t) const {
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

        // set up smile section
        std::vector<Real> money;
        //std::vector<Real> stdDevs;
        for (Size i = 0; i < strikes.size(); i++) {
            money.push_back(strikes[i] / fxFwd);
            //stdDevs.push_back(vols[i]*std::sqrt(t));
        }
        ext::shared_ptr<SmileSection> sec1(
            //new InterpolatedSmileSection<Linear>(
            //    t, strikes, stdDevs, fxFwd));
            new SviInterpolatedSmileSection(t, fxFwd, strikes, false,
                                            Null<Volatility>(), vols));
            
        ext::shared_ptr<KahaleSmileSection> p(
            new KahaleSmileSection(
                sec1, fxFwd, interpolate_, exponentialExtrapolation_, 
                deleteArbitragePoints_, money));
        smileCache_.addSmile(t, p);
        return p;
    }

    void KahaleFxBlackVolatilitySurface::update() {
        smileCache_.clear();
        FxBlackVolatilitySurface::update();
    }

} // namespace QuantLib