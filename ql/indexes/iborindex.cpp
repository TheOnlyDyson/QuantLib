/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 StatPro Italia srl
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

#include <ql/indexes/iborindex.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>

namespace QuantLib {

    IborIndex::IborIndex(const std::string& familyName,
                         const Period& tenor,
                         Natural settlementDays,
                         const Currency& currency,
                         const Calendar& fixingCalendar,
                         BusinessDayConvention convention,
                         bool endOfMonth,
                         const DayCounter& dayCounter,
                         const Handle<YieldTermStructure>& h,		                
		                 const Handle<YieldTermStructure>& h2,
		                 const Date& cessationDate,
		                 const Spread fallback_spread,
		                 const Natural obs_period_shift,
		                 const Calendar& fallbackCalendar)
    : InterestRateIndex(familyName, tenor, settlementDays, currency,
                        fixingCalendar, dayCounter, fallbackCalendar),
      convention_(convention), termStructure_(h), endOfMonth_(endOfMonth), 
	  fallbackTermStructure_(h2), cessationDate_(cessationDate), fallback_spread_(fallback_spread), 
	  obs_period_shift_(obs_period_shift) {
        registerWith(termStructure_);
		registerWith(fallbackTermStructure_);
      }

    Rate IborIndex::forecastFixing(const Date& fixingDate, const Date& fixing_payDate) const {
        Date d1 = valueDate(fixingDate);
		Date d2 = maturityDate(d1);

		if (cessationDate_ == Date() || fallbackTermStructure_.empty() || fixingDate < cessationDate_) {
			Time t = dayCounter_.yearFraction(d1, d2);
			QL_REQUIRE(t > 0.0,
				"\n cannot calculate forward rate between " <<
				d1 << " and " << d2 <<
				":\n non positive time (" << t <<
				") using " << dayCounter_.name() << " daycounter");
			return forecastFixing(d1, d2, t, false);
		}
		else {
			Date d_libor_fix = fixingDate;
			Date fallback_obs_day; // ISDA Suppl 70

			d1 = valueDateFallback(d_libor_fix, obs_period_shift_); // BBG Rulebook Accrual Start Date
			d2 = maturityDateFallback(d1); // BBG Rulebook Accrual End Date
				
			if (fixing_payDate == Date()) // fallback has to be observed 2 days before original Libor coupon pay date, otherwise roll forward
				fallback_obs_day = d2;
			else
				fallback_obs_day = fixingCalendar().advance(fixing_payDate, -static_cast<Integer>(obs_period_shift_), Days); // ISDA Suppl 70

			while (d2 > fallback_obs_day)
			{
				d_libor_fix = fixingCalendar().advance(d_libor_fix, -1, Days);
				d1 = valueDateFallback(d_libor_fix, obs_period_shift_);
				d2 = maturityDateFallback(d1);
			}
			
			Time t = dayCounter_.yearFraction(d1, d2); // right dcc? Probably yes for Actual.x dcc - should be from the RfR but converted to IBOR

			QL_REQUIRE(t > 0.0,
				"\n cannot calculate forward rate between " <<
				d1 << " and " << d2 <<
				":\n non positive time (" << t <<
				") using " << dayCounter_.name() << " daycounter");

			return forecastFixing(d1, d2, t, true);
		}
    }

    Date IborIndex::maturityDate(const Date& valueDate) const {
        return fixingCalendar().advance(valueDate,
                                        tenor_,
                                        convention_,
                                        endOfMonth_);
    }

	Date IborIndex::maturityDateFallback(const Date& valueDate) const {
		// Fallbacks do not follow EOM according to BBG Rule Book https://data.bloomberglp.com/professional/sites/10/IBOR-Fallback-Rate-Adjustments-Rule-Book.pdf
		// If it is in line with ISDA standards to use EOM for Libor, is debatable in any case https://www.isda.org/2012/03/06/linear-interpolation-example/
		return fallbackCalendar().advance(valueDate, tenor_, convention_, false);
	}

    boost::shared_ptr<IborIndex> IborIndex::clone(
                               const Handle<YieldTermStructure>& h,
		                       const Handle<YieldTermStructure>& h2) const {
        return boost::shared_ptr<IborIndex>(
                                        new IborIndex(familyName(),
                                                      tenor(),
                                                      fixingDays(),
                                                      currency(),
                                                      fixingCalendar(),
                                                      businessDayConvention(),
                                                      endOfMonth(),
                                                      dayCounter(),
                                                      h,
											          h2.empty() ? forwardingFallbackTermStructure() : h2,
											          cessationDate(),
											          fallbackSpread(),
											          obsPeriodShift(),
											          fallbackCalendar()
										)
			);
    }


    OvernightIndex::OvernightIndex(const std::string& familyName,
                                   Natural settlementDays,
                                   const Currency& curr,
                                   const Calendar& fixCal,
                                   const DayCounter& dc,
                                   const Handle<YieldTermStructure>& h)
   : IborIndex(familyName, 1*Days, settlementDays, curr,
               fixCal, Following, false, dc, h) {}

    boost::shared_ptr<IborIndex> OvernightIndex::clone(
                               const Handle<YieldTermStructure>& h) const {
        return boost::shared_ptr<IborIndex>(
                                        new OvernightIndex(familyName(),
                                                           fixingDays(),
                                                           currency(),
                                                           fixingCalendar(),
                                                           dayCounter(),
                                                           h));
    }

}
