#include "linkage.h"

Linkage::Linkage(size_t thread):m_thread(thread){}
Linkage::~Linkage(){}
std::mutex Linkage::mtx;


void Linkage::Initialize(const boost::ptr_deque<Genotype> &genotype, const size_t &prevResiduals){
	if(genotype.empty()){
        std::runtime_error("Cannot build LD without genotypes");
	}
	//Use previous informations
    if(prevResiduals == 0){
        m_linkage = Eigen::MatrixXd::Zero(genotype.size(), genotype.size());
        //m_linkageSqrt = Eigen::MatrixXd::Zero(genotype.size(), genotype.size());
    }
    else{
		Eigen::MatrixXd temp = m_linkage.bottomRightCorner(prevResiduals,prevResiduals);
		m_linkage= Eigen::MatrixXd::Zero(genotype.size(), genotype.size());
		m_linkage.topLeftCorner(prevResiduals, prevResiduals) = temp;
        //Currently we only need the R2 matrix, so we will ignore the R matrix
		//temp = m_linkageSqrt.bottomRightCorner(prevResiduals,prevResiduals);
		//m_linkageSqrt= Eigen::MatrixXd::Zero(genotype.size(), genotype.size());
		//m_linkageSqrt.topLeftCorner(prevResiduals, prevResiduals) = temp;

    }
}

void Linkage::buildLd(const bool correction, const size_t vStart, const size_t vEnd, const size_t hEnd, const boost::ptr_deque<Genotype> &genotype, const std::deque<size_t> &ldLoc){
    //Will work on all SNPs from vStart to hEnd
    //Not inclusive of hEnd
    /** ldLoc contains the SNP index on file, therefore corresponds to the number in block e.g. vStart, vEnd, hEnd **/
    size_t genotypeStart = genotype.size();
    size_t genotypeEnd = genotype.size(); //Bound
    size_t bottom = genotype.size(); //Bound

    for(size_t i = 0; i < ldLoc.size(); ++i){
        if(ldLoc[i]==vStart) genotypeStart = i;
        else if(ldLoc[i] == vEnd) bottom = i;
        if(ldLoc[i]==hEnd){
            genotypeEnd = i;
            break;
        }
    }
    /** All the ENDs are exclusive **/

    //Linkage::mtx.lock();
    //std::cerr << "Block info: " << genotypeStart << "\t" << genotypeEnd << "\t" << bottom << "\t" << m_linkage.cols() << "\t" << m_linkage.rows() << "\t" << genotype.size()<< std::endl;
    //Linkage::mtx.unlock();
//
    for(size_t i = genotypeStart; i < bottom; ++i){
        //Transversing vertically
        m_linkage(i,i) = 1.0;
        for(size_t j = i+1; j< genotypeEnd+1 && j < genotype.size(); ++j){
            //Transversing horizontally
            if(m_linkage(i,j) == 0.0){ //Again, this is problematic because you are finding equal of double
                double rSquare=0.0;
                double r =0.0;
                genotype[i].GetbothR(&(genotype[j]),correction, r, rSquare);
                m_linkage(i,j) = rSquare;
                m_linkage(j,i) = rSquare;
            }
        }
    }

    //Do extra, the previous loop make sure thread safe and all block will only get "extra", problem is
    //the last block in the matrix will be missing
    if(bottom +1 ==genotype.size()){
        m_linkage(bottom, bottom) = 1.0;
    }
}

void Linkage::Construct(const boost::ptr_deque<Genotype> &genotype, const size_t &genotypeIndex, const size_t& remainedLD, const boost::ptr_vector<Interval> &blockInfo, const bool correction, const std::deque<size_t> &ldLoc){
	if(genotype.empty())    throw std::runtime_error("There is no genotype to work on");
    /** genotypeIndex = beginning block for this iteration **/
    size_t startRange =  genotypeIndex;
    /** remainedLD = number of genotypes that were from previous situations**/
    size_t endRange=0;
    size_t range = m_thread;
    if(remainedLD==0)range +=2;
    else{
        /**
         *  If something is left behind, we also need them for LD construction
         *  As we typically retain the last two block, we backtrack our startRange to two items forward
         */
        startRange -= 2;//Two steps is only right if I have remove stuffs
    }
    std::string currentChr = blockInfo[startRange].getChr();

    bool firstEntry = true;
    size_t boundHunter = genotypeIndex;
    for(; boundHunter < genotypeIndex+range && boundHunter < blockInfo.size(); ++boundHunter){
        if(blockInfo[boundHunter].getChr().compare(currentChr)!=0) break;
        else if(!firstEntry && blockInfo[boundHunter].getStart() != blockInfo[endRange].getEnd()){
            firstEntry = false;
            break;
        }
        else{
            endRange = boundHunter;
        }
    }
    endRange++;
    //Here, endRange = last block that we need to process +1.

    // So now the startRange and endRange will contain the index of the intervals to include in the LD construction
    // It is inclusive, where we need to work on blocks from [startRange, endRange)
    // Each thread will process one sausage       \------------|
    //                                             \-----------|


    std::vector<std::thread> threadStore;
    size_t threadRunCounter = startRange;
    while(threadRunCounter < endRange){
        while(threadStore.size() <= m_thread && threadRunCounter < endRange){
            if(threadRunCounter+2 < endRange){
                //We on purposely
                threadStore.push_back(std::thread(&Linkage::buildLd, this, correction, blockInfo[threadRunCounter].getStart(), blockInfo[threadRunCounter].getEnd(),blockInfo[threadRunCounter+2].getEnd(), std::cref(genotype), std::cref(ldLoc)));
            }
            else if(threadRunCounter+1 < endRange){
                threadStore.push_back(std::thread(&Linkage::buildLd, this, correction, blockInfo[threadRunCounter].getStart(), blockInfo[threadRunCounter].getEnd(),blockInfo[threadRunCounter+1].getEnd(), std::cref(genotype), std::cref(ldLoc)));
            }
            else{
                threadStore.push_back(std::thread(&Linkage::buildLd, this, correction, blockInfo[threadRunCounter].getStart(), blockInfo[threadRunCounter].getEnd(),blockInfo[threadRunCounter].getEnd(), std::cref(genotype), std::cref(ldLoc)));
            }
            threadRunCounter++;
        }

        for (size_t j = 0; j < threadStore.size(); ++j) {
            threadStore[j].join();
        }
        threadStore.clear();
    }
}

void Linkage::print(){
    std::cout << m_linkage << std::endl;
}

void Linkage::solve(const size_t loc, const size_t length, const Eigen::MatrixXd &betaEstimate, Eigen::MatrixXd &heritability, Eigen::MatrixXd &effectiveNumber, Eigen::VectorXd &ldScore) const {
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(m_linkage.block(loc, loc, length, length));
    //double tolerance = std::numeric_limits<double>::epsilon() * length * es.eigenvalues().array().maxCoeff();

    double tolerance = 2.858 * es.eigenvalues(es.eigen)
    Eigen::MatrixXd rInverse = es.eigenvectors()*(es.eigenvalues().array() > tolerance).select(es.eigenvalues().array().inverse(), 0).matrix().asDiagonal() * es.eigenvectors().transpose();
    /** Calculate the h vector here **/
    heritability= rInverse*betaEstimate.block(loc, 0,length,betaEstimate.cols());
    Eigen::MatrixXd error =m_linkage.block(loc, loc, length, length)*heritability - betaEstimate.block(loc, 0,length,betaEstimate.cols());
	double bNorm = betaEstimate.block(loc, 0,length,betaEstimate.cols()).norm();
    double relative_error = error.norm() / bNorm;
    double prev_error = relative_error+1;
    Eigen::MatrixXd update = heritability;
    //Iterative improvement, arbitrary max iteration
    size_t maxIter = 300;
    size_t iterCount = 0;
    while(relative_error < prev_error && iterCount < maxIter){
        prev_error = relative_error;
        update.noalias()=rInverse*(-error);
        relative_error = 0.0;
        error.noalias()= m_linkage.block(loc, loc, length, length)*(heritability+update) - betaEstimate.block(loc, 0,length,betaEstimate.cols());
        relative_error = error.norm() / bNorm;
        if(relative_error < 1e-300) relative_error = 0;
        heritability = heritability+update;
        iterCount++;
    }

    /** have not decide whether if we should use the original method for sd calculation
     *  maybe it will be more accurate now consider we have used the dynamic block size?
     */
    Eigen::MatrixXd vecOfOne = Eigen::MatrixXd::Constant(length,betaEstimate.cols(), 1.0);
    double eNorm = vecOfOne.norm();
    /** Calculate the effective number here **/
    effectiveNumber = rInverse*vecOfOne;
    error = m_linkage.block(loc, loc, length, length)*effectiveNumber-vecOfOne;
    relative_error = error.norm()/eNorm;
    prev_error = relative_error+1;
    update=effectiveNumber;
    iterCount = 0;
    while(relative_error < prev_error && iterCount < maxIter){
        prev_error = relative_error;
        update.noalias()=rInverse*(-error);
        relative_error = 0.0;
        error.noalias()= m_linkage.block(loc, loc, length, length)*(effectiveNumber+update) - vecOfOne;
        relative_error = error.norm() / eNorm;
        if(relative_error < 1e-300) relative_error = 0;
        effectiveNumber = effectiveNumber+update;
        iterCount++;
    }
    /** Old method of variant calculation */
    /*
    Eigen::MatrixXd ncpEstimate = (4*m_linkageSqrt.block(loc, loc, length, length)).array()*((*sqrtChiSq).segment(loc, length)*(*sqrtChiSq).segment(loc, length).transpose()).array();
    Eigen::VectorXd minusF = (Eigen::VectorXd::Constant(length, 1.0)-(*betaEstimate).segment(start, length));
    for(size_t i = 0; i < length; ++i){
        minusF(i) = minusF(i)/((double)sampleSize-2.0+((*sqrtChiSq).segment(start, length))(i)*((*sqrtChiSq).segment(start, length))(i));
    }
    (*variance).noalias() = (rInverse*minusF.asDiagonal()*(ncpEstimate-2*m_linkage.block(start, start, length, length))*minusF.asDiagonal()*rInverse);
    */

    /** Calculate LDSCore **/
    ldScore =m_linkage.block(loc, loc, length, length).colwise().sum();
}