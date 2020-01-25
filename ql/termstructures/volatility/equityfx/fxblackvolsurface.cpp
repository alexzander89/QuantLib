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

#include <ql/time/calendars/jointcalendar.hpp>
#include <ql/termstructures/volatility/equityfx/fxblackvolsurface.hpp>
#include <ql/experimental/fx/blackdeltacalculator.hpp>
#include <ql/utilities/dataformatters.hpp>

namespace QuantLib {

    FxBlackVolatilitySurface::FxBlackVolatilitySurface(
                const delta_vol_matrix& deltaVolMatrix,
                const Handle<Quote>& fxSpot, 
                const std::vector<Period>& optionTenors,
                const Handle<YieldTermStructure>& domesticTermStructure,
                const Handle<YieldTermStructure>& foreignTermStructure, 
                Natural fxSpotDays,
                const Calendar& advanceCalendar,
                const Calendar& adjustCalendar,
                const Calendar& fxFixingCalendar,
                BusinessDayConvention bdc,
                const DayCounter& dc)
        : BlackVolatilityTermStructure(0, NullCalendar(), bdc, dc), 
        quotesPerSmile_(deltaVolMatrix.front().size()), 
        deltaVolMatrix_(deltaVolMatrix), fxSpot_(fxSpot),
        volMatrix_(deltaVolMatrix.size(), quotesPerSmile_, 0), 
        optionTenors_(optionTenors), optionDates_(optionTenors.size()), 
        optionTimes_(optionTenors.size()), deltas_(quotesPerSmile_), 
        domesticTS_(domesticTermStructure), foreignTS_(foreignTermStructure), 
        fxSpotDays_(fxSpotDays), advanceCalendar_(advanceCalendar), 
        adjustCalendar_(adjustCalendar), fxFixingCalendar_(fxFixingCalendar) {
        
        initializeDates();
        checkInputs();
        //register with DeltaVolQuotes
        //registerWith(fxSpot_); 
        //registerWith(domesticTermStructure);
        //registerWith(foreignTermStructure);
    }

    std::vector<Rate> FxBlackVolatilitySurface::strikesFromVols(
                                        Time t,
                                        std::vector<Volatility> vols,
                                        DeltaVolQuote::DeltaType deltaType,
                                        DeltaVolQuote::AtmType atmType) const {
        QL_REQUIRE(vols.size() == quotesPerSmile_,
                   "vector of vols must be of length " << quotesPerSmile_);
        std::vector<Rate> strikes;
        for(Size j=0; j<quotesPerSmile_; j++) {
            Option::Type ot = deltas_[j] > 0 ? Option::Call : Option::Put;
            BlackDeltaCalculator dbc(
                ot, deltaType,
                fxSpot_->value(),
                domesticTS_->discount(t),
                foreignTS_->discount(t),
                std::sqrt(t)*vols[j]);
            if(deltas_[j] == Null<Real>())
                strikes.push_back(dbc.atmStrike(atmType));
            else 
                strikes.push_back(dbc.strikeFromDelta(deltas_[j]));
        }
        return strikes;
    }

    Rate FxBlackVolatilitySurface::forwardValue(Time t) const {
        // This is an approximation: the forward value should 
        // technically be computed by discounting from the delivery
        // date corresponding to time t back to the fx spot date. 
        // Determining this delivery date is problematic though, 
        // since we cannot easily map from time to dates. 
        DiscountFactor dfDom = domesticTS_->discount(t);
        DiscountFactor dfFor = foreignTS_->discount(t);
        Real fwd = fxSpot_->value()*dfFor/dfDom;
        return fwd;
    }

    void FxBlackVolatilitySurface::initializeDates() {
        for(Size j=0; j<quotesPerSmile_; j++) {
            if(deltaVolMatrix_.front()[j]->atmType() != DeltaVolQuote::AtmNull)
                deltas_[j] = Null<Real>();
            else 
                deltas_[j] = deltaVolMatrix_.front()[j]->delta();
        }
        jointCalendar_ = JointCalendar(advanceCalendar_,
                                       adjustCalendar_,
                                       JoinHolidays);
        fxSpotDate_ = spotDate(referenceDate());
        for (Size i=0; i<optionTenors_.size(); ++i) {
            optionDates_[i] = optionDateFromTenor(optionTenors_[i]);
            optionTimes_[i] = timeFromReference(optionDates_[i]);
        }
    }

    void FxBlackVolatilitySurface::checkInputs() const {
        QL_REQUIRE(optionTenors_.size() > 0, 
                   "at least one date required");
        QL_REQUIRE(quotesPerSmile_ > 2, 
                   "at least three vol quotes required at each tenor");
        QL_REQUIRE(optionTenors_.size() == deltaVolMatrix_.size(), 
                   "mismatch between dimension of date vector ("
                   << optionTenors_.size() << ") and dimension of vol matrix ("
                   << deltaVolMatrix_.size() << ")");
        QL_REQUIRE(std::count(deltas_.begin(), deltas_.end(), Null<Real>()) == 1,
                   "smiles must contain a single atm quote");
        QL_REQUIRE(domesticTS_->referenceDate() == referenceDate(),
                   "reference date of domestic term structure (" 
                   << domesticTS_->referenceDate() << 
                   ") must match that of volatility term structure ("
                   << referenceDate() << ")");
        QL_REQUIRE(foreignTS_->referenceDate() == referenceDate(),
                   "reference date of foreign term structure (" 
                   << foreignTS_->referenceDate() << 
                   ") must match that of volatility term structure ("
                   << referenceDate() << ")"); 
        for(Size i=0; i<optionTenors_.size(); i++) {
            QL_REQUIRE(deltaVolMatrix_[i].size() == quotesPerSmile_,
                       io::ordinal(i+1) << "row of vol matrix contains "
                       << deltaVolMatrix_[i].size() << " vol quotes, whereas "
                       << "1st row contains " << quotesPerSmile_);
            QL_REQUIRE(referenceDate() < optionDates_[i], 
                       "option dates must be greater than reference date ("
                       << referenceDate() << ")");
            if(i > 0) {
                QL_REQUIRE(optionDates_[i] > optionDates_[i-1], 
                           "option dates must be increasing");
            }
            for(Size j=0; j<quotesPerSmile_; j++) {
                QL_REQUIRE(deltaVolMatrix_[i][j]->deltaType() 
                                        == deltaVolMatrix_[i][0]->deltaType(),
                           io::ordinal(i+1) << "row of vol matrix uses "
                           << " more than one delta convention");
                if(deltas_[j] == Null<Real>()) 
                    QL_REQUIRE(deltaVolMatrix_[i][j]->atmType() != DeltaVolQuote::AtmNull,
                               "deltas of  " << io::ordinal(i+1) << "row of vol matrix "
                               << "do not match those in 1st row");
                else
                    QL_REQUIRE(deltaVolMatrix_[i][j]->delta() == deltas_[j],
                               "deltas of  " << io::ordinal(i+1) << "row of vol matrix "
                               << "do not match those in 1st row");
            }
        }
    }

    Date FxBlackVolatilitySurface::spotDate(const Date& fixingDate) const {
        QL_REQUIRE(fxFixingCalendar_.isBusinessDay(fixingDate),
                   "FX fixing date " << fixingDate << " is not valid")
        if (fxSpotDays_ == 0)
            // Calendar::advance() adjusts the date when the number of fixing
            // days is zero; to avoid this behaviour we set the FX spot date
            // explicitly in this case
            return fixingDate;
        else {
            Date d = advanceCalendar_.advance(fixingDate, fxSpotDays_, Days);
            return jointCalendar_.adjust(d);
        }
    }

    Date FxBlackVolatilitySurface::fixingDate(const Date& spotDate) const {
        QL_REQUIRE(jointCalendar_.isBusinessDay(spotDate),
                   "FX spot date " << spotDate << " is not valid");
        Date fixingDate = advanceCalendar_.advance(spotDate,
            -static_cast<Integer>(fxSpotDays_), Days);
        return fixingDate;
    }

    Date FxBlackVolatilitySurface::optionDateFromTenor(const Period& p) const {
        BusinessDayConvention bdc = ModifiedFollowing;
        if (p.units() == Days ||  p.units() == Weeks) {
            bdc = Following;
        } 
        Date deliveryDate =  jointCalendar_.advance(fxSpotDate_,
                                                    p, bdc, true);
        return fixingDate(deliveryDate);
    }

    void FxBlackVolatilitySurface::performCalculations() const {
        convertQuotes();
        // initialise time interpolations
        std::vector<Volatility> vols;
        volCurves_.clear();
        for(Size j=0; j<quotesPerSmile_; j++) {
            vols.clear();
            for(Size i=0; i<optionDates().size(); i++) {
                vols.push_back(volMatrix_[i][j]);
            }
            // We cannot safely copy instances of the BlackVarianceCurve
            // because it contains an Interpolation instance but uses a 
            // default copy constructor. We therefore store a vector of  
            // pointers instead. In C++11, an alternative would have been  
            // to use move semantics, or to construct the curve vector 
            // in-place, without copying, using emplace_back.
            volCurves_.push_back(
                ext::make_shared<BlackVarianceCurve>(referenceDate(), 
                                                     optionDates(),
                                                     vols,
                                                     dayCounter()));
            volCurves_[j]->enableExtrapolation();
        }
    }

    void FxBlackVolatilitySurface::update() {
        // recalculate dates if necessary...
        if (moving_) {
            initializeDates();
        }
        BlackVolatilityTermStructure::update();
        LazyObject::update();
    }

} // namespace QuantLib
