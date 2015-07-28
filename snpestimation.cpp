#include "snpestimation.h"

SnpEstimation::SnpEstimation(GenotypeFileHandler *genotypeFileHandler, std::map<std::string, size_t> *snpIndex, std::vector<Snp*> *snpList, size_t thread, double maf, bool correction, Region *regionInfo):m_genotypeFileHandler(genotypeFileHandler), m_snpIndex(snpIndex), m_snpList(snpList), m_thread(thread), m_maf(maf), m_correction(correction), m_regionInfo(regionInfo){};

void SnpEstimation::Estimate(){
	Genotype::SetsampleNum(m_genotypeFileHandler->GetsampleSize());
	ProcessCode process = startProcess;
    std::deque<Genotype*> genotype;
    std::deque<size_t> snpLoc;
    bool chromosomeStart= true;
    bool chromosomeEnd = false;
    size_t prevResidual;
    size_t blockSize;
    Linkage *linkageMatrix = new Linkage();
    linkageMatrix->setSnpList(m_snpList);
    linkageMatrix->setSnpLoc(&snpLoc);
    linkageMatrix->setThread(m_thread);
    Decomposition *decompositionHandler = new Decomposition( m_snpList, linkageMatrix, m_thread,m_regionInfo);
    size_t numProcessed = 0;
    size_t totalNum = m_genotypeFileHandler->GetestimateSnpTotal()*3;
	while(process != completed && process != fatalError){
		/** Will have terrible problem if the input is corrupted */
        SnpEstimation::loadbar(numProcessed,totalNum);
        //Work one by one first.
		process = m_genotypeFileHandler->getSnps(genotype, snpLoc, m_snpList, chromosomeStart, chromosomeEnd, m_maf,prevResidual, blockSize);
        //Check if we have enough to remove the previous residual
        if(genotype.size() >= blockSize+blockSize/3 && prevResidual> 0){
            m_regionInfo->ConfirmlastVariance();
            for(size_t i = 0; i < blockSize/3 && prevResidual> 0; ++i){
                genotype.pop_front();
                snpLoc.pop_front();
                prevResidual--;
            }
        }
        else{
            m_regionInfo->CleanlastVariance();
        }
		size_t workSize = blockSize/3*m_thread;
		if(chromosomeStart) workSize +=2/3*blockSize+blockSize;
		if(workSize > snpLoc.size()) workSize = snpLoc.size();
		numProcessed+= workSize;
        SnpEstimation::loadbar(numProcessed,totalNum);
		if(process == completed && prevResidual==genotype.size()){
			//Nothing was updated
			//std::cerr << std::endl;
			m_regionInfo->Debuffer();
			//std::cerr << "completed" << std::endl;
		}
		else{
			//Now calculate the LD matrix
			linkageMatrix->Initialize(genotype, prevResidual, blockSize);
			linkageMatrix->Construct(genotype, prevResidual, blockSize, m_correction);
            numProcessed+= workSize; //Finished the LD construction-+
            SnpEstimation::loadbar(numProcessed,totalNum);

            m_regionInfo->CleanBuffer();
            ProcessCode decomposeProcess = decompositionHandler->Decompose(blockSize, snpLoc, genotype, chromosomeStart, chromosomeEnd);
            numProcessed+= workSize; //Finished the LD construction
            SnpEstimation::loadbar(numProcessed,totalNum);
			if(decomposeProcess == fatalError) throw "Fatal error with Decomposition";
            if(!chromosomeEnd){
				if(blockSize > genotype.size()) blockSize= genotype.size();
				size_t retain = blockSize;
				Genotype::clean(genotype, retain);
				size_t removeCount = snpLoc.size() - retain;
				for(size_t i = 0; i < removeCount; ++i)	snpLoc.pop_front();
            }
            else{
                m_regionInfo->Debuffer();
                Genotype::clean(genotype,0);
                snpLoc.clear();
            }

		}
        if(chromosomeStart && !chromosomeEnd){
            chromosomeStart =false;
        }
        else if(chromosomeEnd){
            m_regionInfo->Debuffer();
            chromosomeStart = true;
            chromosomeEnd = false;
        }

	}
/*
	std::cerr << "Here" << std::endl;
	std::cout << DecompositionThread::checking << std::endl;
    linkageMatrix->print();
	exit(-1);
*/
    std::cout << Linkage::m_testing << std::endl;
	m_regionInfo->Debuffer();
    SnpEstimation::loadbar(totalNum, totalNum);
	std::cerr << std::endl;
	delete linkageMatrix;
	delete decompositionHandler;
    Genotype::clean(genotype, 0);
}

SnpEstimation::~SnpEstimation()
{
	//dtor
}

void SnpEstimation::Getresult(std::string outputPrefix){
    size_t regionSize = m_regionInfo->GetnumRegion();
    std::vector<double> regionEstimate(regionSize, 0.0);
    size_t nSnp = 0;
    size_t index;
    std::map<std::string, size_t>::iterator iter;
    double totalSum = 0.0;
       for(iter= m_snpIndex->begin(); iter != m_snpIndex->end(); ++iter){
		index =iter->second;
        double num = (*m_snpList).at(index)->Getheritability();
        totalSum+= num;
        if((*m_snpList).at(index)->GetFlag(0)){
			nSnp++;
            for(size_t j = 0; j < regionSize;++j){
                if((*m_snpList).at(index)->GetFlag(j)){
                    regionEstimate[j]+= num;
                }
            }
        }
    }

    totalSum *= Snp::Getadjustment();
    for(size_t i = 0; i <regionEstimate.size(); ++i){
        regionEstimate[i] *= Snp::Getadjustment();
    }

	if(outputPrefix.empty()){
        std::cout << "Category\tPositive\tNegative\tVariance" << std::endl;
        for(size_t i =0; i < regionEstimate.size(); ++i){
            std::cout << m_regionInfo->Getname(i) << "\t" << regionEstimate[i] << "\t" << totalSum-regionEstimate[i] << "\t" << m_regionInfo->Getvariance(regionEstimate[i],i, Snp::Getadjustment()) << std::endl;
        }
    }
    else{
		std::string resOutName = outputPrefix+".res";
        std::string resSumName = outputPrefix+".sum";
        std::ofstream resOut, resSum;
        resOut.open(resOutName.c_str());
        resSum.open(resSumName.c_str());


        if(!resOut.is_open()){
            std::cerr << "Cannot open result file: " << resOutName << " for write" << std::endl;
            std::cerr << "Will only provide brief summary output " << std::endl;
        }
        else{
			//We want the output in sorted format. So we will have to do extra works just to make sure the result is sorted
            resOut << "Chr\tLoc\trsID\tOriginal\tBeta\tEstimate\tWithLD\tPerfectLD" << std::endl;
            size_t index;
            std::vector<Snp*> resultSnps;
            for(iter = m_snpIndex->begin(); iter != m_snpIndex->end(); ++iter){
                index = iter->second;
				resultSnps.push_back((*m_snpList).at(index));
            }
            std::sort(resultSnps.begin(), resultSnps.end(), Snp::sortSnp);
            for(size_t i =0; i < resultSnps.size(); ++i){
				resOut << resultSnps[i]->Getchr() << "\t" << resultSnps[i]->Getbp() << "\t" << resultSnps[i]->GetrsId() << "\t" << resultSnps[i]->Getoriginal()<< "\t" << resultSnps[i]->Getbeta() << "\t" <<  resultSnps[i]->Getheritability() << "\t" << resultSnps[i]->GetFlag(0) << "\t" <<  resultSnps[i]->GetperfectId() << std::endl;
                //delete resultSnps[i]; //Avoid double deletion
            }
            resultSnps.clear();
        }
        if(!resSum.is_open()){
            std::cerr << "Cannot open summary file: " << resSumName << " for write" << std::endl;
            std::cerr << "Will display on screen" << std::endl;
            std::cout << "Category\tPositive\tNegative\tVariance" << std::endl;

			for(size_t i =0; i < regionEstimate.size(); ++i){
				std::cout << m_regionInfo->Getname(i) << "\t" << regionEstimate[i] << "\t" << totalSum-regionEstimate[i] << "\t" << m_regionInfo->Getvariance(regionEstimate[i],i, Snp::Getadjustment()) << std::endl;
			}
        }
        else{
            resSum << "Category\tPositive\tNegative\tVariance" << std::endl;
            std::cout << "Category\tPositive\tNegative\tVariance" << std::endl;
			for(size_t i =0; i < regionEstimate.size(); ++i){
				std::cout << m_regionInfo->Getname(i) << "\t" << regionEstimate[i] << "\t" << totalSum-regionEstimate[i] << "\t" << m_regionInfo->Getvariance(regionEstimate[i],i, Snp::Getadjustment()) << std::endl;
				resSum << m_regionInfo->Getname(i) << "\t" << regionEstimate[i] << "\t" << totalSum-regionEstimate[i] << "\t" << m_regionInfo->Getvariance(regionEstimate[i],i, Snp::Getadjustment()) << std::endl;
			}
            resSum.close();
        }
    }

}

 void SnpEstimation::loadbar(size_t x, size_t n){
    //std::cerr << "Calling load bar" << std::endl;
/*
    if(n == 0) throw "Total work size should not be zero";
 	size_t w =60;
	double percent  =  x/(double)n;


    size_t c = percent * w;

    std::cerr << std::setw(3) << (size_t)(percent*100) << "% [";
    for (size_t i=0; i<c; i++) std::cerr << "=";
    for (size_t i=c; i<w; i++) std::cerr << " ";
    std::cerr << "]\r" << std::flush;

*/
}
