#include "decompositionthread.h"

std::mutex DecompositionThread::decomposeMtx;

DecompositionThread::DecompositionThread(size_t start, size_t length, Eigen::VectorXd *betaEstimate, Linkage *linkage, std::deque<size_t>  *snpLoc, std::vector<Snp*> *snpList, bool chrStart, bool lastOfBlock):m_start(start), m_length(length), m_betaEstimate(betaEstimate), m_linkage(linkage), m_snpLoc(snpLoc), m_snpList(snpList), m_chrStart(chrStart), m_lastOfBlock(lastOfBlock){}

DecompositionThread::~DecompositionThread()
{}


void *DecompositionThread::ThreadProcesser(void *in){
    ((DecompositionThread *) in)->solve();
	return nullptr;
}

void DecompositionThread::solve(){
    size_t betaLength = (*m_betaEstimate).rows();
    size_t processLength = m_length;
    if(m_lastOfBlock) processLength=betaLength-m_start;
	Eigen::VectorXd effective = Eigen::VectorXd::Constant(processLength, 1.0);
	//Eigen::VectorXd result = m_linkage->solve(m_start, processLength, m_betaEstimate, &effective); //DEBUG
	Eigen::VectorXd result = m_linkage->quickSolve(m_start, processLength, m_betaEstimate, &effective);
	size_t copyStart = m_length/3;
	size_t copyEnd = m_length/3;
	if(m_chrStart && m_start == 0){
		copyStart = 0;
        copyEnd += m_length/3;
	}
	if(m_lastOfBlock) copyEnd = processLength-copyStart;
	//std::cerr << "Copy start: " << copyStart << "\t" << copyEnd << "\t" << m_start << "\t" << m_lastOfBlock << "\t" <<  betaLength << std::endl;
	//DecompositionThread::decomposeMtx.unlock();
    for(size_t i = copyStart; i < copyStart+copyEnd; ++i){
		(*m_snpList)[(*m_snpLoc)[m_start+i]]->Setheritability(result(i));
		(*m_snpList)[(*m_snpLoc)[m_start+i]]->Seteffective(effective(i));
	}
	//DecompositionThread::decomposeMtx.unlock();
}
