/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2014 Peter Caspers

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

/*! \file sviinterpolatedsmilesection.hpp
    \brief svi interpolating smile section
*/

#ifndef quantlib_svi_interpolated_smile_section_hpp
#define quantlib_svi_interpolated_smile_section_hpp

#include <ql/handle.hpp>
#include <ql/patterns/lazyobject.hpp>
#include <ql/termstructures/volatility/smilesection.hpp>
#include <ql/experimental/volatility/sviinterpolation.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

namespace QuantLib {

class Quote;
class SviInterpolatedSmileSection : public SmileSection, public LazyObject {
  public:
    //! \name Constructors
    //@{
    //! all market data are quotes
    SviInterpolatedSmileSection(
        const Date &optionDate, const Handle<Quote> &forward,
        const std::vector<Rate> &strikes, bool hasFloatingStrikes,
        const Handle<Quote> &atmVolatility,
        const std::vector<Handle<Quote> > &volHandles, 
        Real a = Null<Real>(), Real b = Null<Real>(),
        Real sigma = Null<Real>(), Real rho = Null<Real>(), 
        Real m = Null<Real>(), bool aIsFixed = false, 
        bool bIsFixed = false, bool sigmaIsFixed = false, 
        bool rhoIsFixed = false, bool mIsFixed = false,
        bool vegaWeighted = true,
        const ext::shared_ptr<EndCriteria> &endCriteria =
            ext::shared_ptr<EndCriteria>(),
        const ext::shared_ptr<OptimizationMethod> &method =
            ext::shared_ptr<OptimizationMethod>(),
        const DayCounter &dc = Actual365Fixed());
    
    SviInterpolatedSmileSection(
        Time optionTime, const Handle<Quote> &forward,
        const std::vector<Rate> &strikes, bool hasFloatingStrikes,
        const Handle<Quote> &atmVolatility,
        const std::vector<Handle<Quote> > &volHandles, 
        Real a = Null<Real>(), Real b = Null<Real>(),
        Real sigma = Null<Real>(), Real rho = Null<Real>(), 
        Real m = Null<Real>(), bool aIsFixed = false, 
        bool bIsFixed = false, bool sigmaIsFixed = false, 
        bool rhoIsFixed = false, bool mIsFixed = false,
        bool vegaWeighted = true,
        const ext::shared_ptr<EndCriteria> &endCriteria =
            ext::shared_ptr<EndCriteria>(),
        const ext::shared_ptr<OptimizationMethod> &method =
            ext::shared_ptr<OptimizationMethod>(),
        const DayCounter &dc = Actual365Fixed());

    //! no quotes
    SviInterpolatedSmileSection(
        const Date &optionDate, Rate forward,
        const std::vector<Rate> &strikes, bool hasFloatingStrikes,
        Volatility atmVolatility, const std::vector<Volatility> &vols,
        Real a = Null<Real>(), Real b = Null<Real>(),
        Real sigma = Null<Real>(), Real rho = Null<Real>(), 
        Real m = Null<Real>(), bool aIsFixed = false, 
        bool bIsFixed = false, bool sigmaIsFixed = false, 
        bool rhoIsFixed = false, bool mIsFixed = false,
        bool vegaWeighted = true,
        const ext::shared_ptr<EndCriteria> &endCriteria =
            ext::shared_ptr<EndCriteria>(),
        const ext::shared_ptr<OptimizationMethod> &method =
            ext::shared_ptr<OptimizationMethod>(),
        const DayCounter &dc = Actual365Fixed());

    SviInterpolatedSmileSection(
        Time optionTime, Rate forward,
        const std::vector<Rate> &strikes, bool hasFloatingStrikes,
        Volatility atmVolatility, const std::vector<Volatility> &vols,
        Real a = Null<Real>(), Real b = Null<Real>(),
        Real sigma = Null<Real>(), Real rho = Null<Real>(), 
        Real m = Null<Real>(), bool aIsFixed = false, 
        bool bIsFixed = false, bool sigmaIsFixed = false, 
        bool rhoIsFixed = false, bool mIsFixed = false,
        bool vegaWeighted = true,
        const ext::shared_ptr<EndCriteria> &endCriteria =
            ext::shared_ptr<EndCriteria>(),
        const ext::shared_ptr<OptimizationMethod> &method =
            ext::shared_ptr<OptimizationMethod>(),
        const DayCounter &dc = Actual365Fixed());
    //@}
    //! \name LazyObject interface
    //@{
    virtual void performCalculations() const;
    virtual void update();
    //@}
    //! \name SmileSection interface
    //@{
    Real minStrike() const;
    Real maxStrike() const;
    Real atmLevel() const;
    //@}
    Real varianceImpl(Rate strike) const;
    Volatility volatilityImpl(Rate strike) const;
    //! \name Inspectors
    //@{
    Real a() const;
    Real b() const;
    Real sigma() const;
    Real rho() const;
    Real m() const;
    Real rmsError() const;
    Real maxError() const;
    EndCriteria::Type endCriteria() const;
    std::vector<Volatility> inputVols() const;
    std::vector<Rate> inputStrikes() const;
    //@}

  protected:
    //! Creates the mutable SviInterpolation
    void createInterpolation() const;
    mutable ext::shared_ptr<SviInterpolation> sviInterpolation_;

    //! Market data
    const Handle<Quote> forward_;
    const Handle<Quote> atmVolatility_;
    std::vector<Handle<Quote> > volHandles_;
    mutable std::vector<Rate> strikes_;
    //! Only strikes corresponding to valid market data
    mutable std::vector<Rate> actualStrikes_;
    bool hasFloatingStrikes_;

    mutable Real forwardValue_;
    mutable std::vector<Volatility> vols_;
    //! Svi parameters
    Real a_, b_, sigma_, rho_, m_;
    //! Svi interpolation settings
    bool isAFixed_, isBFixed_, isSigmaFixed_, isRhoFixed_, isMFixed_;
    bool vegaWeighted_;
    const ext::shared_ptr<EndCriteria> endCriteria_;
    const ext::shared_ptr<OptimizationMethod> method_;
};

inline void SviInterpolatedSmileSection::update() {
    LazyObject::update();
    SmileSection::update();
}

inline Real SviInterpolatedSmileSection::volatilityImpl(Rate strike) const {
    calculate();
    return (*sviInterpolation_)(strike, true);
}

inline Real SviInterpolatedSmileSection::a() const {
    calculate();
    return sviInterpolation_->a();
}

inline Real SviInterpolatedSmileSection::b() const {
    calculate();
    return sviInterpolation_->b();
}

inline Real SviInterpolatedSmileSection::sigma() const {
    calculate();
    return sviInterpolation_->sigma();
}

inline Real SviInterpolatedSmileSection::rho() const {
    calculate();
    return sviInterpolation_->rho();
}

inline Real SviInterpolatedSmileSection::m() const {
    calculate();
    return sviInterpolation_->m();
}

inline Real SviInterpolatedSmileSection::rmsError() const {
    calculate();
    return sviInterpolation_->rmsError();
}

inline Real SviInterpolatedSmileSection::maxError() const {
    calculate();
    return sviInterpolation_->maxError();
}

inline EndCriteria::Type SviInterpolatedSmileSection::endCriteria() const {
    calculate();
    return sviInterpolation_->endCriteria();
}

inline std::vector<Volatility> SviInterpolatedSmileSection::inputVols() const {
    calculate();
    return vols_;
}

inline std::vector<Rate> SviInterpolatedSmileSection::inputStrikes() const {
    calculate();
    return strikes_;
}

inline Real SviInterpolatedSmileSection::minStrike() const {
    calculate();
    return actualStrikes_.front();
}

inline Real SviInterpolatedSmileSection::maxStrike() const {
    calculate();
    return actualStrikes_.back();
}

inline Real SviInterpolatedSmileSection::atmLevel() const {
    calculate();
    return forwardValue_;
}
}

#endif
