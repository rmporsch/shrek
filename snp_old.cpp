#include "snp.h"
size_t Snp::MAX_SAMPLE_SIZE = 0;

Snp::Snp(std::string chr, std::string rs, size_t bp, size_t sampleSize, double original, std::string refAllele, std::string altAllele, int direction):m_chr(chr), m_rs(rs), m_ref(refAllele), m_alt(altAllele), m_bp(bp), m_sampleSize(sampleSize),m_original(original),m_direction(direction){}
Snp::~Snp(){}

void Snp::computeVarianceExplained(const Command &commander){
    //There are 4 possibilities
    bool qt = commander.quantitative();
    bool cc = commander.caseControl();
    bool rqt = commander.conRisk();
    bool rcc = commander.diRisk();
    bool isP = commander.isPvalue();
    if(qt || rqt){
        //Sample size should already be obtained for the qt runs
        if(isP){
            double beta = usefulTools::qnorm(1.0-((m_original+0.0)/2.0));
            if(m_original >= 1.0) beta = 0.0;
            else if(m_original ==0.0){ //This is unsafe as == of double is always a problem
                beta=0.0;
            }
            beta = beta*m_direction;
            if(rqt) beta = beta/sqrt(m_sampleSize-2.0+beta*beta);
            else{
                beta = beta*beta;
                beta = (beta-1.0)/(m_sampleSize-2.0+beta);
            }
            m_beta=beta;
        }
        else{
            double beta = m_original;
            if(rqt) beta = beta/sqrt(m_sampleSize-2.0+beta*beta);
            else if(qt){
                beta=beta*beta;
                beta = (beta-1.0)/(m_sampleSize-2.0+beta);
            }
            m_beta=beta;
        }
    }
    else if(cc || rcc){
        size_t caseSize=commander.getCaseSize();
        size_t controlSize=commander.getControlSize();
        if(isP){
            double beta = usefulTools::qnorm(1.0-((m_original+0.0)/2.0));
            if(m_original >= 1.0) beta = 0.0;
            else if(m_original ==0.0){//This is unsafe as == of double is always a problem
                beta=0.0;
            }
            beta = beta*m_direction;
            if(rcc) beta = beta/sqrt(caseSize+controlSize-2.0+beta*beta);
            else{
                beta = beta*beta;
                beta = (beta-1.0)/(caseSize+controlSize-2.0+beta);
            }
            m_beta=beta;
        }
        else{
            double beta =m_original;
            if(rcc){
                beta = sqrt(beta)*m_direction;
                beta = (beta)/(caseSize+controlSize -2.0+beta*beta);
            }
            else if(cc){
                beta = (beta-1.0)/(caseSize+controlSize -2.0+beta);
            }
            m_beta=beta;
        }
    }
    else{
        throw std::runtime_error("Undefined mode for SNP processing");
    }
}


void Snp::generateSnpIndex(std::map<std::string, size_t> &snpIndex, boost::ptr_vector<Snp> &snpList, const Command &commander, const Region &regionList, std::vector<int> &genoInclusion){
    std::vector<size_t> regionIncrementationIndex(regionList.getNumRegion(), 0);
	size_t duplicate = 0;
    //check if it is risk prediction
    std::map<std::string, size_t> snpIndexTemp;
    std::string line;
	for(size_t i = 0; i < snpList.size(); ++i){
        //If the snp is new
        if(snpIndexTemp.find(snpList[i].getRs())== snpIndexTemp.end()){
            snpIndexTemp[snpList[i].getRs()] =i ;

            snpList[i].computeVarianceExplained(commander);
            //The default flag (with LD), is always false at this stage
            snpList[i].m_regionFlag.push_back(false);
            if(regionList.getNumRegion() != 0){
                std::vector<bool> padding(regionList.getNumRegion(), false);
                snpList[i].m_regionFlag.insert(snpList[i].m_regionFlag.end(), padding.begin(), padding.end());
            }
            for(size_t j = 0; j < regionList.getNumRegion(); ++j){
                for(unsigned k = regionIncrementationIndex.at(j); k < regionList.getIntervalSize(j); ++k){
                    //check whether if this snp falls within the region
                    if( regionList.getChr(j,k).compare(snpList[i].getChr())==0 &&
                        regionList.getStart(j,k) <= snpList[i].getBp() &&
                        regionList.getEnd(j,k) >= snpList[i].getBp()){

                        regionIncrementationIndex.at(j) = k;
                        snpList[i].setFlag(j+1, true);
                        break;
                    }
                }
            }
        }
        else{
            duplicate++;
        }

    }
    if(commander.diRisk() || commander.conRisk()){
        //Here we will update the SNP Index such that only the SNPs found in the genotype file will be used

        std::string bimFileName = commander.getGenotype()+".bim";
        std::ifstream bimFile;
        bimFile.open(bimFileName.c_str());
        if(!bimFile.is_open()){
            throw  "Cannot open genotype file, please check";
        }
        size_t warnings = 0;
        size_t ignoreSnp = 0;
        size_t finalNumSnp= 0;
        while(std::getline(bimFile,line)){
            line = usefulTools::trim(line);
            if(!line.empty()){
                std::vector<std::string> token;
                usefulTools::tokenizer(line, "\t ", &token);
                if(token.size() >=6){
                    std::string chr= token[0];
                    std::string rs = token[1];
                    size_t bp = std::atoi(token[3].c_str());
                    if(snpIndexTemp.find(rs)!= snpIndexTemp.end()){
                        if(!commander.validate() && snpList[snpIndexTemp[rs]].Concordant(chr, bp, rs)){
                        //Add it to the snpIndex
                            if(!commander.validate() && ! snpList[snpIndexTemp[rs]].Concordant(chr, bp, rs)) warnings++;
                            snpIndex[rs] = snpIndexTemp[rs];
                            genoInclusion.push_back(snpIndex[rs]);
                            finalNumSnp++;
                        }
                        else{
                            ignoreSnp++;
                            genoInclusion.push_back(-1);
                        }
                    }
                    else{
                        ignoreSnp++;
                        genoInclusion.push_back(-1);
                    }
                }
            }
        }
        bimFile.close();
        std::cerr << std::endl;
        std::cerr << "Genotype File information: " << std::endl;
        std::cerr << "========================================" << std::endl;
        std::cerr << "Invalid SNPs:      " << ignoreSnp << std::endl;
        std::cerr << "Concordant SNPs:   " << finalNumSnp << std::endl;
        if(warnings!=0) std::cerr << "WARNING: " << warnings << " invalid SNPs included" << std::endl;
    }
    else{
        snpIndex = snpIndexTemp;
    }

}

void Snp::setFlag(const size_t i, bool flag){
    m_regionFlag.at(i) = flag;
}

void Snp::generateSnpList(boost::ptr_vector<Snp> &snpList, const Command &commander){
    //First, we read the p-value file
    std::ifstream pValue;
    pValue.open(commander.getPvalueFileName().c_str());
    if(!pValue.is_open()){
        throw std::runtime_error("Cannot read the p-value file");
    }
    std::string line;
    //Assume the p-value file has a header
    std::getline(pValue, line);
    // Know what we are dealing with at the moment
    bool qt = commander.quantitative();
    bool cc = commander.caseControl();
    bool rqt = commander.conRisk(); //Kept them here for now, they are useless
    bool rcc = commander.diRisk(); //Kept them here for now, they are useless
    // Check whether if we have p-value or summary statistic
    bool isP = commander.isPvalue();
    // Check whether if the sample size is provided or not (if not, then we need to read from
    // the file
    bool sampleProvided = commander.sampleSizeProvided();


    size_t expectedTokenSize=0; //This is also the index, the token need to be bigger than this
    size_t chrIndex = commander.getChr();
    expectedTokenSize =(expectedTokenSize<chrIndex)?chrIndex:expectedTokenSize;
    size_t rsIndex = commander.getRs();
    expectedTokenSize =(expectedTokenSize<rsIndex)?rsIndex:expectedTokenSize;
    size_t bpIndex = commander.getBp();
    expectedTokenSize =(expectedTokenSize<bpIndex)?bpIndex:expectedTokenSize;

    size_t sampleIndex = commander.getSampleIndex(); //No harm in reading it, but only update the token size when we actually use it
    if(!sampleProvided && (qt || rqt))   expectedTokenSize =(expectedTokenSize<sampleIndex)?sampleIndex:expectedTokenSize;

    size_t signIndex = commander.getSign();
    if(commander.hasSign()) expectedTokenSize =(expectedTokenSize<signIndex)?signIndex:expectedTokenSize;
    size_t refIndex = commander.getRef();
    if(commander.hasRef()) expectedTokenSize = (expectedTokenSize < refIndex) ?refIndex:expectedTokenSize;
    size_t altIndex = commander.getAlt();
    expectedTokenSize =(expectedTokenSize<signIndex)?signIndex:expectedTokenSize;
    //size_t largestStatIndex =commander.maxStatIndex();
    //expectedTokenSize = (expectedTokenSize < largestStatIndex) ?largestStatIndex : expectedTokenSize;
    size_t statIndex = commander.getStat();
    expectedTokenSize = (expectedTokenSize < statIndex) ?statIndex : expectedTokenSize;

    //size_t numIndex = commander.getStatSize();
    size_t samplesize = commander.getSampleSize();

    size_t lineSkipped = 0;

    std::map<std::string, bool> duplication;
    size_t duplicateCount=0;
    size_t removeCount = 0;
    while(std::getline(pValue, line)){
        line =usefulTools::trim(line);
        if(!line.empty()){
            std::vector<std::string> token;
            usefulTools::tokenizer(line, " \t", &token);
            if(token.size() > expectedTokenSize){
                std::string rsId = token[rsIndex];
                if(duplication.find(rsId) !=duplication.end()){
                    duplicateCount++;
                }
                else{
                    std::string chr = token[chrIndex];
                    size_t bp = atoi(token[bpIndex].c_str());
                    size_t sizeOfSample = samplesize;
                    if(!sampleProvided) sizeOfSample = atoi(token[sampleIndex].c_str());
                    std::string refAllele = "";
                    std::string altAllele = "";
                    int direction = 1;
                    if(rcc || rqt){
                        refAllele = token[refIndex];
                        altAllele = token[altIndex];
                        direction =atof(token[dirIndex].c_str()); //Assumption here, we assume their input is correct
                        if(rcc){
                            //odd ratio
                            direction = (1 <= direction) ?1 : -1;
                        }
                        else if(rqt){
                            //test statistic
                            direction=usefulTools::signum(direction);
                        }
                    }
                    //Now get the statistics
                    //std::vector<std::string> statistics;
                    //for(size_t i = 0; i < numIndex;++i){
                    //    statistics.push_back(token[commander.getStatIndex(i)]);
                    //}
                    if(!usefulTools::isNumeric(token[statIndex])){
                        removeCount++;
                    }
                    else{
                        double statistic = atof(token[statIndex].c_str());
                        if(statistic == 0.0 && isP){ //This is unsafe as == of double is always a problem
                            removeCount++;
                        }
                        else{
                            if(sizeOfSample> Snp::MAX_SAMPLE_SIZE) Snp::MAX_SAMPLE_SIZE = sizeOfSample;
                            snpList.push_back(new Snp(chr, rsId, bp, sizeOfSample, statistic, refAllele, altAllele,direction));
                            duplication[rsId] = true;
                        }
                    }
                }
            }
            else{
                lineSkipped++;
            }
        }
    }

    pValue.close();
    snpList.sort(Snp::sortSnp);

    std::cerr << std::endl;
    std::cerr << "P-value File information: " << std::endl;
    std::cerr << "========================================" << std::endl;
    if(lineSkipped!=0){
        std::cerr << "line(s) skipped:   " << lineSkipped << std::endl;
    }
    if(commander.quantitative() && !commander.sampleSizeProvided()){
        std::cerr << "Max Sample Size:   " << Snp::MAX_SAMPLE_SIZE << std::endl;
    }
    std::cerr << "Duplicated Snps:   " << duplicateCount << std::endl;
    std::cerr << "Filtered Snps:     " << removeCount << std::endl;
    std::cerr << "Final Snps number: " << snpList.size()  << std::endl;
    if(snpList.size() ==0) throw std::runtime_error("Programme terminated as there are no snp provided");
}


bool Snp::sortSnp (const Snp& i, const Snp& j){
    if(i.getChr().compare(j.getChr()) == 0)
		if(i.getBp() == j.getBp())
			return i.getRs().compare(j.getRs()) < 0;
		else
			return i.getBp() < j.getBp();
	else return (i.getChr().compare(j.getChr()) < 0);
}
