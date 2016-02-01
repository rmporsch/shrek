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


void GenotypeFileHandler::getSNP(const std::map<std::string, size_t> &snpIndex, boost::ptr_vector<Snp> &snpList, boost::ptr_deque<Genotype> &genotype, std::deque<size_t> &snpLoc, bool &finalizeBuff, bool &completed, std::deque<size_t> &boundary){

    // Very complicated need to write carefully
    // Something we know
    // When we reached the end of the chromosome or when the distance between the two SNPs are too large
    // we will set finalizeBuff to true such that other function will start cleaning up
    /** The content of the boundary is important so it is worth the time to document it
     *  The boundary should contain the boundaries of the block. The start of the first
     *  block will always be 0, therefore the first entry of the boundary will be the
     *  start of the second block, which is not included in the first block. So let the
     *  start of the second block be x, then the first block will be [0,x) and second
     *  block will be [x,y). Also, the distance of SNP[x] - SNP[0] >= defined distance
     */
    // If the boundary size is 0, then we need a lot of blocks (3), otherwise, we only
    // need to read one additional block
    size_t nBlock = (boundary.size()==0)? 3:1;
    std::string line;
    bool read = false;
    // We need to first find the next usable SNP
    /** ALWAYS END WITH THE NEXT USABLE SNP FOR THIS TO WORK **/
    while(m_prevChr.empty()){ //Continue to read the file until we find the first Usable SNP
        if(std::getline(m_bimFile, line)) line = usefulTools::trim(line);
        else throw "No valid SNPs from the bim file!";
        while(line.empty()){
            // Read until we find the first line with content
            // This is actually too nice for the user...
            // Who will self modify the plink file to have multiple empty line
            // in the start of the file?
            if(std::getline(m_bimFile, line)) line = usefulTools::trim(line);
            else throw "No valid SNPs from the bim file!";
        }
        std::vector<std::string> token;
        usefulTools::tokenizer(line, "\t ", &token);
        if(token.size() >=6){ // Now make sure the number of column is big enough for it to contain all required information
            std::string rs = token[1];
            if(m_duplicateCheck.find(rs)!=m_duplicateCheck.end()) m_nDuplicated++; // Don't want it if it is duplicated (impossible here actually)
            else if(snpIndex.find(rs) != snpIndex.end()){
                //This is something we have, so we check if we need it
                size_t location = snpIndex.at(rs);
                bool ambig = false;
                if(snpList.at(location).concordant(token[0], atoi(token[3].c_str()), rs, token[4], token[5], ambig )){
                    if(!m_keepAmbiguous && ambig) m_nAmbig ++; // We don't want to keep ambiguous SNPs and this is an ambiguous SNP
                    else{

                        //This is found in the reference file, so it is in the base set
                        // Now check if we want to keep it based on MAF
                        read = true;
                        Genotype *tempGenotype = new Genotype();
                        size_t indx = 0; //The sample index
                        double oldM=0.0, newM=0.0,oldS=0.0, newS=0.0; // For the calculation of sd and mean
                        size_t alleleCount=0;  // For the calculation of MAF
                        size_t validSamples = 0; // In case there is missingness, this will allow a more accurate calculation of MAF
                        while(indx < m_nRefSample){ // For all possible samples
                            std::bitset<8> b; //Initiate the bit array with maximum size of 8 bit
                            char ch[1];
                            m_bedFile.read(ch,1); //Read the information
                            if (!m_bedFile) throw std::runtime_error("Problem with the BED file...has the FAM/BIM file been changed?");
                            b = ch[0];
                            int c=0;
                            while (c<7 && indx < m_nRefSample ){
                                //Going through the bit flag. Stop when it have read all the samples as the end == NULL
                                //As each bit flag can only have 8 numbers, we need to move to the next bit flag to continue
                                ++indx; //Each sample is represented by 2 bit
                                int first = b[c++]; // Use c, then plus 1 to it
                                int second = b[c++];
                                double value = 0.0;
                                if(first == 1 && second == 0) first = 3; //Missing value should be 3
                                else{
                                    value = first+second+0.0;
                                    alleleCount+= first+second;
                                    validSamples++; //Not missing
                                    if(indx==1){
                                        oldM = newM = value;
                                        oldS = 0.0;
                                    }
                                    else{
                                        newM = oldM + (value-oldM)/(validSamples); //Use valid sample instead
                                        newS = oldS + (value-oldM)*(value-newM);
                                        oldM = newM;
                                        oldS = newS;
                                    }
                                }
                                tempGenotype->AddsampleGenotype(first+second, indx-1); //0 1 2 or 3 where 3 is missing
                            }
                        }

                        double currentMaf = (alleleCount+0.0)/(2.0*validSamples*1.0);
                        currentMaf = (currentMaf > 0.5)? 1.0-currentMaf : currentMaf; // Possible problem of direction??
                        if(m_mafThreshold > currentMaf){ // Does not pass the p-value threshold
                            m_nFilter++;
                            snpList.at(location).setStatus('F'); // Filtered
                        }
                        else{ //This is truly the first SNP that will be included in the analysis
                            snpList.at(location).setFlag(0, true);
                            m_duplicateCheck[rs] = true;
                            snpList.at(location).setStatus('I');
                            m_nSnp++;
                            m_prevChr = token[0]; // This is the first SNP we ever use
                            m_prevLoc = atoi(token[3].c_str());
                            // Also buff the genotypes just so we don't waste the run time
                            m_buffGenotype = new Genotype(*tempGenotype);
                            // Add in the SD and mean information
                            validSamples > 0 ? m_buffGenotype->Setmean(newM) : m_buffGenotype->Setmean(0.0);
                            validSamples > 1 ? m_buffGenotype->SetstandardDeviation(std::sqrt(newS/(validSamples - 1.0))) : m_buffGenotype->SetstandardDeviation(0.0);
                        }
                        break;
                    }
                }
                else{
                    snpList.at(location).setStatus('V'); //This SNP is invalid (discordant)
                    m_nInvalid++;
                }
            }
            // If we can reach here, it means that the last SNP we've got is useless
            // so we need to read the bed file to advance it too
            if(!read){
                size_t indx = 0; //The sample index
                while(indx < m_nRefSample){
                    std::bitset<8> b; //Initiate the bit array
                    char ch[1];
                    m_bedFile.read(ch,1); //Read the information
                    if (!m_bedFile){
                        throw std::runtime_error("Problem with the BED file...has the FAM/BIM file been changed?");
                    }
                    b = ch[0];
                    int c=0;
                    while (c<7 && indx < m_nRefSample ){
                        //Going through the bit flag. Stop when it have read all the samples as the end == NULL
                        //As each bit flag can only have 8 numbers, we need to move to the next bit flag to continue
                        ++indx; //Each sample is represented by 2 bit
                        c+=2; // That's why we increase c by 2
                    }
                }
                read =false;
            }
            // Now we continue to read the samples
        }
        else throw "Malformed bim file, does not contain all necessary information";
    }

    /** From this point onward, we assume prevChr, prevLoc and bufferGenotype contains the last SNP entry **/


    /** Two situation here
     *  1. boundary.size()==0  : Continue to read the files until you find the first SNP that is usable (considering also the MAF)
     *  2. boundary.size()!=0  : Continue to read the files until we find the first SNP that is usable (considering also the MAF)
     *                           Important thing is, if this usable SNP is too far or on a new chromosome, then set finalizeBuff=true
     */

    // Now we start the SNP reading part

/*
    for(size_t i = 0; i < nBlock; ++i){
        getBlock(snpIndex, snpList, genotype, snpLoc, finalizeBuff,completed, boundary);
        if(finalizeBuff){
            return; //Done with this
        }
    }
*/
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
