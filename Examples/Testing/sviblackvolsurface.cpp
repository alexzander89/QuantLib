

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
#include <ql/models/equity/hestonmodel.hpp>
#include <ql/termstructures/volatility/equityfx/localvolsurface.hpp>
#include <ql/pricingengines/vanilla/fdblackscholesvanillaengine.hpp>
#include <ql/termstructures/volatility/equityfx/fixedlocalvoladapter.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/exercise.hpp>
#include <ql/experimental/models/hestonslvfdmmodel.hpp>
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

    DayCounter dc = Actual365Fixed();
    Date today(2, May, 2019);
    Settings::instance().evaluationDate() = today;
    Natural fxFixingDays = 2;
    Calendar advanceCal = TARGET();
    Calendar adjustCal_ = UnitedStates();

    // term structures
    ext::shared_ptr<SimpleQuote> spotFx(new SimpleQuote(1.1172));
    ext::shared_ptr<SimpleQuote> forRate(new SimpleQuote(-0.01));
    ext::shared_ptr<SimpleQuote> domRate(new SimpleQuote(0.02));
    ext::shared_ptr<YieldTermStructure> forTS(new 
                FlatForward(today, Handle<Quote>(forRate), dc));
    ext::shared_ptr<YieldTermStructure> domTS(new 
                FlatForward(today, Handle<Quote>(domRate), dc));

    // option maturities
    std::vector<Period> optionTenors;
    optionTenors.push_back(Period(1, Months));
    optionTenors.push_back(Period(2, Months));
    optionTenors.push_back(Period(3, Months));
    optionTenors.push_back(Period(6, Months));
    optionTenors.push_back(Period(9, Months));
    optionTenors.push_back(Period(1, Years));

    // vol quotes
    DeltaVolData deltaVols[] = {
        // vol,   delta, atm type,            delta type  
        // 1M    
        {0.0554625,  -0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0514875,  -0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0483000,   0.00, DeltaVolQuote::Spot, DeltaVolQuote::AtmDeltaNeutral},
        {0.0483125,   0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0499875,   0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        // 2M    
        {0.0599625,  -0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0554875,  -0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0522000,   0.00, DeltaVolQuote::Spot, DeltaVolQuote::AtmDeltaNeutral},
        {0.0524125,   0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0544375,   0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        // 3M    
        {0.0627500,  -0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0578750,  -0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0544500,   0.00, DeltaVolQuote::Spot, DeltaVolQuote::AtmDeltaNeutral},
        {0.0548750,   0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0574000,   0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        // 6M    
        {0.0681875,  -0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0620750,  -0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0582000,   0.00, DeltaVolQuote::Spot, DeltaVolQuote::AtmDeltaNeutral},
        {0.0590750,   0.25, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        {0.0628125,   0.10, DeltaVolQuote::Spot, DeltaVolQuote::AtmNull        },
        // 9M
        {0.0716875,  -0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0648500,  -0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0607500,   0.00, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmDeltaNeutral},
        {0.0619000,   0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0663125,   0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        // 1Y
        {0.0744375,  -0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0670750,  -0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0628500,   0.00, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmDeltaNeutral},
        {0.0640750,   0.25, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        },
        {0.0690625,   0.10, DeltaVolQuote::Fwd,  DeltaVolQuote::AtmNull        }
    };

    // delta vol matrix
    Size numRows = 6;
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
        new SviFxBlackVolatilitySurface(
                                deltaVolMatrix,
                                Handle<Quote>(spotFx),
                                optionTenors,
                                Handle<YieldTermStructure>(domTS),
                                Handle<YieldTermStructure>(forTS),
                                fxFixingDays,
                                advanceCal,
                                adjustCal_,
                                WeekendsOnly(),
                                Following,
                                Actual365Fixed(),
                                false)); 
    sviVolSurface->enableExtrapolation(true);

    Real strike = 1.1;
    Date exerciseDate(3, February, 2020);
    Real impliedVol = sviVolSurface->blackVol(exerciseDate, strike);
    std::cout << "Market implied vol: " 
              << impliedVol 
              << std::endl << std::endl;

    ext::shared_ptr<LocalVolSurface> localVolSurface(
		ext::make_shared<LocalVolSurface>(
            Handle<BlackVolTermStructure>(sviVolSurface), 
            Handle<YieldTermStructure>(domTS),
            Handle<YieldTermStructure>(forTS), 
            Handle<Quote>(spotFx)));

    FixedLocalVolSurfaceAdapter fixedLocalVolSurface(
            localVolSurface, 
            1.6, 0.5, 51, 200);

    std::cout << "Local vol: " 
              << fixedLocalVolSurface.localVol(exerciseDate, strike, true) 
              << std::endl << std::endl;
    

    //HestonSLVFokkerPlanckFdmParams fdmParams = {
    //    101, 101, 1000, 25, 3.0, 0, 2,
    //    0.1, 1e-4, 10000,
    //    1e-8, 1e-8, 0.0,
    //    1.0, 1.0, 1.0, 1e-6,
    //    FdmHestonGreensFct::Gaussian,
    //    FdmSquareRootFwdOp::Log,
    //    FdmSchemeDesc::ModifiedCraigSneyd()
    //};

    //Real kappa = 1.0;
    //Real theta = 0.06;
    //Real rho   = -0.75;
    //Real sigma = 0.1;
    //Real v0    = 0.09;

    //Date finalDate(3, February, 2021);

    //ext::shared_ptr<HestonProcess> hestonProcess(
	// 	ext::make_shared<HestonProcess>(Handle<YieldTermStructure>(forTS), 
    //                                    Handle<YieldTermStructure>(domTS),
    //                                    Handle<Quote>(spotFx), 
    //                                    v0, kappa, theta, sigma, rho));

    //Handle<HestonModel> hestonModel(
    //         ext::make_shared<HestonModel>(hestonProcess));

    //HestonSLVFDMModel slvModel(
    //                Handle<LocalVolTermStructure>(localVolSurface), 
    //                hestonModel, finalDate, fdmParams);

    // this includes a calibration of the leverage function!
    //ext::shared_ptr<LocalVolTermStructure> l = slvModel.leverageFunction();

    //ext::shared_ptr<GeneralizedBlackScholesProcess> bsProcess(
    //    ext::make_shared<GeneralizedBlackScholesProcess>(
    //                            Handle<Quote>(spotFx), 
    //                            Handle<YieldTermStructure>(forTS), 
    //                            Handle<YieldTermStructure>(domTS),
    //                            Handle<BlackVolTermStructure>(sviVolSurface)));
    //                            //Handle<LocalVolTermStructure>(localVolSurface)));

    // ext::shared_ptr<BlackVolTermStructure> flatVol(
    //     ext::make_shared<BlackConstantVol>(
    //                             0, NullCalendar(), impliedVol, dc));

    // ext::shared_ptr<GeneralizedBlackScholesProcess> bsProcessFlatVol(
    //     ext::make_shared<GeneralizedBlackScholesProcess>(
    //                             Handle<Quote>(spotFx), 
    //                             Handle<YieldTermStructure>(forTS), 
    //                             Handle<YieldTermStructure>(domTS),
    //                             Handle<BlackVolTermStructure>(flatVol)));

    // ext::shared_ptr<Exercise> exercise(
	// 	ext::make_shared<EuropeanExercise>(exerciseDate));
    // ext::shared_ptr<StrikedTypePayoff> payoff(
	// 	ext::make_shared<PlainVanillaPayoff>(
    //                 spotFx->value() < strike ? Option::Call : Option::Put,
    //                 strike));
    // VanillaOption option(payoff, exercise);

    // Size tSteps = 1001;
    // Size xSteps = 1001;

    // ext::shared_ptr<PricingEngine> volCurveEngine(
	// 	ext::make_shared<FdBlackScholesVanillaEngine>(bsProcess, 
    //                                                   tSteps, xSteps, 0,
    //                                                   FdmSchemeDesc::Douglas(), 
    //                                                   false));

    // ext::shared_ptr<PricingEngine> localVolEngine(
	// 	ext::make_shared<FdBlackScholesVanillaEngine>(bsProcess, 
    //                                                   tSteps, xSteps, 0,
    //                                                   FdmSchemeDesc::Douglas(), 
    //                                                   true));

    // ext::shared_ptr<PricingEngine> flatVolEngine(
	// 	ext::make_shared<FdBlackScholesVanillaEngine>(bsProcessFlatVol, 
    //                                                   tSteps, xSteps, 0,
    //                                                   FdmSchemeDesc::Douglas(), 
    //                                                   false));

    // option.setPricingEngine(volCurveEngine);
    // std::cout << "NPV (term-structure): " 
    //           << option.NPV()
    //           << std::endl;
    // std::cout << "Implied vol (term-structure): " 
    //           << option.impliedVolatility(option.NPV(), bsProcess, 1e-6, 1000) 
    //           << std::endl << std::endl;
    
    // option.setPricingEngine(localVolEngine);
    // std::cout << "NPV (local vol): " 
    //           << option.NPV()
    //           << std::endl;
    // std::cout << "Implied vol (local): " 
    //           << option.impliedVolatility(option.NPV(), bsProcess, 1e-6, 1000) 
    //           << std::endl << std::endl;

    // option.setPricingEngine(flatVolEngine);
    // std::cout << "NPV (flat): " 
    //           << option.NPV()
    //           << std::endl;
    // std::cout << "Implied vol (flat): " 
    //           << option.impliedVolatility(option.NPV(), bsProcessFlatVol, 1e-6, 1000) 
    //           << std::endl << std::endl;

    return 0;
}