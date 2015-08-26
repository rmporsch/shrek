#ifndef RISKPREDICTION_H
#define RISKPREDICTION_H
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include "snp.h"
#include "command.h"
#include "usefulTools.h"

class RiskPrediction
{
    public:
        /** Default constructor */
        RiskPrediction(const Command *commander,std::vector<Snp*> *snpList);
        /** Default destructor */
        virtual ~RiskPrediction();
        void checkGenotype();
    protected:
    private:
        std::vector<Snp*> *m_snpList;
        std::vector<bool> m_flipCheck;
        std::vector<int> m_genoInclude;
        size_t m_thread;
        size_t m_minBlock;
        size_t m_maxBlock;
        double m_maf;
        bool m_ldCorrection;
        bool m_keep;
        bool m_maxBlockSet;
        std::string m_genotypeFilePrefix;
        std::map<std::string, size_t> snpIndex;
};

#endif // RISKPREDICTION_H
