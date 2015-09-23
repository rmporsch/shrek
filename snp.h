// This file is part of SHREK, Snp HeRitability Estimate Kit
//
// Copyright (C) 2014-2015 Sam S.W. Choi <choishingwan@gmail.com>
//
// This Source Code Form is subject to the terms of the GNU General
// Public License v. 2.0. If a copy of the GPL was not distributed
// with this file, You can obtain one at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html.
#ifndef SNP_H
#define SNP_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <memory>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include "usefulTools.h"
#include "region.h"
#include "command.h"

/**
 * \class Snp
 * \brief Store Snp information and perform some basic logistic of these information
 */
class Snp
{
public:
        Snp(std::string chr, std::string rs, size_t bp, size_t sampleSize, std::vector<std::string> &original, std::string refAllele, std::string altAllele, int direction);
        virtual ~Snp();
        static void generateSnpList(boost::ptr_vector<Snp> &snpList, const Command *commander);
        inline std::string getChr() const{return m_chr;};
        inline std::string getRs() const{return m_rs; };
        inline std::string getRef() const{return m_ref; };
        inline std::string getAlt() const{return m_alt; };
        inline size_t getBp() const{return m_bp;};
        inline size_t getSampleSize() const{return m_sampleSize;};
        inline int getDirection() const{return m_direction;};

        Snp(const Snp& that) = delete;
protected:
private:
        std::string m_chr;
        std::string m_rs;
        std::string m_ref;
        std::string m_alt;
        size_t m_bp;
        size_t m_sampleSize;
        int m_direction;
        std::vector<double> m_original;
        std::vector<bool> m_remove;
        static bool sortSnp(const Snp& i, const Snp& j);

};

#endif // SNP_H
