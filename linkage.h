// This file is part of SHREK, Snp HeRitability Estimate Kit
//
// Copyright (C) 2014-2015 Sam S.W. Choi <choishingwan@gmail.com>
//
// This Source Code Form is subject to the terms of the GNU General
// Public License v. 2.0. If a copy of the GPL was not distributed
// with this file, You can obtain one at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html.
#ifndef LINKAGE_H
#define LINKAGE_H

class LinkageThread;
#include <algorithm>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <deque>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <limits>
#include <map>
#include <math.h>
#include <mutex>
#include <thread>
#include "genotype.h"
#include "snp.h"

/** \class Linkage
 *  \brief Responsible for the LD matrix
 *
 *  This class is responsible for anything related to the LD matrix. Most importantly,
 *  it is responsible for the construction of LD matrix and the core decomposition step
 *  of the matrix equation.
 *  The most complicated part of this class is the part to deal with perfect LD. We take
 *  additional windows so that we make sure that for any window that were processing,
 *  their perfect LD partners are taken into account. This is based on the assumption that
 *  there will be no true perfect LD between two Snps if they are 1 window size away from
 *  each others.
 */
class Linkage
{
	public:
		Linkage(size_t thread);
		virtual ~Linkage();
        void Initialize(const boost::ptr_deque<Genotype> &genotype, const size_t &prevResiduals);
        void Construct(const boost::ptr_deque<Genotype> &genotype, const size_t &genotypeIndex, const size_t& remainedLD, const boost::ptr_vector<Interval> &blockSize, const bool correction, const std::deque<size_t> &ldLoc);
	    void print();
	    void solve(const size_t loc, const size_t length, const Eigen::MatrixXd &betaEstimate, Eigen::MatrixXd &heritability, Eigen::MatrixXd &effectiveNumber, Eigen::VectorXd &ldScore) const;
        void solve(const size_t loc, const size_t length, const Eigen::MatrixXd &betaEstimate, const Eigen::MatrixXd &sqrtChi, Eigen::MatrixXd &heritability, Eigen::MatrixXd &variance, Eigen::VectorXd &ldScore,const Eigen::MatrixXd &sampleSize) const;
	    inline size_t size()const {return m_linkage.rows(); };
	protected:
	private:
	    void buildLd(const bool correction, const size_t vStart, const size_t vEnd, const size_t hEnd, const boost::ptr_deque<Genotype> &genotype, const std::deque<size_t> &ldLoc);
        Eigen::MatrixXd m_linkage;
        Eigen::MatrixXd m_linkageSqrt;
        size_t m_thread=1;
        static std::mutex mtx;

};

#endif // LINKAGE_H
