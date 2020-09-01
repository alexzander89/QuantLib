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

#include <ql/termstructures/volatility/equityfx/fixedlocalvoladapter.hpp>
#include <ql/methods/finitedifferences/utilities/localvolrndcalculator.hpp>
#include <ql/timegrid.hpp>

namespace QuantLib {

    FixedLocalVolSurfaceAdapter::FixedLocalVolSurfaceAdapter(
        const Handle<LocalVolSurface>& localVol,
        Real xMax,
        Real xMin,
        Size tGrid, 
        Size xGrid) 
    : LocalVolTermStructure(localVol->businessDayConvention(),
      localVol->dayCounter()) {

        // we use a time grid and mesher consistent with the one used
        // within the LocalVolRNDCalculator
        ext::shared_ptr<LocalVolRNDCalculator> localVolRND(
            ext::make_shared<LocalVolRNDCalculator>(
                        localVol->underlying().currentLink(),
                        localVol->riskFreeYield().currentLink(),
                        localVol->dividendYield().currentLink(),
                        localVol.currentLink(), tGrid, xGrid));
        
        ext::shared_ptr<TimeGrid> timeGrid = localVolRND->timeGrid();
        std::vector<Time> expiries(timeGrid->begin()+1, timeGrid->end());

        // generate strike matrix
        //std::vector<ext::shared_ptr<std::vector<Real> > > strikes;
        //for (Size i=1; i < timeGrid->size(); ++i) {
        //    Time t = timeGrid->at(i);
        //    ext::shared_ptr<Fdm1dMesher> fdm1dMesher = localVolRND->mesher(t);
        //    const std::vector<Real>& logStrikes = fdm1dMesher->locations();
        //    ext::shared_ptr<std::vector<Real> > strikeSlice(
        //        ext::make_shared<std::vector<Real> >(logStrikes.size()));
        //    for (Size j=0; j < logStrikes.size(); ++j) {
        //        (*strikeSlice)[j] = std::exp(logStrikes[j]);
        //    }
        //    strikes.push_back(strikeSlice);
        //}

        Real h = (xMax - xMin) / static_cast<Real>(xGrid-1);
        std::vector<Real> strikes(xGrid);
        std::vector<Real>::iterator k;
        Real val;
        for (k = strikes.begin(), val = xMin; k != strikes.end(); ++k, val += h) {
            *k = val;
        }

        // generate matrix of fixed local vol points
        //Size nStrikes = strikes.front()->size();
        //ext::shared_ptr<Matrix> localVolMatrix(
        //    ext::make_shared<Matrix>(nStrikes, timeGrid->size()-1));
        ext::shared_ptr<Matrix> localVolMatrix(
            ext::make_shared<Matrix>(xGrid, timeGrid->size()-1));
        for (Size i=1; i < timeGrid->size(); ++i) {
            Time t = timeGrid->at(i);
            //ext::shared_ptr<std::vector<Real> > strikeSlice = strikes[i-1];
            for (Size j=0; j < xGrid; ++j) {
                //Real s = (*strikeSlice)[j];
                Real s = strikes[j];
                (*localVolMatrix)[j][i-1] = localVol->localVol(t, s, true);
            }
        }
        fixedLocalVol_ =  ext::make_shared<FixedLocalVolSurface>(
                                            localVol->referenceDate(), 
                                            expiries, 
                                            strikes, 
                                            localVolMatrix, 
                                            localVol->dayCounter());
    }

} // namespace QuantLib