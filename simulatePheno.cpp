#include <stdio.h>
#include <random>
#include <armadillo>
#include <map>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <string>
#include <time.h>
#include <bitset>
#include <algorithm>
#include "usefulTools.h"

bool openPlinkBinaryFile(const std::string s, std::ifstream & BIT);

typedef std::pair<size_t, size_t>  subindex;

int main(int argc, char* argv[]){
    if(argc < 4){
	fprintf(stderr, "Usage: %s <Plink prefix> <Number of Sets> <String of Causal SNPs> <Seed>\n", argv[0]);
	return EXIT_FAILURE;
    }
    std::string plinkPrefix = argv[1];
    int numSet = atoi(argv[2]);
    std::string causalSets = argv[3];
    causalSets.erase(std::remove(causalSets.begin(), causalSets.end(), '\"'), causalSets.end());
    double seed = (argc!=5)? 0: atof(argv[4]);
    std::vector<std::string> token;
    usefulTools::tokenizer(causalSets, " ", &token);
    std::vector<int> numCausal;
    if(token.size() > 0){
	for(size_t i = 0; i < token.size(); ++i){
	    int num = atoi(token[i].c_str());
	    if(num > 0){
		numCausal.push_back(num);
	    }
	}
    }
    else{
	fprintf(stderr, "No information on number of causal SNPs!\n");
	fprintf(stderr, "Terminated!\n");
	return EXIT_FAILURE;
    }

    //Now we read the bim file to get all the SNP names and fam file for all the sample names
    std::vector<std::string> sampleName;
    std::vector<std::string> snpName;
    std::string bimFileName = plinkPrefix+".bim";
    std::string famFileName = plinkPrefix+".fam";
    std::ifstream bimFile, famFile;
    bimFile.open(bimFileName.c_str());
    famFile.open(famFileName.c_str());
    bool error=false;
    if(!bimFile.is_open()){
	error = true;
	fprintf(stderr, "Cannot open bim file: %s\n", bimFileName.c_str());
    }
    if(!famFile.is_open()){
	error = true;
	fprintf(stderr, "Cannot open fam file: %s\n", famFileName.c_str());
    }
    if(error) return EXIT_FAILURE;
    std::string line;
    while(std::getline(bimFile, line)){
	line = usefulTools::trim(line);
	if(!line.empty()){
	    usefulTools::tokenizer(line, " \t", &token);
	    if(token.size() > 2){
		snpName.push_back(token[1]);
	    }
	}
    }
    bimFile.close();
    while(std::getline(famFile, line)){
	line=usefulTools::trim(line);
	if(!line.empty()){
	    usefulTools::tokenizer(line, " \t", &token);
	    if(token.size() > 2){
		sampleName.push_back(token[1]);
	    }
	}
    }
    famFile.close();


    //Now generate the required effect Sizes
    std::default_random_engine generator;
    std::exponential_distribution<double> distribution(1);
    std::uniform_int_distribution<int> uniDist(0, snpName.size()-1);
    std::vector<std::vector<double> > effectSize;
    std::multimap<size_t, subindex> effectIndex;
    std::vector<std::vector<std::string> > causalSnpName;
    std::vector<size_t> indexList;
    std::map<size_t, bool> check;
    for(size_t i = 0; i < numCausal.size(); ++i){
	//For each number of causal SNPs, we will repeat Number of Sets times
	if(numCausal[i] >= snpName.size()){
	    fprintf(stderr, "Require more causal SNPs than available!\n");
	    return EXIT_FAILURE;
	}
	for(size_t j = 0; j < numSet; ++j){
	    std::vector<double> effect;
	    std::vector<std::string> names;
	    std::map<std::string, bool> duplicated;
	    for(size_t k = 0; k < numCausal[i]; ++k){
		effect.push_back(distribution(generator));
		int index = -1;
		while(index < 0 || index >= snpName.size()){
		    index = uniDist(generator);
		}
		std::string name = snpName[index];
		while(duplicated.find(name) != duplicated.end()){
		    index = uniDist(generator);
		    while(index<0||index >= snpName.size()){
			index= uniDist(generator);
		    }
		    name =snpName[index];
		}
		if(check.find(index)==check.end()){
		    check[index]=true;
		    indexList.push_back(index);
		}
		duplicated[name]=true;
		names.push_back(name);
		effectIndex.insert(std::pair<size_t ,subindex >(index, subindex(effectSize.size(),k )) );
	    }

	    causalSnpName.push_back(names);
	    effectSize.push_back(effect);
	}
    }
    // Now we've got all the Causal SNPs and their corresponding effect sizes
    sort(indexList.begin(), indexList.end());
    std::string effectName = plinkPrefix+".eff";
    std::string causeName = plinkPrefix+".snp";
    std::ofstream effectOutput, causeOutput;
    effectOutput.open(effectName.c_str());
    causeOutput.open(causeName.c_str());
    if(!effectOutput.is_open()){
	fprintf(stderr, "Cannot open effect file to write: %s\n", effectName.c_str());
	return EXIT_FAILURE;
    }
    if(!causeOutput.is_open()){
	fprintf(stderr, "Cannot open file to write the causal SNP to: %s\n", causeName.c_str());
	return EXIT_FAILURE;
    }
    for(size_t i = 0; i < effectSize.size(); ++i){
	size_t currentSize = effectSize[i].size();
	effectOutput << effectSize[i][0];
	causeOutput << causalSnpName[i][0];
	for(size_t j=1; j< currentSize; ++j){
	    effectOutput<< " " << effectSize[i][j];
	    causeOutput << " " << causalSnpName[i][j];
	}
	effectOutput<<std::endl;
	causeOutput << std::endl;
    }
    effectOutput.close();
    causeOutput.close();
    //This is the result
    arma::mat result = arma::mat(sampleName.size(), numSet*numCausal.size(),arma::fill::zeros);
    //arma::mat genotype = arma::mat(sampleName.size(),indexList.size(), arma::fill::zeros);
    //Start working on the bim file
    std::string bedFileName = plinkPrefix+".bed";
    std::ifstream bedFile;
    bool snpMajor = openPlinkBinaryFile(bedFileName, bedFile);
    if(!snpMajor){
	fprintf(stderr, "Current only support SNP major format!\n");
	return EXIT_FAILURE;
    }
    //Now start reading the file and getting the genotype information.
    size_t nBytes=ceil((double)sampleName.size()/4.0);
    size_t prevInfo = 0;
    size_t totalLine =0;
    for(size_t i =0; i < indexList.size(); ++i){
	//As the index is 0 base, it represent the number of SNPs that we are going to skip
        bedFile.seekg((indexList[i]-prevInfo) * nBytes, bedFile.cur);
        prevInfo = indexList[i]+1;
        //Now we read the SNP that we are using
        std::bitset<8> b; //Initiate the bit array
        char ch[nBytes];
        bedFile.read(ch, nBytes);
        if (!bedFile) throw std::runtime_error("Problem with the BED file...has the FAM/BIM file been changed?");
        //note on effectIndex, the second value in the value pair contains the column INDEX of the SNP
        std::pair<std::multimap<size_t,subindex >::iterator, std::multimap<size_t, subindex >::iterator> ii;
        std::vector<double> currentEffect;
        std::vector<size_t> effectCol;
        ii = effectIndex.equal_range(indexList[i]);
        for(std::multimap<size_t, subindex>::iterator it=ii.first; it !=ii.second; ++it){
            currentEffect.push_back(effectSize[it->second.first][it->second.second]);
            effectCol.push_back(it->second.first);
        }
        arma::vec currentgeno = arma::vec(sampleName.size(), arma::fill::zeros);
        double oldM=0.0, newM=0.0,oldS=0.0, newS=0.0; // This is for the online algorithm which calculates the mean + sd in one go
        size_t alleleCount=0, validSample=0;
        size_t j = 0;
        for(size_t jj=0; jj < nBytes; ++jj){
            b=ch[jj];
            size_t c = 0;
            while (c<7 && j<sampleName.size() ){
                int first = b[c++];
                int second = b[c++];
                if(first == 1 && second == 0) first = 3; //Missing value should be 3
                else{ //This is not missing
                    currentgeno(j) = first+second;
                    //genotype(j,i) = first+second;
                    validSample++;
                    alleleCount += first+second;
                    double value = (double)first+second;
                    if(validSample==1) oldM = newM = value;
                    else{
                        newM = oldM + (value-oldM)/(validSample);
                        newS = oldS + (value-oldM)*(value-newM);
                        oldM = newM;
                        oldS = newS;
                    }
                }
                j++;
            }
        }
        //Now we have all the information of the selected SNP
        double mean = (validSample>0) ?newM:0.0;
        double sd = (validSample>1) ? std::sqrt(newS/(validSample - 1.0)) : 0.0;
        double currentMaf = (validSample>0)?(alleleCount+0.0)/(2.0*validSample*1.0):-1.0;
            currentMaf = (currentMaf > 0.5)? 1-currentMaf : currentMaf;
        //By definition of our simulation, the samples should be preprocessed such that we don't need to worry about the MAF
        //Standardize the genotype
        currentgeno= (currentgeno-mean)/sd;
        //Then calculate the XB (There can be multiple B for this particular SNP);
        for(size_t eff = 0; eff < currentEffect.size(); ++eff){
            result.col(effectCol[eff])+=currentgeno*currentEffect[eff];
        }
    }
    bedFile.close();
    std::string phenoName = plinkPrefix+".pheno";
    std::ofstream phenoOut;
    phenoOut.open(phenoName.c_str());
    if(!phenoOut.is_open()){
	fprintf(stderr, "Cannot open phenotype file: %s\n", phenoName.c_str());
	return EXIT_FAILURE;
    }
    phenoOut << result << std::endl;
    phenoOut.close();

    //std::ofstream checking;
    //checking.open("genotype");
    //checking << genotype << std::endl;
    //checking.close();
    return EXIT_SUCCESS;
}




bool openPlinkBinaryFile(const std::string s, std::ifstream & BIT){
    BIT.open(s.c_str(), std::ios::in | std::ios::binary);
    if(!BIT.is_open()){
        throw "Cannot open the bed file";
    }

    //std::cerr << "BIT open" << std::endl;
    // 1) Check for magic number
    // 2) else check for 0.99 SNP/Ind coding
    // 3) else print warning that file is too old
    char ch[1];
    BIT.read(ch,1);
    std::bitset<8> b;
    b = ch[0];
    bool bfile_SNP_major = false;
    bool v1_bfile = true;
    // If v1.00 file format
    // Magic numbers for .bed file: 00110110 11011000 = v1.00 bed file
    //std::cerr << "check magic number" << std::endl;
    if (   ( b[2] && b[3] && b[5] && b[6] ) &&
       ! ( b[0] || b[1] || b[4] || b[7] )    ){
    // Next number
    BIT.read(ch,1);
    b = ch[0];
    if (   ( b[0] && b[1] && b[3] && b[4] ) &&
          ! ( b[2] || b[5] || b[6] || b[7] )    ){
	    // Read SNP/Ind major coding
	    BIT.read(ch,1);
	    b = ch[0];
	    if ( b[0] ) bfile_SNP_major = true;
	    else bfile_SNP_major = false;

	    //if (bfile_SNP_major) std::cerr << "Detected that binary PED file is v1.00 SNP-major mode" << std::endl;
	    //else std::cerr << "Detected that binary PED file is v1.00 individual-major mode" << std::endl;

	} else v1_bfile = false;

    } else v1_bfile = false;
    // Reset file if < v1
    if ( ! v1_bfile ) {
	std::cerr << "Warning, old BED file <v1.00 : will try to recover..." << std::endl;
	std::cerr << "  but you should --make-bed from PED )" << std::endl;
	BIT.close();
	BIT.clear();
	BIT.open(s.c_str(), std::ios::in | std::ios::binary);
	BIT.read(ch,1);
	b = ch[0];
    }
    // If 0.99 file format
    if ( (!v1_bfile) && ( b[1] || b[2] || b[3] || b[4] || b[5] || b[6] || b[7] ) ){
	std::cerr << std::endl << " *** Possible problem: guessing that BED is < v0.99      *** " << std::endl;
	std::cerr << " *** High chance of data corruption, spurious results    *** " << std::endl;
	std::cerr << " *** Unless you are _sure_ this really is an old BED file *** " << std::endl;
	std::cerr << " *** you should recreate PED -> BED                      *** " << std::endl << std::endl;
	bfile_SNP_major = false;
	BIT.close();
	BIT.clear();
	BIT.open(s.c_str(), std::ios::in | std::ios::binary);
    }
    else if ( !v1_bfile ){
	if ( b[0] ) bfile_SNP_major = true;
	else bfile_SNP_major = false;
	std::cerr << "Binary PED file is v0.99" << std::endl;
	if (bfile_SNP_major) std::cerr << "Detected that binary PED file is in SNP-major mode" << std::endl;
	else std::cerr << "Detected that binary PED file is in individual-major mode" << std::endl;
    }
    return bfile_SNP_major;
}


