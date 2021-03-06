#ifndef DECOMPOSITION_H
#define DECOMPOSITION_H

#include <iterator>
#include <deque>
#include <list>
#include <stdexcept>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <armadillo>
#include <assert.h>
#include "linkage.h"
#include "snp.h"

class Decomposition
{
    public:
        /** Default constructor */
        Decomposition(size_t thread);
        /** Default destructor */
        virtual ~Decomposition();
        void run(Linkage &linkage, std::deque<size_t> &snpLoc, std::vector<size_t> &boundary, boost::ptr_vector<Snp> &snpList, bool windowEnd, bool decomposeAll, bool starting, boost::ptr_vector<Region> &regionList);
    protected:
    private:
        static size_t check;
        size_t m_thread = 0;
        void decompose(Linkage &linkage, std::deque<size_t> &snpLoc, size_t startDecompIter, size_t endDecompIter, size_t startVarIter, size_t endVarIter, boost::ptr_vector<Snp> &snpList, boost::ptr_vector<Region> &regionList, bool sign, bool start, bool ending);
        void complexSEUpdate(size_t decompStartIndex, size_t decompEndIndex, size_t midStartIndex, size_t midEndIndex, std::deque<size_t> &snpLoc,boost::ptr_vector<Snp> &snpList, boost::ptr_vector<Region> &regionList, bool starting, bool ending, const arma::mat &varResult);

};

#endif // DECOMPOSITION_H
