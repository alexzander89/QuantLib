

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

#include <ql/time/daycounters/actual360.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/svifxblackvolsurface.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/calendars/all.hpp>
#include <ql/instruments/vanillaoption.hpp>
#include <ql/termstructures/volatility/equityfx/localvolsurface.hpp>
#include <ql/pricingengines/vanilla/fdblackscholesvanillaengine.hpp>
#include <ql/termstructures/volatility/equityfx/fixedlocalvoladapter.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/exercise.hpp>
#include <iostream>

using namespace QuantLib;

namespace {

    struct DeltaVolData {
        Volatility vol;
        Real delta;
        DeltaVolQuote::DeltaType deltaType;
        DeltaVolQuote::AtmType atmType;
    };
}

int main(int, char* []) {

    DayCounter dc = Actual360();
    Date today(1, February, 2019);
    Settings::instance().evaluationDate() = today;
    Natural fxFixingDays = 2;
    Calendar advanceCal = TARGET();
    Calendar adjustCal_ = UnitedStates();

    // term structures
    ext::shared_ptr<SimpleQuote> spotFx(new SimpleQuote(1.1));
    ext::shared_ptr<SimpleQuote> forRate(new SimpleQuote(0.01));
    ext::shared_ptr<SimpleQuote> domRate(new SimpleQuote(0.01));
    ext::shared_ptr<YieldTermStructure> forTS(new 
                FlatForward(today, Handle<Quote>(forRate), dc));
    ext::shared_ptr<YieldTermStructure> domTS(new 
                FlatForward(today, Handle<Quote>(domRate), dc));

    // option maturities
    std::vector<Period> optionTenors;
    optionTenors.push_back(Period(6, Months));
    optionTenors.push_back(Period(1, Years));
    optionTenors.push_back(Period(2, Years));

    // vol quotes
    DeltaVolData deltaVols[] = {
        // vol,   delta, atm type,            delta type  
        // 6M    
        {0.0764,  -0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0704,  -0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0663,   0.00, DeltaVolQuote::Spot, DeltaVolQuote::AtmDeltaNeutral},
        {0.0663,   0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0691,   0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        // 1Y
        {0.0831,  -0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0748,  -0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0695,   0.00, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmDeltaNeutral},
        {0.0695,   0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0736,   0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        // 2Y
        {0.0890,  -0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0798,  -0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0742,   0.00, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmDeltaNeutral},
        {0.0746,   0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0796,   0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        }
    };

    // delta vol matrix
    Size numRows = 3;
    Size numCols = 5;
    ext::shared_ptr<DeltaVolQuote> dvq;
    std::vector<std::vector<Handle<DeltaVolQuote> > > 
            deltaVolMatrix(numRows, std::vector<Handle<DeltaVolQuote> >(numCols));
    for (Size i=0; i<numRows; i++) {
        for (Size j=0; j<numCols; j++) {
            Size k = i*numCols + j; 
            ext::shared_ptr<Quote> vol(new SimpleQuote(deltaVols[k].vol));
            if (deltaVols[k].atmType == DeltaVolQuote::AtmNull) {
                dvq = ext::make_shared<DeltaVolQuote>(
                                            deltaVols[k].delta,
                                            Handle<Quote>(vol), 
                                            0.0, 
                                            deltaVols[k].deltaType);
            } else {
                dvq = ext::make_shared<DeltaVolQuote>(
                                            Handle<Quote>(vol), 
                                            deltaVols[k].deltaType,
                                            0.0, 
                                            deltaVols[k].atmType);
            }
            deltaVolMatrix[i][j] = Handle<DeltaVolQuote>(dvq);
        }
    }

    ext::shared_ptr<BlackVolTermStructure> sviVolSurface(
        ext::make_shared<SviFxBlackVolatilitySurface>(
                                deltaVolMatrix,
                                Handle<Quote>(spotFx),
                                optionTenors,
                                Handle<YieldTermStructure>(domTS),
                                Handle<YieldTermStructure>(forTS),
                                fxFixingDays,
                                advanceCal,
                                adjustCal_)); 
    sviVolSurface->enableExtrapolation(true);

    Real strike = 1.2;
    Date exerciseDate(26, February, 2020);
    Real impliedVol = sviVolSurface->blackVol(exerciseDate, strike);
    std::cout << "Market implied vol: " 
              << impliedVol 
              << std::endl << std::endl;

    //computedVol = sviVolSurface->blackVol(0, 1.1);
    //std::cout << "computed vol: " << computedVol << std::endl;

    Size tSteps = 801;
    Size xSteps = 801;

    ext::shared_ptr<LocalVolSurface> localVolSurface(
		ext::make_shared<LocalVolSurface>(Handle<BlackVolTermStructure>(sviVolSurface), 
                                          Handle<YieldTermStructure>(domTS),
                                          Handle<YieldTermStructure>(forTS), 
                                          Handle<Quote>(spotFx)));

    //ext::shared_ptr<LocalVolTermStructure> fixLocalVolTS(
    //    ext::make_shared<FixedLocalVolSurfaceAdapter>(localVolSurface, tSteps, xSteps));

    ext::shared_ptr<GeneralizedBlackScholesProcess> bsProcess(
        ext::make_shared<GeneralizedBlackScholesProcess>(
                                Handle<Quote>(spotFx), 
                                Handle<YieldTermStructure>(forTS), 
                                Handle<YieldTermStructure>(domTS),
                                Handle<BlackVolTermStructure>(sviVolSurface)));
                                //Handle<LocalVolTermStructure>(localVolSurface)));

    ext::shared_ptr<BlackVolTermStructure> flatVol(
        ext::make_shared<BlackConstantVol>(
                                0, NullCalendar(), impliedVol, dc));

    ext::shared_ptr<GeneralizedBlackScholesProcess> bsProcessFlatVol(
        ext::make_shared<GeneralizedBlackScholesProcess>(
                                Handle<Quote>(spotFx), 
                                Handle<YieldTermStructure>(forTS), 
                                Handle<YieldTermStructure>(domTS),
                                Handle<BlackVolTermStructure>(flatVol)));

    ext::shared_ptr<Exercise> exercise(
		ext::make_shared<EuropeanExercise>(exerciseDate));
    ext::shared_ptr<StrikedTypePayoff> payoff(
		ext::make_shared<PlainVanillaPayoff>(
                    spotFx->value() < strike ? Option::Call : Option::Put,
                    strike));
     VanillaOption option(payoff, exercise);

    ext::shared_ptr<PricingEngine> volCurveEngine(
		ext::make_shared<FdBlackScholesVanillaEngine>(bsProcess, 
                                                      tSteps, xSteps, 0,
                                                      FdmSchemeDesc::Douglas(), 
                                                      false));

    ext::shared_ptr<PricingEngine> localVolEngine(
		ext::make_shared<FdBlackScholesVanillaEngine>(bsProcess, 
                                                      tSteps, xSteps, 0,
                                                      FdmSchemeDesc::Douglas(), 
                                                      true));

    ext::shared_ptr<PricingEngine> flatVolEngine(
		ext::make_shared<FdBlackScholesVanillaEngine>(bsProcessFlatVol, 
                                                      tSteps, xSteps, 0,
                                                      FdmSchemeDesc::Douglas(), 
                                                      false));

    option.setPricingEngine(volCurveEngine);
    std::cout << "NPV (term-structure): " 
              << option.NPV()
              << std::endl;
    std::cout << "Implied vol (term-structure): " 
              << option.impliedVolatility(option.NPV(), bsProcess, 1e-6, 1000) 
              << std::endl << std::endl;
    
    option.setPricingEngine(localVolEngine);
    std::cout << "NPV (local vol): " 
              << option.NPV()
              << std::endl;
    std::cout << "Implied vol (local): " 
              << option.impliedVolatility(option.NPV(), bsProcess, 1e-6, 1000) 
              << std::endl << std::endl;

    option.setPricingEngine(flatVolEngine);
    std::cout << "NPV (flat): " 
              << option.NPV()
              << std::endl;
    std::cout << "Implied vol (flat): " 
              << option.impliedVolatility(option.NPV(), bsProcessFlatVol, 1e-6, 1000) 
              << std::endl << std::endl;

    return 0;
}