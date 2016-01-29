#include "genotypefilehandler.h"

GenotypeFileHandler::GenotypeFileHandler()
{
    //ctor
}

GenotypeFileHandler::~GenotypeFileHandler()
{
    //dtor
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
	else if ( !v1_bfile ){
		if ( b[0] ) bfile_SNP_major = true;
		else bfile_SNP_major = false;
		std::cerr << "Binary PED file is v0.99" << std::endl;
		if (bfile_SNP_major) std::cerr << "Detected that binary PED file is in SNP-major mode" << std::endl;
		else std::cerr << "Detected that binary PED file is in individual-major mode" << std::endl;
	}
	return bfile_SNP_major;
}





void GenotypeFileHandler::initialize(const Command &commander, const std::map<std::string, size_t> &snpIndex, boost::ptr_vector<Snp> &snpList){
    m_keepAmbiguous = commander.keepAmbiguous();
    m_thread = commander.getNThread();
    m_genotypeFilePrefix = commander.getReferenceFilePrefix();
    m_mafThreshold = commander.getMAF();
    bool keepAmbiguous = commander.keepAmbiguous();
    // First, we need to know the number of samples such that we can read the plink binary file correctly
    std::string famFileName = m_genotypeFilePrefix+".fam";
    std::ifstream famFile;
    famFile.open(famFileName.c_str());
    if(!famFile.is_open()){
        throw std::runtime_error("Cannot open fam file");
    }
    std::string line;
    while(std::getline(famFile, line)){
        line = usefulTools::trim(line);
        if(!line.empty()) m_nRefSample++;
    }
    famFile.close();
    // End up realized that if I want to have a dynamic block construction
    // when I read the SNPs, I will still need to read both the bim and bed
    // files together, so don't bother with reading the file now

    // However, I should still initialize the bed file

    std::string bedFileName = m_genotypeFilePrefix+".bed";

    bool bfile_SNP_major = openPlinkBinaryFile(bedFileName, m_bedFile); //We will try to open the connection to bedFile
    if(!bfile_SNP_major)
        throw std::runtime_error("We currently have no plan of implementing the individual-major mode. Please use the snp-major format");
    std::string bimFileName = m_genotypeFilePrefix+".bim";
    m_bimFile.open(bimFileName.c_str());
    if(!m_bimFile.is_open()){
        throw std::runtime_error("Cannot open bim file");
    }
}



void GenotypeFileHandler::getSNP(const std::map<std::string, size_t> &snpIndex, boost::ptr_deque<Snp> &snpList, boost::ptr_deque<Genotype> &genotype, std::deque<size_t> &snpLoc, bool &finalizeBuff, bool &completed, std::deque<size_t> &boundary){
    // The inclusion should be done on the fly
    // Very complicated need to write carefully
    // Something we know
    // When boundary is empty, we will get more SNPs for more block (required for the removal of perfect LD)
    // When the boundary is not empty, we only need to read SNPs until the first SNP
    // that is Xbp away from the last
    // We can replace submatrix, which will come in handy when we perform the perfect LD removal
    // When we reached the end of the chromosome or when the distance between the two SNPs are too large
    // we will set finalizeBuff to true such that other function will start cleaning up
    // An important thing to note is that the bim will run one line ahead of bed
    // So we need something to store the chr, rs, loc beforehand
    // maybe we can first perform the QC anyway
    /** The content of the boundary is important so it is worth the time to document it
     *  The boundary should contain the boundaries of the block. The start of the first
     *  block will always be 0, therefore the first entry of the boundary will be the
     *  start of the second block, which is not included in the first block. So let the
     *  start of the second block be x, then the first block will be [0,x) and second
     *  block will be [x,y). Also, the distance of SNP[x] - SNP[0] >= defined distance
     */
    // If the boundary size is 0, then we need a lot of blocks (6) whereas when the boundary
    // size isn't 0, then we only need to read one more
    size_t nBlock = (boundary.size()==0)? 6:1;
    std::string line;
    // Now deal with the previous bim line
    if(m_prevChr!=""){
        // This is new, so we should at least read one line of the bimFile first
        std::getline(m_bimFile, line);
        line = usefulTools::trim(line);
        while(line.empty()){
            //Read until we find the first line with content
            if(std::getline(m_bimFile, line)) line = usefulTools::trim(line);
            else throw "The whole bim file is empty!";
        }
        std::vector<std::string> token;
        usefulTools::tokenizer(line, "\t ", &token);
        if(token.size() >=6){
            m_prevChr = token[0];
            m_prevLoc = atoi(token[3].c_str());
            std::string rs = token[1];
            if(m_duplicateCheck.find(rs)!=m_duplicateCheck) m_nDuplicated++;
            else if(snpIndex.find(rs) != snpIndex.end()){
                //This is something we have, so we check if we need it
                size_t location = snpIndex[rs];
                bool ambig = false;
                std::string refAllele = token[4];
                std::string altAllele = token[5];
                if(snpList.at(location).concordant(m_prevChr, m_prevLoc, rs, refAllele, altAllele, ambig )){
                    if(!m_keepAmbiguous && ambig) m_nAmbig ++;
                    else{
                        //This is found in the reference file, so it is in the base set
                        snpList.at(location).setFlag(0, true);
                        duplicateCheck[rs] = true;
                        snpList.at(location).setStatus('I');
                        m_nSnp++;
                        m_include = true; //This indicate that we need this SNP
                    }
                }
                else{
                    snpList.at(location).setStatus('v');
                    m_nInvalid++;
                }
            }
        }
        else throw "Malformed bim file, does not contain all necessary information";
    }

    // From this point onward, we should remember that the bim file is ahead of the
    // bed file, so we should only refer to the m_prevChr, m_prevLoc
    // and at the end of the while function, we should read the next line of bim file

    // Now we start the SNP reading part

    while(nBlock>0){



        m_include =false; //This is the default, only set it to true when we need it

        //When no more things are left to read
        if(!std::getline(m_bimFile, line)) break;
        else{
            //When there is still something to read
            line = usefulTools::trim(line);
            bool ending = false;
            while(line.empty()){
                if(std::getline(m_bimFile, line)) line = usefulTools::trim(line);
                else{
                    ending =true;
                    break; //This is the end. Only come in because the end few lines are all empty
                }
            }
            if(ending) break; //All things are finished
            else{
                std::vector<std::string> token;
                usefulTools::tokenizer(line, "\t ", &token);
                if(token.size() >=6){
                    m_prevChr = token[0];
                    m_prevLoc = atoi(token[3].c_str());
                    std::string rs = token[1];
                    if(m_duplicateCheck.find(rs)!=m_duplicateCheck) m_nDuplicated++;
                    else if(snpIndex.find(rs) != snpIndex.end()){
                        //This is something we have, so we check if we need it
                        size_t location = snpIndex[rs];
                        bool ambig = false;
                        std::string refAllele = token[4];
                        std::string altAllele = token[5];
                        if(snpList.at(location).concordant(m_prevChr, m_prevLoc, rs, refAllele, altAllele, ambig )){
                            if(!m_keepAmbiguous && ambig) m_nAmbig ++;
                            else{
                                //This is found in the reference file, so it is in the base set
                                snpList.at(location).setFlag(0, true);
                                duplicateCheck[rs] = true;
                                snpList.at(location).setStatus('I');
                                m_nSnp++;
                                m_include = true; //This indicate that we need this SNP
                            }
                        }
                        else{
                            snpList.at(location).setStatus('v');
                            m_nInvalid++;
                        }
                    }
                }
            }
        }
    }
}


/*
    Store the script here for now
    std::string bimFileName = m_genotypeFilePrefix+".bim";
    std::ifstream bimFile;
    bimFile.open(bimFileName.c_str());
    if(!bimFile.is_open()){
        throw std::runtime_error("Cannot open bim file");
    }
    size_t nDuplicated= 0; //Check for duplication within the REFERENCE
    size_t nInvalid=0; // SNPs that have different information as the p-value file
    size_t nSnp=0;
    size_t nAmbig=0; // Number of SNPs that are ambiguous

	std::string prevChr="";
	std::map<std::string, bool> duplicateCheck,sortCheck; //The sortCheck is basically a sanity check, bim file should be sorted to start with

    while(std::getline(bimFile, line)){
        line = usefulTools::trim(line);
        if(!line.empty()){
            std::vector<std::string> token;
            usefulTools::tokenizer(line, "\t ", &token);
            if(token.size() >=6){
                std::string chr= token[0];
                std::string rs = token[1];
                size_t bp = std::atoi(token[3].c_str());
                std::string refAllele = token[4];
                std::string altAllele = token[5];
                m_nSnp++; //Number of total input Snps
                m_inclusion.push_back(-1); // -1 = now include, otherwise this is the index
                if(prevChr.empty()){
                    sortCheck[chr]=true;
                }
                else if(prevChr.compare(chr) != 0){
                    //Check whether if the bim file is sorted correctly
                    if(sortCheck.find(chr)!=sortCheck.end())
                        throw std::runtime_error("The programme require the SNPs to be sorted according to their chromosome.");
                    else sortCheck[chr] = true;
                }
                //Check if the SNP is duplicated in the reference panel
                if(snpIndex.find(rs)!=snpIndex.end() && duplicateCheck.find(rs)==duplicateCheck.end()){
                    //This is something that we need
                    size_t snpLoc = snpIndex.at(rs);
                    bool ambig = false;
                    if(snpList.at(snpLoc).concordant(chr, bp, rs, refAllele, altAllele, ambig)){
                        if(!keepAmbiguous && ambig) nAmbig ++;
                        else{
                            //This is found in the reference file, so it is in the base set
                            snpList.at(snpLoc).setFlag(0, true);
                            m_inclusion.back()=snpLoc;
                            duplicateCheck[rs] = true;
                            snpList.at(snpLoc).setStatus('I');
                            nSnp++;
                        }
                    }
                    else{
                        snpList.at(snpLoc).setStatus('v');
                        nInvalid++;
                    }
                }
                else if(duplicateCheck.find(rs) != duplicateCheck.end()) nDuplicated++;
            }

        }
    }
    bimFile.close();
    fprintf(stderr, "\n");
    fprintf(stderr, "Reference File Information:\n");
    fprintf(stderr, "================================\n");
    fprintf(stderr, "%lu samples found in the reference panel\n", m_nRefSample);
    if(nDuplicated != 0) fprintf(stderr, "%lu duplicated SNP(s) in the reference panel\n", nDuplicated);
    if(nInvalid != 0) fprintf(stderr, "%lu discordant SNP(s)\n", nInvalid);
    if(nAmbig!= 0 ) fprintf(stderr, "%lu ambiguous SNP(s) removed\n", nAmbig);
*/
