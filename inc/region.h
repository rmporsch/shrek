#ifndef REGION_H
#define REGION_H

#include <string>
#include <vector>
#include <assert.h>
#include <stdio.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include "interval.h"
#include "usefulTools.h"

// Each region should only contain the information of their own intervals
// and the corresponding variance
// A rather embarrassing class... It only do half the work here (e.g. useless when class not provided)
class Region
{
    public:
        /** Default constructor */
        Region(const std::string name);
        Region(const std::string name, const std::string bedFileName);
        /** Default destructor */
        size_t getIntervalSize() const{return m_intervalList.size(); };
        std::string getChr(size_t i) const {return m_intervalList.at(i).getChr();};
        size_t getStart(size_t i) const {return m_intervalList.at(i).getStart();};
        size_t getEnd(size_t i) const {return m_intervalList.at(i).getEnd();};
        double getVariance() const{return m_variance;};
        double getVariance(double i) const{return i*m_variance+i*i*m_addVar;};
        std::string getName() const{return m_name;};
        void addVariance(double i){m_variance+=i;};
        void addVariance(double i, double j){m_variance+=i; m_addVar+=j;};
        virtual ~Region();
        // The aim of this function is to generate the required region list, each region item
        // should contain its corresponding intervals
        static void generateRegionList(boost::ptr_vector<Region> &regionList, const std::string regionInput);
    protected:
    private:
        boost::ptr_vector<Interval> m_intervalList;
        std::string m_name="";
        double m_variance=0.0;
        double m_addVar=0.0;
        //double m_varianceBuff=0.0;
};

#endif // REGION_H
