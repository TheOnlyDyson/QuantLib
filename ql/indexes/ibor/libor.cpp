/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007 Ferdinando Ametrano
 Copyright (C) 2007 Chiara Fornarola
 Copyright (C) 2005, 2006, 2008 StatPro Italia srl

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

#include <ql/indexes/ibor/libor.hpp>
#include <ql/time/calendars/jointcalendar.hpp>
#include <ql/time/calendars/unitedkingdom.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/calendars/switzerland.hpp>
#include <ql/time/calendars/japan.hpp>
#include <ql/currencies/europe.hpp>

namespace QuantLib {

    namespace {

        BusinessDayConvention liborConvention(const Period& p) {
            switch (p.units()) {
              case Days:
              case Weeks:
                return Following;
              case Months:
              case Years:
                return ModifiedFollowing;
              default:
                QL_FAIL("invalid time units");
            }
        }

        bool liborEOM(const Period& p) {
            switch (p.units()) {
              case Days:
              case Weeks:
                return false;
              case Months:
              case Years:
                return true;
              default:
                QL_FAIL("invalid time units");
            }
        }

		Spread liborFallbackSpread(const Currency& ccy, const Period& p) {
			if (ccy.code() == "USD")
				switch (p.frequency()) {
				case Monthly:
					return 0.11448 / 100.0;
				case Quarterly:
					return 0.26161 / 100.0;
				case Semiannual:
					return 0.42826 / 100.0;
				case Annual:
					return 0.71513 / 100.0;
				default:
					QL_FAIL("invalid time units");
				}
			else if (ccy.code() == "GBP")
				switch (p.frequency()) {
				case Monthly:
					return 0.0326 / 100.0;
				case Quarterly:
					return 0.1193 / 100.0;
				case Semiannual:
					return 0.2766 / 100.0;
				case Annual:
					return 0.4644 / 100.0;
				default:
					QL_FAIL("invalid time units");
				}
			else if (ccy.code() == "CHF")
				switch (p.frequency()) {
				case Monthly:
					return -0.0571 / 100.0;
				case Quarterly:
					return 0.0031 / 100.0;
				case Semiannual:
					return 0.0741 / 100.0;
				case Annual:
					return 0.2048 / 100.0;
				default:
					QL_FAIL("invalid time units");
				}
			else if (ccy.code() == "JPY")
				switch (p.frequency()) {
				case Monthly:
					return -0.02923 / 100.0;
				case Quarterly:
					return 0.00835 / 100.0;
				case Semiannual:
					return 0.05809 / 100.0;
				case Annual:
					return 0.16600 / 100.0;
				default:
					QL_FAIL("invalid time units");
				}
			else
				return 0.0;
		}

		Date liborCessationDate(const Currency& ccy, const Period& p) {
			if (ccy.code() == "CHF" || ccy.code() == "GBP" || ccy.code() == "JPY")
				return Date(31, December, 2021); 
			else if (ccy.code() == "USD")
				switch (p.frequency()) {
				 case Monthly:
				 case Quarterly:
				 case Semiannual:
					return Date(30, June, 2023);
				 default:
					return Date(31, December, 2021);
			}
			else
				return Date();
		}
		
		Calendar liborFallbackCalendar(const Currency& ccy) {
			if (ccy.code() == "USD")
				return UnitedStates(UnitedStates::GovernmentBond);
			else if (ccy.code() == "GBP")
				return UnitedKingdom(UnitedKingdom::Exchange);
			else if (ccy.code() == "CHF")
				return Switzerland(); // 
			else if (ccy.code() == "JPY")
				return Japan();
			else
				return Calendar();
		}

    }


    Libor::Libor(const std::string& familyName,
                 const Period& tenor,
                 Natural settlementDays,
                 const Currency& currency,
                 const Calendar& financialCenterCalendar,
                 const DayCounter& dayCounter,
                 const Handle<YieldTermStructure>& h,
				 const Handle<YieldTermStructure>& h2,
		         const Date& cessationDate,
				 const Spread fallback_spread,
				 const Natural obs_period_shift,
		         const Calendar& fallbackCalendar)
    : IborIndex(familyName, tenor, settlementDays, currency,
                // http://www.bba.org.uk/bba/jsp/polopoly.jsp?d=225&a=1412 :
                // UnitedKingdom::Exchange is the fixing calendar for
                // a) all currencies but EUR
                // b) all indexes but o/n and s/n
                UnitedKingdom(UnitedKingdom::Exchange),
                liborConvention(tenor), liborEOM(tenor),
                dayCounter, h,  h2, 
		        cessationDate==Date() ? liborCessationDate(currency, tenor) : cessationDate, 
		        fallback_spread==QL_NULL_INTEGER ? liborFallbackSpread(currency,tenor) : fallback_spread,
		        obs_period_shift, 
		        fallbackCalendar==Calendar() ? liborFallbackCalendar(currency) : fallbackCalendar),
      financialCenterCalendar_(financialCenterCalendar),
      jointCalendar_(JointCalendar(UnitedKingdom(UnitedKingdom::Exchange),
                                   financialCenterCalendar,
                                   JoinHolidays)) {
        QL_REQUIRE(this->tenor().units()!=Days,
                   "for daily tenors (" << this->tenor() <<
                   ") dedicated DailyTenor constructor must be used");
        QL_REQUIRE(currency!=EURCurrency(),
                   "for EUR Libor dedicated EurLibor constructor must be used");
    }

    Date Libor::valueDate(const Date& fixingDate) const {

        QL_REQUIRE(isValidFixingDate(fixingDate),
                   "Fixing date " << fixingDate << " is not valid");

        // http://www.bba.org.uk/bba/jsp/polopoly.jsp?d=225&a=1412 :
        // For all currencies other than EUR and GBP the period between
        // Fixing Date and Value Date will be two London business days
        // after the Fixing Date, or if that day is not both a London
        // business day and a business day in the principal financial centre
        // of the currency concerned, the next following day which is a
        // business day in both centres shall be the Value Date.
        Date d = fixingCalendar().advance(fixingDate, fixingDays_, Days);
        return jointCalendar_.adjust(d);
    }

    Date Libor::maturityDate(const Date& valueDate) const {
        // Where a deposit is made on the final business day of a
        // particular calendar month, the maturity of the deposit shall
        // be on the final business day of the month in which it matures
        // (not the corresponding date in the month of maturity). Or in
        // other words, in line with market convention, BBA LIBOR rates
        // are dealt on an end-end basis. For instance a one month
        // deposit for value 28th February would mature on 31st March,
        // not the 28th of March.
        return jointCalendar_.advance(valueDate, tenor_, convention_,
                                                         endOfMonth());
    }

    Calendar Libor::jointCalendar() const {
        return jointCalendar_;
    }

    boost::shared_ptr<IborIndex> Libor::clone(
                                  const Handle<YieldTermStructure>& h, const Handle<YieldTermStructure>& h2) const {
        return boost::shared_ptr<IborIndex>(new Libor(familyName(),
                                                      tenor(),
                                                      fixingDays(),
                                                      currency(),
                                                      financialCenterCalendar_,
                                                      dayCounter(),
                                                      h,
		                                              h2.empty() ? forwardingFallbackTermStructure() : h2,
													  cessationDate(),
			                                          fallbackSpread(),
			                                          obsPeriodShift(),
			                                          fallbackCalendar()));
    }


    DailyTenorLibor::DailyTenorLibor(
                 const std::string& familyName,
                 Natural settlementDays,
                 const Currency& currency,
                 const Calendar& financialCenterCalendar,
                 const DayCounter& dayCounter,
                 const Handle<YieldTermStructure>& h)
    : IborIndex(familyName, 1*Days, settlementDays, currency,
                // http://www.bba.org.uk/bba/jsp/polopoly.jsp?d=225&a=1412 :
                // no o/n or s/n fixings (as the case may be) will take place
                // when the principal centre of the currency concerned is
                // closed but London is open on the fixing day.
                JointCalendar(UnitedKingdom(UnitedKingdom::Exchange),
                              financialCenterCalendar,
                              JoinHolidays),
                liborConvention(1*Days), liborEOM(1*Days),
                dayCounter, h) {
        QL_REQUIRE(currency!=EURCurrency(),
                   "for EUR Libor dedicated EurLibor constructor must be used");
    }

}
