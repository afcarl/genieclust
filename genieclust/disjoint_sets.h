/*  class DisjointSets
 *
 *  Copyright (C) 2018-2019 Marek.Gagolewski.com
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __disjoint_sets_h
#define __disjoint_sets_h

#include <stdexcept>
#include <algorithm>
#include <vector>


typedef unsigned long long ulonglong;



/*! Disjoint Sets (Union-Find) Data Structure
 *
 *   A class to represent partitions of the set {0,1,...,n-1} for any n.
 *
 *   Path compression for find() is implemented,
 *   but the union() operation is naive (neither
 *   it is union by rank nor by size),
 *   see https://en.wikipedia.org/wiki/Disjoint-set_data_structure.
 *   This is by design, as some other operations in the current
 *   package rely on the assumption that the parent id of each
 *   element is always <= than itself.
 */
class DisjointSets {

protected:
    ulonglong n;                //!< number of distinct elements
    ulonglong k;                //!< number of subsets
    std::vector<ulonglong> par; /*!< par[i] is the id of the parent 
                                 *   of the i-th element
                                 */

public:
    /*!  Starts with a "weak" partition {  {0}, {1}, ..., {n-1}  },
     *   i.e., n singletons.
     * 
     *   @param n number of elements, n>=0.
     */
    DisjointSets(ulonglong n) :
        par(n)
    {
        // if (n < 0) throw std::domain_error("n < 0");
        this->n = n;
        this->k = n;
        for (ulonglong i=0; i<n; ++i)
            this->par[i] = i;
    }


    /*! A nullary constructor allows Cython to allocate
     *  the instances on the stack. Do not use otherwise.
    */
    DisjointSets() : DisjointSets(0) { }


    /*! Returns the current number of sets in the partition.
     */
    ulonglong get_k() const { return this->k; }


    /*! Returns the total cardinality of the set being partitioned.
     */
    ulonglong get_n() const { return this->n; }


    /*! Finds the subset id for a given x.
     *
     *  @param x a value in {0,...,n-1}
     */
    ulonglong find(ulonglong x) {
        if (x < 0 || x >= this->n) throw std::domain_error("x not in [0,n)");

        if (this->par[x] != x) {
            this->par[x] = this->find(this->par[x]);
        }
        return this->par[x];
    }


    /*!  Merges the sets containing x and y.
     *
     *   Let px be the parent id of x, and py be the parent id of y.
     *   If px < py, then the new parent id of py will be set to py.
     *   Otherwise, px will have py as its parent.
     *
     *   If x and y are already members of the same subset, 
     *   an exception is thrown.
     *
     *   @return the id of the parent of x or y, whichever is smaller.
     * 
     *   @param x a value in {0,...,n-1}
     *   @param y a value in {0,...,n-1}
     */
    virtual ulonglong merge(ulonglong x, ulonglong y) { // well, union is a reserved C++ keyword :)
        x = this->find(x); // includes a range check for x
        y = this->find(y); // includes a range check for y
        if (x == y) throw std::invalid_argument("find(x) == find(y)");
        if (y < x) std::swap(x, y);

        this->par[y] = x;
        this->k -= 1;

        return x;
    }

};

#endif
