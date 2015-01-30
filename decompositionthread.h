#ifndef DECOMPOSITIONTHREAD_H
#define DECOMPOSITIONTHREAD_H

#include <mutex>
#include <Eigen/Dense>
#include <deque>
#include <vector>
#include "linkage.h"
#include "snp.h"

class DecompositionThread
{
	public:
		DecompositionThread(size_t start, size_t length, Eigen::VectorXd *betaEstimate, Linkage *linkage, std::deque<size_t>  *snpLoc, std::vector<Snp*> *snpList, bool chrStart, bool chrEnd);
		virtual ~DecompositionThread();
		static void *ThreadProcesser(void *in);
		void solve();
	protected:
	private:
		size_t m_start;
		size_t m_length;
		Eigen::VectorXd *m_betaEstimate;
		Linkage *m_linkage;
		std::deque<size_t> const *m_snpLoc;
        std::vector<Snp*> *m_snpList;
        bool m_chrStart;
        bool m_chrEnd;
        static std::mutex decomposeMtx;
};

#endif // DECOMPOSITIONTHREAD_H
