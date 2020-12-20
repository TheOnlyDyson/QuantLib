/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007, 2009 StatPro Italia srl
 Copyright (C) 2011 Ferdinando Ametrano

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

/*! \file discountingrelativedateswapengine.hpp
    \brief discounting relative date swap engine
*/

#ifndef quantlib_discounting_relative_date_swap_engine_hpp
#define quantlib_discounting_relative_date_swap_engine_hpp

#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/handle.hpp>

namespace QuantLib {

    class DiscountingRelativeDateSwapEngine : public Swap::engine {
      public:
        DiscountingRelativeDateSwapEngine(
               const Handle<YieldTermStructure>& discountCurve =
                                                 Handle<YieldTermStructure>(),
               boost::optional<bool> includeSettlementDateFlows = boost::none,
               Natural settlementDateOffset = 0, 
               Natural npvDateOffset = 0,
			   const Calendar& offsetCalendar = Calendar());

        void calculate() const;
		Handle<YieldTermStructure> discountCurve() const {
			return discountCurve_;
		}

	private:
		Handle<YieldTermStructure> discountCurve_;
		boost::optional<bool> includeSettlementDateFlows_;
		Date settlementDate_, npvDate_;
        Natural settlementDateOffset_, npvDateOffset_;
		Calendar offsetCalendar_;
    };

}

#endif
