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

/*! \file fxblackvolsurface.hpp
    \brief FX Black volatility surface
*/

#ifndef fx_black_vol_surface_hpp
#define fx_black_vol_surface_hpp

#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/volatility/smilesection.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/experimental/fx/deltavolquote.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/weekendsonly.hpp>
#include <ql/math/matrix.hpp>
#include <map>

namespace QuantLib {

    //! FX Black volatility surface
    /*! This class is purely abstract and defines the interface of
        the concrete FX volatility surfaces that will be derived
        from this one.
    */
    class FxBlackVolatilitySurface : public LazyObject,
                                     public BlackVolatilityTermStructure {
      public:
        typedef std::vector<std::vector<Handle<DeltaVolQuote> > > delta_vol_matrix;
        FxBlackVolatilitySurface(
                    const delta_vol_matrix& deltaVolMatrix,
                    const Handle<Quote>& fxSpot, 
                    const std::vector<Period>& optionTenors,
                    const Handle<YieldTermStructure>& domesticTermStructure,
                    const Handle<YieldTermStructure>& foreignTermStructure, 
                    Natural fxSpotDays,
                    const Calendar& advanceCalendar,
                    const Calendar& adjustCalendar,
                    const Calendar& fxFixingCalendar = WeekendsOnly(),
                    BusinessDayConvention bdc = Following,
                    const DayCounter& dc = Actual365Fixed());
        //! \name TermStructure interface
        //@{
        virtual Date maxDate() const;
        virtual Date optionDateFromTenor(const Period&) const;
        //@}
        //! \name VolatilityTermStructure interface
        //@{
        virtual Real minStrike() const;
        virtual Real maxStrike() const;
        //@}
        //! returns the smile for a given option tenor
        ext::shared_ptr<SmileSection> smileSection(const Period& optionTenor,
                                                   bool extrapolate = false) const;
        //! returns the smile for a given option date
        ext::shared_ptr<SmileSection> smileSection(const Date& optionDate,
                                                   bool extrapolate = false) const;
        //! returns the smile for a given option time
        ext::shared_ptr<SmileSection> smileSection(Time optionTime,
                                                   bool extrapolate = false) const;
        //! \name inspectors
        //@{
        std::vector<Date> optionDates() const;
        std::vector<Period> optionTenors() const;
        std::vector<Time> optionTimes() const;
        delta_vol_matrix deltaVolMatrix() const;
        //@}    
        //! \name LazyObject interface
        //@{
        virtual void update();
        virtual void performCalculations() const;
        //@}
        //! \name Visitability
        //@{
        virtual void accept(AcyclicVisitor&);
        //@}
        //! compute fx forward for a particular time
        Rate forwardValue(Time t) const;
      protected:
        //! \name BlackVolTermStructure interface
        //@{
        virtual Volatility blackVolImpl(Time optionTime, Real strike) const;
        //@}

        //! converts vol quotes to a common set of delta and atm conventions
        virtual void convertQuotes() const = 0;

        //! implements the actual smile calculation in derived classes
        virtual ext::shared_ptr<SmileSection> smileSectionImpl(
                                                Time optionTime) const = 0;  

        //! computes strikes corresponding to vol quotes at a particular time
        std::vector<Rate> strikesFromVols(Time t,
                                          std::vector<Volatility> vols,
                                          DeltaVolQuote::DeltaType deltaType,
                                          DeltaVolQuote::AtmType atmType) const;

        // can any of these be made private?
        Size quotesPerSmile_;
        delta_vol_matrix deltaVolMatrix_;
        mutable Matrix volMatrix_;     
        mutable std::vector<ext::shared_ptr<BlackVarianceCurve> > volCurves_;

      private:
        void initializeDates();
        void checkInputs() const;

        //! \name Date calculations
        //@{
        Date spotDate(const Date& fixingDate) const;
        Date fixingDate(const Date& spotDate) const;
        //@}

        Handle<Quote> fxSpot_;
        std::vector<Period> optionTenors_;
        std::vector<Date> optionDates_;
        std::vector<Time> optionTimes_;
        Date fxSpotDate_;
        std::vector<Real> deltas_;
        Handle<YieldTermStructure> domesticTS_;
        Handle<YieldTermStructure> foreignTS_;
        Natural fxSpotDays_;
        Calendar advanceCalendar_;
        Calendar adjustCalendar_;
        Calendar jointCalendar_;
        Calendar fxFixingCalendar_;
    };

    // inline definitions
    inline Date 
    FxBlackVolatilitySurface::maxDate() const { 
        return optionDates_.back(); 
    }

    inline Real 
    FxBlackVolatilitySurface::minStrike() const { 
        return 0; 
    } 

    inline Real 
    FxBlackVolatilitySurface::maxStrike() const { 
        return QL_MAX_REAL; 
    }

    inline std::vector<Date>
    FxBlackVolatilitySurface::optionDates() const {
        return optionDates_;
    }

    inline std::vector<Period>
    FxBlackVolatilitySurface::optionTenors() const {
        return optionTenors_;
    }

    inline std::vector<Times>
    FxBlackVolatilitySurface::optionTimes() const {
        return optionTimes_;
    }

    inline FxBlackVolatilitySurface::delta_vol_matrix
    FxBlackVolatilitySurface::deltaVolMatrix() const {
        return deltaVolMatrix_;
    }

    inline ext::shared_ptr<SmileSection>
    FxBlackVolatilitySurface::smileSection(const Period& optionTenor,
                                           bool extrapolate) const {
        Date optionDate = optionDateFromTenor(optionTenor);
        checkRange(optionDate, extrapolate);
        calculate();
        return smileSection(optionDate, extrapolate);
    }

    inline ext::shared_ptr<SmileSection>
    FxBlackVolatilitySurface::smileSection(const Date& optionDate,
                                           bool extrapolate) const {
        checkRange(optionDate, extrapolate);
        calculate();
        return smileSectionImpl(timeFromReference(optionDate));
    }

    inline ext::shared_ptr<SmileSection>
    FxBlackVolatilitySurface::smileSection(Time optionTime,
                                           bool extrapolate) const {
        checkRange(optionTime, extrapolate);
        calculate();
        return smileSectionImpl(optionTime);
    }

    inline Volatility 
    FxBlackVolatilitySurface::blackVolImpl(Time optionTime, Real strike) const {
        // for small times we extrapolate backwards in flat volatility
        // at constant moneyness
        if (optionTime < optionTimes_.front()) {
            Time t = optionTimes_.front();
            Rate fwd1 = forwardValue(t);
            Rate fwd2 = forwardValue(optionTime);
            return smileSection(t)->volatility(strike*fwd2/fwd1);
        } else
            return smileSection(optionTime)->volatility(strike);
    }

    inline void 
    FxBlackVolatilitySurface::accept(AcyclicVisitor& v) {
        Visitor<FxBlackVolatilitySurface>* v1 = 
            dynamic_cast<Visitor<FxBlackVolatilitySurface>*>(&v);
        if (v1 != 0)
            v1->visit(*this);
        else
            BlackVolatilityTermStructure::accept(v);
    }

} // namespace QuantLib
#endif
