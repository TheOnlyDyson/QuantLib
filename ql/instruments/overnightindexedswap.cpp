/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2009 Roland Lichters
 Copyright (C) 2009 Ferdinando Ametrano

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

#include <ql/instruments/overnightindexedswap.hpp>
#include <ql/cashflows/overnightindexedcoupon.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>

namespace QuantLib {

    OvernightIndexedSwap::OvernightIndexedSwap(
                    Type type,
                    Real nominal,
                    const Schedule& schedule,
                    Rate fixedRate,
                    const DayCounter& fixedDC,
                    const boost::shared_ptr<OvernightIndex>& overnightIndex,
                    Spread spread)
    : Swap(2), type_(type),
      nominals_(std::vector<Real>(1, nominal)),
      paymentFrequency_(schedule.tenor().frequency()),
      fixedRate_(fixedRate), fixedDC_(fixedDC),
      overnightIndex_(overnightIndex), spread_(spread), 
	   /*AFR*/ indexCurrency_(overnightIndex->currency()), swapPaymentLag_(0), hasCustomLag_(false) {

		  initialize(schedule);

    }

    OvernightIndexedSwap::OvernightIndexedSwap(
                    Type type,
                    std::vector<Real> nominals,
                    const Schedule& schedule,
                    Rate fixedRate,
                    const DayCounter& fixedDC,
                    const boost::shared_ptr<OvernightIndex>& overnightIndex,
                    Spread spread)
    : Swap(2), type_(type), nominals_(nominals),
      paymentFrequency_(schedule.tenor().frequency()),
      fixedRate_(fixedRate), fixedDC_(fixedDC),
      overnightIndex_(overnightIndex), spread_(spread),
	  /*AFR*/ indexCurrency_(overnightIndex->currency()), swapPaymentLag_(0), hasCustomLag_(false)  {

		  initialize(schedule);

    }

	/* +AFR */
	OvernightIndexedSwap::OvernightIndexedSwap(
					Type type,
					Real nominal,
					const Schedule& schedule,
					Rate fixedRate,
					const DayCounter& fixedDC,
					const boost::shared_ptr<OvernightIndex>& overnightIndex,
					int swapPaymentLag,
					Spread spread)
		: Swap(2), type_(type),
		nominals_(std::vector<Real>(1, nominal)),
		paymentFrequency_(schedule.tenor().frequency()),
		fixedRate_(fixedRate), fixedDC_(fixedDC),
		overnightIndex_(overnightIndex), swapPaymentLag_(swapPaymentLag), /*AFR*/
		spread_(spread), indexCurrency_(overnightIndex->currency()), hasCustomLag_(true) /*AFR*/ {

		initialize(schedule);

	}

	OvernightIndexedSwap::OvernightIndexedSwap(
					Type type,
					std::vector<Real> nominals,
					const Schedule& schedule,
					Rate fixedRate,
					const DayCounter& fixedDC,
					const boost::shared_ptr<OvernightIndex>& overnightIndex,
					int swapPaymentLag,
					Spread spread)
		: Swap(2), type_(type),
		nominals_(nominals),
		paymentFrequency_(schedule.tenor().frequency()),
		fixedRate_(fixedRate), fixedDC_(fixedDC),
		overnightIndex_(overnightIndex), swapPaymentLag_(swapPaymentLag), /*AFR*/
		spread_(spread), indexCurrency_(overnightIndex->currency()), hasCustomLag_(true) /*AFR*/ {

		initialize(schedule);

	}
	/* -AFR */


    void OvernightIndexedSwap::initialize(const Schedule& schedule) {
        if (fixedDC_==DayCounter())
            fixedDC_ = overnightIndex_->dayCounter();
		
		/*AFR*/
		if (!hasCustomLag_) 
			switch (indexCurrency_.numericCode())
			{
			case 978: /*EUR*/
				swapPaymentLag_ = 1;
				break;
			case 756: /*CHF*/
				swapPaymentLag_ = 1;
				break;
			case 826: /*GBP*/
				swapPaymentLag_ = 0;
				break;
			case 840: /*USD*/
				swapPaymentLag_ = 2;
				break;
			case 392: /*JPY*/
				swapPaymentLag_ = 0;
				break;
			default: 
				swapPaymentLag_ = 0;
			}		
		/*AFR*/
		//swapPaymentLag_ = 4;

        legs_[0] = FixedRateLeg(schedule)
            .withNotionals(nominals_)
            .withCouponRates(fixedRate_, fixedDC_)
			.withPaymentLag(swapPaymentLag_); /*AFR*/

        legs_[1] = OvernightLeg(schedule, overnightIndex_)
            .withNotionals(nominals_)
            .withSpreads(spread_)
			.withPaymentLag(swapPaymentLag_); /*AFR*/

        for (Size j=0; j<2; ++j) {
            for (Leg::iterator i = legs_[j].begin(); i!= legs_[j].end(); ++i)
                registerWith(*i);
        }

        switch (type_) {
          case Payer:
            payer_[0] = -1.0;
            payer_[1] = +1.0;
            break;
          case Receiver:
            payer_[0] = +1.0;
            payer_[1] = -1.0;
            break;
          default:
            QL_FAIL("Unknown overnight-swap type");
        }
    }

    Real OvernightIndexedSwap::fairRate() const {
        static Spread basisPoint = 1.0e-4;
        calculate();
        return fixedRate_ - NPV_/(fixedLegBPS()/basisPoint);
    }

    Spread OvernightIndexedSwap::fairSpread() const {
        static Spread basisPoint = 1.0e-4;
        calculate();
        return spread_ - NPV_/(overnightLegBPS()/basisPoint);
    }

    Real OvernightIndexedSwap::fixedLegBPS() const {
        calculate();
        QL_REQUIRE(legBPS_[0] != Null<Real>(), "result not available");
        return legBPS_[0];
    }

    Real OvernightIndexedSwap::overnightLegBPS() const {
        calculate();
        QL_REQUIRE(legBPS_[1] != Null<Real>(), "result not available");
        return legBPS_[1];
    }

    Real OvernightIndexedSwap::fixedLegNPV() const {
        calculate();
        QL_REQUIRE(legNPV_[0] != Null<Real>(), "result not available");
        return legNPV_[0];
    }

    Real OvernightIndexedSwap::overnightLegNPV() const {
        calculate();
        QL_REQUIRE(legNPV_[1] != Null<Real>(), "result not available");
        return legNPV_[1];
    }

}
