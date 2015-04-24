#ifndef LINKAGETHREAD_H
#define LINKAGETHREAD_H

#include <deque>
#include <mutex>
#include <limits>
#include "linkage.h"
#include "configure.h"
#include "genotype.h"
#include "snp.h"

class LinkageThread
{
	public:
		LinkageThread(bool correction, const size_t blockEnd, Eigen::MatrixXd *ldMatrix, Eigen::MatrixXd *varMatrix, std::deque<Genotype* > *genotype, std::deque<size_t> *snpLoc, std::vector<size_t> *perfectLd, std::vector<Snp*> *snpList);
		LinkageThread(bool correction, const size_t snpStart, const size_t snpEnd, const size_t boundStart, const size_t boundEnd, Eigen::MatrixXd *ldMatrix, Eigen::MatrixXd *varMatrix, std::deque<Genotype* > *genotype, std::deque<size_t> *snpLoc, std::vector<size_t> *perfectLd, std::vector<Snp*> *snpList);
		virtual ~LinkageThread();

		void Addstart(size_t i);
static void *triangularProcess(void *in);
        static void *rectangularProcess(void *in);
	protected:
	private:
        bool m_correction;
        size_t m_snpStart;
        size_t m_snpEnd;
        size_t m_boundStart;
        size_t m_boundEnd;
        Eigen::MatrixXd *m_ldMatrix;
        Eigen::MatrixXd *m_varLdMatrix;
        std::deque<Genotype*> *m_genotype;
        std::deque<size_t> *m_snpLoc;
        std::vector<size_t> m_startLoc;
        std::vector<size_t> *m_perfectLd;
        std::vector<Snp*> *m_snpList;
		void triangularProcess();
        void rectangularProcess();
        static std::mutex mtx;
};

#endif // LINKAGETHREAD_H
