#include "linkage.h"

std::mutex Linkage::linkageMtx;

Linkage::Linkage(size_t thread):m_thread(thread){}
Linkage::~Linkage(){}


void Linkage::computeLd(const boost::ptr_list<Genotype> &genotype, const std::list<size_t> &snpLoc, size_t startIndex, size_t verEnd, size_t horistart, size_t horiEnd, boost::ptr_vector<Snp> &snpList, const bool &correction, std::vector<size_t> &perfectLd){
linkageMtx.lock();
    fprintf(stderr, "Start: %lu\n", startIndex);
    fprintf(stderr, "VerEnd: %lu\n", verEnd);
    fprintf(stderr, "HorStart: %lu\n", horistart);
    fprintf(stderr, "HorEnd: %lu\n", horiEnd);
linkageMtx.unlock();

    std::vector<size_t> perfectBuff;
    boost::ptr_list<Genotype>::const_iterator  iter = genotype.begin(), endIter = genotype.begin(), horiEndIter=genotype.begin();
    std::advance(iter, startIndex);
    std::advance(endIter, verEnd);
    std::advance(horiEndIter, horiEnd);

    size_t i = startIndex;
    for(; iter != endIter; ++iter){
        size_t nextStart = (i>horistart)? i:horistart;
        size_t j = nextStart;
        boost::ptr_list<Genotype>::const_iterator  horiStartIter=genotype.begin();
        std::advance(horiStartIter, nextStart);
        for(; horiStartIter!=horiEndIter;  ++horiStartIter){
                if(i==j){
                    m_linkage(i,j) = 1.0;
                    m_linkageSqrt(i,j) = 1.0;
                }
                else{
                    double r = 0.0;
                    double r2 = 0.0;
                    (*iter).GetbothR((*horiStartIter), correction, r, r2);
                    m_linkage(i,j) = r2;
                    m_linkageSqrt(i,j) = r;
                    // Now check if it is perfect LD
                    // Store the index j into perfectBuff
                    if(std::fabs(r-1.0) < 1e-6 || r > 1.0 || r < -1.0) perfectBuff.push_back(j);
                }
            //This is for the update of the matrix indexing;
            ++j;
        }
        ++i; //Again, this is for the update of matrix index
    }
    // Now we can insert the buff back to the result
    linkageMtx.lock();
        perfectLd.insert(perfectLd.end(), perfectBuff.begin(), perfectBuff.end());
    linkageMtx.unlock();

}



void Linkage::construct(boost::ptr_list<Genotype> &genotype, std::list<size_t> &snpLoc, std::deque<std::list<size_t>::iterator > &boundary, boost::ptr_vector<Snp> &snpList, const bool correction){
    // This is yet another complicated algorithm
    // The aim of this function is to construct the required LD matrix
    // (Think about it, maybe I will shift the perfect LD removal to the other function)
    if(genotype.empty()){
        throw std::runtime_error("Cannot build LD without genotypes");
	}
    // If this is the first of its kind, then we need to initialize the matrix
    bool start = false;
	if(m_linkage.n_cols==0){
        start = true;
        m_linkage = arma::mat(genotype.size(), genotype.size(),arma::fill::eye);
        m_linkageSqrt = arma::mat(genotype.size(), genotype.size(), arma::fill::eye);
    }
    else{ // Otherwise, just make sure the matrix is of the right size
        m_linkage.resize(genotype.size(), genotype.size());
        m_linkageSqrt.resize(genotype.size(), genotype.size());
    }
    // We don't need the buff information
    // because for this function, we should build all the matrix anyway
    // Now if we are not working on the new stuff, we assume the LD matrix has been clean
    // of the unwanted materials
    // So we only need to construct the matrix here
    // When we construct the LD matrix, we do it per SNP level but still use multi-threading
    // then we skip any SNP that are found to be in perfect LD with others
    // This will allow us to avoid the need of removing rols (only column)
    // which should be fast considering armadillo is column major
    std::vector<size_t> perfectLd; // This is the vector use to store the perfect LD information
    std::vector<std::thread> threadStore;
    if(boundary.size() < 3){
        //Very small input, so we will do the whole part together
        size_t startIndex = 0, endBound= genotype.size();
        if(genotype.size() < m_thread){
            // Use as much thread as we can
            for(size_t i = startIndex; i < endBound; ++i){
                threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), i, i+1, i,genotype.size(), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
            }
            for (size_t i = 0; i < threadStore.size(); ++i) {
                threadStore[i].join();
            }
            threadStore.clear();
        }
        else{
            // distribute the work
            int jobBlock = endBound/m_thread;
            int remainder = endBound % m_thread;
            int currentBlock = startIndex;
            for(size_t i = 0; i < m_thread; ++i){
                // For each thread
                if(remainder > 0){
                    threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock+1, currentBlock, genotype.size(), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                    --remainder;
                    currentBlock+=jobBlock+1;
                }
                else{
                    threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock, currentBlock, genotype.size(), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                    currentBlock+=jobBlock;
                }

            }
            for (size_t i = 0; i < threadStore.size(); ++i) {
                threadStore[i].join();
            }
            threadStore.clear();
        }
    }
    else{ // This is big enough
        // Two situation, either this is the start or this is not the start
        if(start){
            // If this is the start, then we need special handling of the first block
            // in order to have a faster computation
            size_t endBound =(*(boundary[1]));
            size_t startIndex = 0;
            int numSnps = endBound-startIndex;
            assert(numSnps>0 && "There must be some SNPs for the construct to work with!");
            if(numSnps < (signed)m_thread){
                // Use as much threads as possible
                for(size_t i = startIndex; i < endBound; ++i){
                    threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), i, i+1, i, (*(boundary.back())), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                }
                for (size_t i = 0; i < threadStore.size(); ++i) {
                    threadStore[i].join();
                }
                threadStore.clear();
            }
            else{
                // divide the jobs
                int jobBlock = numSnps/m_thread;
                int remainder =numSnps%m_thread;
                int currentBlock = startIndex;
                for(size_t i = 0; i < m_thread; ++i){
                    if(remainder>0){
                        threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock+1, currentBlock, (*(boundary.back())), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                        currentBlock+= jobBlock+1;
                        remainder--;
                    }
                    else{
                        threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock, currentBlock, (*(boundary.back())), std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                        currentBlock+= jobBlock;
                    }
                }
                for (size_t i = 0; i < threadStore.size(); ++i) {
                    threadStore[i].join();
                }
                threadStore.clear();
            }
        }

        // If it is not the start, then we can ignore the first block
        // and only need to compute the end of each block as most were already calculated before
        size_t startIndex = (*(boundary[1]));
        size_t endBound = genotype.size();
        // We do have enough stuff to work on
        int numSnps = endBound-startIndex;
        assert(numSnps>0 && "There must be some SNPs for the construct to work with!");

        size_t horiStart = (start)? startIndex : (*(boundary.back()));
        if(numSnps< (signed) m_thread){
            //very unlikely, but we will consider it anyway
            for(size_t i = startIndex; i < endBound; ++i){
                threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), i, i+1, horiStart, endBound, std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
            }
            for (size_t i = 0; i < threadStore.size(); ++i) {
                threadStore[i].join();
            }
            threadStore.clear();
        }
        else{
            // Divide the jobs
            int jobBlock = numSnps/m_thread;
            int remainder =numSnps%m_thread;
            int currentBlock = startIndex;
            for(size_t i = 0; i < m_thread; ++i){
                if(remainder>0){
                    threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock+1, horiStart, endBound, std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                    currentBlock+= jobBlock+1;
                    remainder--;
                }
                else{
                    threadStore.push_back(std::thread(&Linkage::computeLd, this, std::cref(genotype), std::cref(snpLoc), currentBlock, currentBlock+jobBlock, horiStart, endBound, std::ref(snpList), std::cref(correction), std::ref(perfectLd)));
                    currentBlock+= jobBlock;
                }
            }
            for (size_t i = 0; i < threadStore.size(); ++i) {
                threadStore[i].join();
            }
            threadStore.clear();
        }
        // Join all the results

    }
    std::cerr << "Problem: " << perfectLd.size() << std::endl;
    std::sort(perfectLd.begin(), perfectLd.end());
    // Now we want to remove duplicated index
    perfectLd.erase( std::unique( perfectLd.begin(), perfectLd.end() ), perfectLd.end() );
    //Here goes the crazy part
    for(size_t i = 0; i < perfectLd.size(); ++i) std::cerr << perfectLd[i] << std::endl;


    std::cout << m_linkage << std::endl;
    exit(-1);
    // Now we need to handle the perfect LD stuff
}



