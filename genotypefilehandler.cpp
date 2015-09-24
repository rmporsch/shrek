#include "genotypefilehandler.h"

void GenotypeFileHandler::initialize(Command *commander, const std::map<std::string, size_t> &snpIndex, boost::ptr_vector<Snp> &snpList, boost::ptr_vector<Interval> &blockInfo){
    std::string outputPrefix = commander->getOutputPrefix();
    std::string line;
    //Get the number of samples in the ld file
    std::string famFileName = outputPrefix+".fam";
    std::ifstream famFile;
    famFile.open(famFileName.c_str());
    if(!famFile.is_open()){
        throw std::runtime_error("Cannot open fam file");
    }
    while(std::getline(famFile, line)){
        line = usefulTools::trim(line);
        if(!line.empty()) m_ldSampleSize++;
    }
    famFile.close();
    //Check the bed file, and filter all SNPs not passing the MAF filtering
    std::string bedFileName = m_genotypeFilePrefix+".bed";
	bool bfile_SNP_major = openPlinkBinaryFile(bedFileName, m_bedFile); //We will try to open the connection to bedFile
    if(bfile_SNP_major){
        //This is ok
    }
    else{
        throw std::runtime_error("We currently have no plan of implementing the individual-major mode. Please use the snp-major format");
    }
    //We need to know the number of SNPs when transversing the bed file
    std::string bimFileName = outputPrefix+".bim";
    std::ifstream bimFile;
    bimFile.open(bimFileName.c_str());
    if(!bimFile.is_open()){
        throw std::runtime_error("Cannot open bim file");
    }
    size_t distance = commander->getDistance();
    size_t prevStart = 0;
    while(std::getline(line, bimFile)){
        line = usefulTools::trim(line);
        if(!line.empty()){
            //
        }
    }
    bimFile.close();
    bool snp=false;
    size_t indx = 0; //The iterative count
    size_t alleleCount=0;
    while ( indx < m_ldSampleSize ){
    std::bitset<8> b; //Initiate the bit array
    char ch[1];
    m_bedFile.read(ch,1); //Read the information
    if (!m_bedFile){
        throw std::runtime_error("Problem with the BED file...has the FAM/BIM file been changed?");
    }
    b = ch[0];
    int c=0;
    while (c<7 && indx < m_ldSampleSize ){ //Going through the bit flag. Stop when it have read all the samples as the end == NULL
    //As each bit flag can only have 8 numbers, we need to move to the next bit flag to continue
        ++indx; //so that we only need to modify the indx when adding samples but not in the mean and variance calculation
        int first = b[c++];
        int second = b[c++];
        if(first == 1 && second == 0) first = 0; //We consider the missing value to be reference
        alleleCount += first+second;
    }
                }

                double currentMaf = (alleleCount+0.0)/(2.0*m_ldSampleSize*1.0);
                currentMaf = (currentMaf > 0.5)? 1-currentMaf : currentMaf;
                //remove snps with maf too low
                if(maf >= 0.0 && maf > currentMaf){
                    m_inclusion.back()= -1;
                    //std::cerr << "Snp: " << rs << " not included due to maf filtering" << std::endl;
                    mafSnp++;
                }
                else if(m_inclusion.back() != -1){
                    if(m_chrProcessCount.find(chr)==m_chrProcessCount.end()) m_chrProcessCount[chr] = 1;
                    else m_chrProcessCount[chr]++;
                }


    //Check the bim file for SNP information
    //Need to remove duplicated SNPs, remove SNPs that doesn't pass the MAF filtering and need to check if the bim file is sorted correctly




}



bool GenotypeFileHandler::openPlinkBinaryFile(const std::string s, std::ifstream & BIT){
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
	else if ( ! v1_bfile ){
		if ( b[0] ) bfile_SNP_major = true;
		else bfile_SNP_major = false;
		std::cerr << "Binary PED file is v0.99" << std::endl;
		if (bfile_SNP_major) std::cerr << "Detected that binary PED file is in SNP-major mode" << std::endl;
		else std::cerr << "Detected that binary PED file is in individual-major mode" << std::endl;
	}
	return bfile_SNP_major;
}

