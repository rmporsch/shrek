#include <iostream>

#include <stdlib.h>

#include <stdio.h>

#include <stdint.h>

#include <math.h>

#include <cstring>

#include <string>

#include <bitset>

#include <cstdlib>

#include <limits.h>

#include <fstream>

#include <stdexcept>

#include <vector>



//typedef __m128i mlong;

typedef unsigned int mlong;



class Genotype

{

	public:

		Genotype();

		virtual ~Genotype();

        void GetbothR(const Genotype *snpB, const bool correction, double &r, double &rSq) const;

        static void SetsampleNum(size_t sampleNum);

        void AddsampleGenotype(const int first, const int second, const size_t sampleIndex);

	protected:

	private:

        mlong *m_genotype;

        mlong *m_missing;

        size_t m_nonMissSample=0;

        static size_t m_sampleNum;

        unsigned int m_bitSize;

        unsigned int m_requiredBit;

        //const static mlong m1 = 0x5555555555555555LLU;
		const static mlong FIVEMASK = 0x55555555;

        //const static mlong m2 = 0x3333333333333333LLU;
		const static mlong m2 = 0x33333333;
		const static mlong AAAAMASK = 0xaaaaaaaa;

        //const static mlong m4 = 0x0f0f0f0f0f0f0f0fLLU;

        //const static mlong m8 = 0x00ff00ff00ff00ffLLU;



};



size_t Genotype::m_sampleNum;

void Genotype::GetbothR(const Genotype *snpB, const bool correction, double &r, double &rSq) const{
    size_t range = (m_requiredBit /(m_bitSize))+1;
    mlong loader1, loader2, sum1, sum2, sum11, sum12, sum22;
	uint32_t final_sum1 = 0;
	uint32_t final_sum2 = 0;
	uint32_t final_sum11 = 0;
	uint32_t final_sum22 = 0;
	uint32_t final_sum12 = 0;
	double return_vals[5];
    return_vals[0] = (double)m_sampleNum;
    return_vals[1] = -(double)snpB->m_nonMissSample;
    return_vals[2] = -(double)m_nonMissSample;
    return_vals[3] = return_vals[1];
    return_vals[4] = return_vals[2];
    unsigned int N =0;
    for(size_t i = 0; i < range;){
        loader1 = m_genotype[i];
        loader2 = snpB->m_genotype[i];
        sum1 = snpB->m_missing[i];
        sum2 = m_missing[i];
		i++;
		N+= __builtin_popcountll(sum1&sum2)/2;
		sum12 = (loader1 | loader2) & FIVEMASK;
		sum1 = sum1 & loader1;
		sum2 = sum2 & loader2;
		loader1 = (loader1 ^ loader2) & (AAAAMASK - sum12);
		sum12 = sum12 | loader1;
		sum11 = sum1 & FIVEMASK;
		sum22 = sum2 & FIVEMASK;
		sum1 = (sum1 & 0x33333333) + ((sum1 >> 2) & 0x33333333);
		sum2 = (sum2 & 0x33333333) + ((sum2 >> 2) & 0x33333333);
		sum12 = (sum12 & 0x33333333) + ((sum12 >> 2) & 0x33333333);
		mlong tmp_sum1=0 , tmp_sum2=0;
		if(i < range){
	    	loader1 = m_genotype[i];
	    	loader2 = snpB->m_genotype[i];
	    	tmp_sum1 = snpB->m_missing[i];
	    	tmp_sum2 =m_missing[i];
			N+= __builtin_popcountll(tmp_sum1&tmp_sum2)/2;
		}
		else{
			loader1 = 0;
			loader2 = 0;
		}
	    i++;
	    mlong tmp_sum12 = (loader1 | loader2) & FIVEMASK;
		tmp_sum1 = tmp_sum1 & loader1;
		tmp_sum2 = tmp_sum2 & loader2;
		loader1 = (loader1 ^ loader2) & (AAAAMASK - tmp_sum12);
		tmp_sum12 = tmp_sum12 | loader1;
		sum11 += tmp_sum1 & FIVEMASK;
		sum22 += tmp_sum2 & FIVEMASK;
		sum1 += (tmp_sum1 & 0x33333333) + ((tmp_sum1 >> 2) & 0x33333333);
		sum2 += (tmp_sum2 & 0x33333333) + ((tmp_sum2 >> 2) & 0x33333333);
		sum12 += (tmp_sum12 & 0x33333333) + ((tmp_sum12 >> 2) & 0x33333333);
	    if(i < range){
			loader1 = m_genotype[i];
			loader2 = snpB->m_genotype[i];
			tmp_sum1 = snpB->m_missing[i];
			tmp_sum2 =m_missing[i];
			N+= __builtin_popcountll(tmp_sum1&tmp_sum2)/2;
		}
		else{
			loader1=0;
			loader2=0;
			tmp_sum1=0;
			tmp_sum2=0;
		}
		i++;
		tmp_sum12 = (loader1 | loader2) & FIVEMASK;
		tmp_sum1 = tmp_sum1 & loader1;
		tmp_sum2 = tmp_sum2 & loader2;
		loader1 = (loader1 ^ loader2) & (AAAAMASK - tmp_sum12);
		tmp_sum12 = tmp_sum12 | loader1;
		sum11 += tmp_sum1 & FIVEMASK;
		sum22 += tmp_sum2 & FIVEMASK;
		sum1 += (tmp_sum1 & 0x33333333) + ((tmp_sum1 >> 2) & 0x33333333);
		sum2 += (tmp_sum2 & 0x33333333) + ((tmp_sum2 >> 2) & 0x33333333);
		sum11 = (sum11 & 0x33333333) + ((sum11 >> 2) & 0x33333333);
		sum22 = (sum22 & 0x33333333) + ((sum22 >> 2) & 0x33333333);
		sum12 += (tmp_sum12 & 0x33333333) + ((tmp_sum12 >> 2) & 0x33333333);
		sum1 = (sum1 & 0x0f0f0f0f) + ((sum1 >> 4) & 0x0f0f0f0f);
		sum2 = (sum2 & 0x0f0f0f0f) + ((sum2 >> 4) & 0x0f0f0f0f);
		sum11 = (sum11 & 0x0f0f0f0f) + ((sum11 >> 4) & 0x0f0f0f0f);
		sum22 = (sum22 & 0x0f0f0f0f) + ((sum22 >> 4) & 0x0f0f0f0f);
		sum12 = (sum12 & 0x0f0f0f0f) + ((sum12 >> 4) & 0x0f0f0f0f);
		final_sum1 += (sum1 * 0x01010101) >> 24;
		final_sum2 += (sum2 * 0x01010101) >> 24;
		final_sum11 += (sum11 * 0x01010101) >> 24;
		final_sum22 += (sum22 * 0x01010101) >> 24;
		final_sum12 += (sum12 * 0x01010101) >> 24;
	}

	return_vals[0] -= final_sum12;
	return_vals[1] += final_sum1;
	return_vals[2] += final_sum2;
	return_vals[3] += final_sum11;
	return_vals[4] += final_sum22;

    double dxx = return_vals[1];
    double dyy = return_vals[2];
    double n = N;
    double cov12 = return_vals[0] * n - dxx * dyy;
    dxx = (return_vals[3] * n + dxx * dxx) * (return_vals[4] * n + dyy * dyy);
    r=cov12 / sqrt(dxx);
    rSq =(cov12 * cov12) / dxx;

}



void Genotype::SetsampleNum(size_t sampleNum){

    if(sampleNum < 2){

        throw "ERROR! Sample number must be bigger than 1!";

    }

    Genotype::m_sampleNum = sampleNum;

}

Genotype::Genotype(){

    m_bitSize = sizeof(mlong)*CHAR_BIT;

    m_requiredBit = Genotype::m_sampleNum*2;

    m_genotype = new mlong [(m_requiredBit /(m_bitSize))+1];

    m_missing = new mlong [(m_requiredBit /(m_bitSize))+1];

    memset(m_genotype, 0x0,((m_requiredBit /(m_bitSize))+1)*sizeof(mlong));

    memset(m_missing, 0x0,((m_requiredBit /(m_bitSize))+1)*sizeof(mlong));

}



Genotype::~Genotype(){

    delete [] m_genotype;

    delete [] m_missing;

}





void Genotype::AddsampleGenotype(const int first, const int second, const size_t sampleIndex){

    if(first==second && first==1){ //hom ref 10

        m_missing[(sampleIndex*2)/(m_bitSize)] = m_missing[(sampleIndex*2)/(m_bitSize)]  | 0x3LLU << ((sampleIndex*2)% (m_bitSize));

        m_genotype[(sampleIndex*2)/(m_bitSize)] = m_genotype[(sampleIndex*2)/(m_bitSize)]  | 0x2LLU << ((sampleIndex*2)% (m_bitSize));

        m_nonMissSample++;

    }

    else if(first==second && first==0){//hom alt 00

        m_missing[(sampleIndex*2)/(m_bitSize)] = m_missing[(sampleIndex*2)/(m_bitSize)]  | 0x3LLU << ((sampleIndex*2)% (m_bitSize));

        m_nonMissSample++;

    }

    else if(first==0 && second==1){ //Het 01

        m_missing[(sampleIndex*2)/(m_bitSize)] = m_missing[(sampleIndex*2)/(m_bitSize)]  | 0x3LLU << ((sampleIndex*2)% (m_bitSize));

        m_genotype[(sampleIndex*2)/(m_bitSize)] = m_genotype[(sampleIndex*2)/(m_bitSize)]  | 0x1LLU << ((sampleIndex*2)% (m_bitSize));

        m_nonMissSample++;

    }

    else if(first==1 && second ==0){ //missing

        m_genotype[(sampleIndex*2)/(m_bitSize)] = m_genotype[(sampleIndex*2)/(m_bitSize)]  | 0x1LLU << ((sampleIndex*2)% (m_bitSize));

    }

}



std::string clean(const std::string seq) {

    // clean out the special characters at the end of the sequence

    if(seq.empty()) return "";

    size_t j = seq.length()-1;

    while ((j>=0) && ((seq.at(j)<'!') || (seq.at(j)>='~')))

        j--;



    if (j<0) return "";

    else return seq.substr(0,j+1);

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









int main(int argc, char *argv[]){

    std::string prefix = argv[1];

    std::string famName = prefix+".fam";

    std::ifstream famFile;

    famFile.open(famName.c_str());

    if(!famFile.is_open()){

        std::cerr << "Cannot open fam file" << std::endl;

        return EXIT_FAILURE;

    }

    std::string line;

    size_t numSample = 0;

    while(std::getline(famFile, line)){

        if(!clean(line).empty()) numSample++;

    }

    famFile.close();

    Genotype::SetsampleNum(numSample);

    std::string bimName = prefix+".bim";

    std::ifstream bimFile;

    bimFile.open(bimName.c_str());

    if(!bimFile.is_open()){

        std::cerr << "Cannot open bim file" << std::endl;

    }

    size_t numSnp =0;

    while(std::getline(bimFile, line)){

        if(!clean(line).empty()) numSnp++;

    }



    std::string bedName = prefix+".bed";

    std::ifstream bedFile;

    bool snpMajor = openPlinkBinaryFile(bedName, bedFile);

    std::vector<Genotype*> genotype;

    for(size_t i = 0; i < numSnp; ++i){

        Genotype *tempGenotype = new Genotype();

        size_t indx = 0; //The iterative count

        while ( indx < numSample ){

            std::bitset<8> b; //Initiate the bit arrays

            char ch[1]; // This is to read the bit

            bedFile.read(ch,1); //Read the information

            if (!bedFile) throw std::runtime_error("Problem with the BED file...has the FAM/BIM file been changed?");

            b = ch[0];

            int c=0;

            while (c<7 && indx < numSample ){

                ++indx;

                int first = b[c++];

                int second = b[c++];

                if(!(first == 1 && second == 0)){

                    int genoRep=0;

                    if(first==0 && second==0) genoRep=-1;

                    else if(first==0 && second==1) genoRep=0;

                    else if(first==1 && second==1) genoRep=1;

                    double value = (double)(genoRep);

                }

                tempGenotype->AddsampleGenotype(first,second, indx-1); //0 1 2 or 3 where 3 is missing

            }

        }

        genotype.push_back(tempGenotype);

    }

    for(size_t i = 0; i < genotype.size(); ++i){

        for(size_t j = 0; j < i; ++j) std::cerr << "0 ";

        for(size_t j = i+1; j < genotype.size(); ++j){

            double r, r2;

            genotype[i]->GetbothR(genotype[j], false, r, r2);

            std::cerr << r2 << " ";

        }

        std::cerr << std::endl;

    }

    return 0;

}
