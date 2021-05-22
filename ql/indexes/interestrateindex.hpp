/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009 StatPro Italia srl
 Copyright (C) 2006, 2011 Ferdinando Ametrano

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

/*! \file interestrateindex.hpp
    \brief base class for interest rate indexes
*/

#ifndef quantlib_interestrateindex_hpp
#define quantlib_interestrateindex_hpp

#include <ql/index.hpp>
#include <ql/time/calendar.hpp>
#include <ql/currency.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/period.hpp>

namespace QuantLib {

    //! base class for interest rate indexes
    /*! \todo add methods returning InterestRate */
    class InterestRateIndex : public Index,
                              public Observer {
      public:
        InterestRateIndex(const std::string& familyName,
                          const Period& tenor,
                          Natural settlementDays,
                          const Currency& currency,
                          const Calendar& fixingCalendar,
                          const DayCounter& dayCounter,
			              const Calendar& fallbackCalendar = Calendar());
        //! \name Index interface
        //@{
        std::string name() const;
        Calendar fixingCalendar() const;
		Calendar fallbackCalendar() const;
        bool isValidFixingDate(const Date& fixingDate) const;
		Rate fixing(const Date& fixingDate,
			bool forecastTodaysFixing = false) const;
		Rate fixing2(const Date& fixingDate,
			bool forecastTodaysFixing = false,
			const Date& fallback_payDate = Date()) const; // ** REMOVE **
        //@}
        //! \name Observer interface
        //@{
        void update();
        //@}
        //! \name Inspectors
        //@{
        std::string familyName() const { return familyName_; }
        Period tenor() const { return tenor_; }
        Natural fixingDays() const { return fixingDays_; }
        Date fixingDate(const Date& valueDate) const;
        const Currency& currency() const { return currency_; }
        const DayCounter& dayCounter() const { return dayCounter_; }
        //@}
        /*! \name Date calculations

            These method can be overridden to implement particular
            conventions (e.g. EurLibor)

            @{
        */
        virtual Date valueDate(const Date& fixingDate) const;
        virtual Date maturityDate(const Date& valueDate) const = 0;
		virtual Date valueDateFallback(const Date& fixingDate, const Natural offset = 2) const;
		virtual Date maturityDateFallback(const Date& valueDate) const;
        //@}
        //! \name Fixing calculations
        //@{
        //! It can be overridden to implement particular conventions
        virtual Rate forecastFixing(const Date& fixingDate, const Date& fallback_payDate = Date()) const = 0;
        Rate pastFixing(const Date& fixingDate) const;
        // @}
      protected:
        std::string familyName_;
        Period tenor_;
        Natural fixingDays_;
        Currency currency_;
        DayCounter dayCounter_;
        std::string name_;
      private:
        Calendar fixingCalendar_;
		Calendar fallbackCalendar_;
    };


    // inline definitions

    inline std::string InterestRateIndex::name() const {
        return name_;
    }

    inline Calendar InterestRateIndex::fixingCalendar() const {
        return fixingCalendar_;
    }

	inline Calendar InterestRateIndex::fallbackCalendar() const {
		return fallbackCalendar_;
	}

    inline bool InterestRateIndex::isValidFixingDate(const Date& d) const {
        return fixingCalendar().isBusinessDay(d);
    }

    inline void InterestRateIndex::update() {
        notifyObservers();
    }

    inline Date InterestRateIndex::fixingDate(const Date& valueDate) const {
        Date fixingDate = fixingCalendar().advance(valueDate,
            -static_cast<Integer>(fixingDays_), Days);
        return fixingDate;
    }

    inline Date InterestRateIndex::valueDate(const Date& fixingDate) const {
        QL_REQUIRE(isValidFixingDate(fixingDate),
                   fixingDate << " is not a valid fixing date");
        return fixingCalendar().advance(fixingDate, fixingDays_, Days);
    }

	inline Date InterestRateIndex::valueDateFallback(const Date& fixingDate, const Natural offset) const {
		QL_REQUIRE(isValidFixingDate(fixingDate),
			fixingDate << " is not a valid fixing date"); // not sure if required as per the BBG rulebook but doesn't hurt ...
		return fallbackCalendar().advance( 
			                fallbackCalendar().advance(fixingDate, fixingDays_, Days), // BBG Rulebook "Accrual Spot Date" 
			                -static_cast<Integer>(offset), Days); // BBG Rulebook "Accrual Start Date" 
	}

	inline Date InterestRateIndex::maturityDateFallback(const Date& valueDate) const {
		return maturityDate(valueDate);  // just rely on this as a default
	}

    inline Rate InterestRateIndex::pastFixing(const Date& fixingDate) const {
        QL_REQUIRE(isValidFixingDate(fixingDate),
                   fixingDate << " is not a valid fixing date");
        return timeSeries()[fixingDate];
    }

}

#endif
