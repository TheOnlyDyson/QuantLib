/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 StatPro Italia srl
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

/*! \file iborindex.hpp
    \brief base class for Inter-Bank-Offered-Rate indexes
*/

#ifndef quantlib_ibor_index_hpp
#define quantlib_ibor_index_hpp

#include <ql/indexes/interestrateindex.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>

namespace QuantLib {

    //! base class for Inter-Bank-Offered-Rate indexes (e.g. %Libor, etc.)
    class IborIndex : public InterestRateIndex {
      public:
        IborIndex(const std::string& familyName,
                  const Period& tenor,
                  Natural settlementDays,
                  const Currency& currency,
                  const Calendar& fixingCalendar,
                  BusinessDayConvention convention,
                  bool endOfMonth,
                  const DayCounter& dayCounter,
                  const Handle<YieldTermStructure>& h =
                                    Handle<YieldTermStructure>(),			      
			      const Handle<YieldTermStructure>& h2 =
			                        Handle<YieldTermStructure>(),
			      const Date& cessationDate = Date(),
			      const Spread fallback_spread = QL_NULL_INTEGER,
			      const Natural obs_period_shift = 2,
			      const Calendar& fallbackCalendar = Calendar()); 
		// const boost::shared_ptr<IborIndex>& fallbackIndex = boost::shared_ptr<IborIndex>() maybe better ....
        //! \name InterestRateIndex interface
        //@{
        Date maturityDate(const Date& valueDate) const;
		Date maturityDateFallback(const Date& valueDate) const override;
		Rate forecastFixing(const Date& fixingDate, const Date& fixing_payDate = Date()) const;
        // @}
        //! \name Inspectors
        //@{
        BusinessDayConvention businessDayConvention() const;
        bool endOfMonth() const { return endOfMonth_; }
		Date cessationDate() const { return cessationDate_; }
		Spread fallbackSpread() const { return fallback_spread_; }
		Natural obsPeriodShift() const { return obs_period_shift_; }
        //! the curve used to forecast fixings
        Handle<YieldTermStructure> forwardingTermStructure() const;
		Handle<YieldTermStructure> forwardingFallbackTermStructure() const;

		//IborIndex withFallbackYTS(const Handle<YieldTermStructure>& fallbackYTS) { fallbackTermStructure_ = fallbackYTS; return *this; };
		//IborIndex withCessationDate(const Date& cessationDate) { cessationDate_ = cessationDate; return *this; };
		//IborIndex withFallbackSpread(const Spread fallback_spread) { fallback_spread_ = fallback_spread; return *this; };

		void setFallbackYTS(const Handle<YieldTermStructure>& fallbackYTS) { fallbackTermStructure_ = fallbackYTS; };
		void setCessationDate(const Date& cessationDate) { cessationDate_ = cessationDate; };
		void setFallbackSpread(const Spread fallback_spread) { fallback_spread_ = fallback_spread; };
        //@}
        //! \name Other methods
        //@{
        //! returns a copy of itself linked to a different forwarding curve
        virtual boost::shared_ptr<IborIndex> clone(
                        const Handle<YieldTermStructure>& forwarding, 
			            const Handle<YieldTermStructure>& fallback = Handle<YieldTermStructure>()) const;
        // @}
      protected:
        BusinessDayConvention convention_;
        Handle<YieldTermStructure> termStructure_;
		Handle<YieldTermStructure> fallbackTermStructure_;
		Date cessationDate_;
		Spread fallback_spread_;
		Natural obs_period_shift_;
        bool endOfMonth_;
      private:
        // overload to avoid date/time (re)calculation
        /* This can be called with cached coupon dates (and it does
           give quite a performance boost to coupon calculations) but
           is potentially misleading: by passing the wrong dates, one
           can ask a 6-months index for a 1-year fixing.

           For that reason, we're leaving this method private and
           we're declaring the IborCoupon class (which uses it) as a
           friend.  Should the need arise, we might promote it to
           public, but before doing that I'd think hard whether we
           have any other way to get the same results.
        */
        /*
		Rate forecastFixing(const Date& valueDate,
                            const Date& endDate,
                            Time t) const;
							*/
		Rate forecastFixing(const Date& valueDate,
			                const Date& endDate,
			                Time t,
			                bool has_fallback_triggered = false) const;
        friend class IborCoupon;
    };


    class OvernightIndex : public IborIndex {
      public:
        OvernightIndex(const std::string& familyName,
                       Natural settlementDays,
                       const Currency& currency,
                       const Calendar& fixingCalendar,
                       const DayCounter& dayCounter,
                       const Handle<YieldTermStructure>& h =
                                    Handle<YieldTermStructure>());
        //! returns a copy of itself linked to a different forwarding curve
        boost::shared_ptr<IborIndex> clone(
                                   const Handle<YieldTermStructure>& h) const;
    };


    // inline

    inline BusinessDayConvention IborIndex::businessDayConvention() const {
        return convention_;
    }

    inline Handle<YieldTermStructure>
    IborIndex::forwardingTermStructure() const {
        return termStructure_;
    }

	inline Handle<YieldTermStructure>
		IborIndex::forwardingFallbackTermStructure() const {
		return fallbackTermStructure_;
	}

	inline Rate IborIndex::forecastFixing(const Date& d1,
		                                  const Date& d2,
		                                  Time t,
		                                  bool has_fallback_triggered) const {
		QL_REQUIRE(!termStructure_.empty(),
			"null term structure set to this instance of " << name());

		if (!has_fallback_triggered) {
			DiscountFactor disc1 = termStructure_->discount(d1);
			DiscountFactor disc2 = termStructure_->discount(d2);
			return (disc1 / disc2 - 1.0) / t;
		}
		else {
			QL_REQUIRE(!fallbackTermStructure_.empty(),
				"null fallback term structure set to this instance of " << name());
			DiscountFactor disc1 = fallbackTermStructure_->discount(d1);
			DiscountFactor disc2 = fallbackTermStructure_->discount(d2);
			return (disc1 / disc2 - 1.0) / t + fallback_spread_; 
		}
	}


}

#endif
