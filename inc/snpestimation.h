#ifndef SNPESTIMATION_H
#define SNPESTIMATION_H

#include <deque>
#include <list>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <stdio.h>
#include <fstream>
#include <complex>
#include <iostream>
#include "genotypefilehandler.h"
#include "genotype.h"
#include "region.h"
#include "linkage.h"
#include "decomposition.h"
// I didn't include all the required header yet
// Only include them as I start to use them

class SnpEstimation
{
    public:
        /** Default constructor */
        SnpEstimation(const Command &commander);
        /** Default destructor */
        virtual ~SnpEstimation();
        void estimate(GenotypeFileHandler &genotypeFileHandler, boost::ptr_vector<Snp> &snpList, boost::ptr_vector<Region> &regionList);
        void result(const boost::ptr_vector<Snp> &snpList, const boost::ptr_vector<Region> &regionList);
    protected:
    private:
        int m_thread =1;
        bool m_ldCorrection = false;
        double m_extreme =-1.0;
        double m_prevalence =-1.0;
        bool m_qt=false;
        bool m_bt = false;
        size_t m_blockSize =0;
        std::string m_output="";
        void progress(size_t doneItems, size_t totalSnp, std::string chr);

};

#endif // SNPESTIMATION_H
