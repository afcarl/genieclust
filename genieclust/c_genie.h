/*  The Genie+ Clustering Algorithm
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


#ifndef __c_genie_h
#define __c_genie_h

#include <stdexcept>
#include <algorithm>
#include <vector>
#include "c_gini_disjoint_sets.h"


/*! Compute the degree of each vertex in an undirected graph
 * over vertex set {0,...,n-1}
 *
 * Edges with ind[2*i+0]<0 or ind[2*i+1]<0 are purposedly ignored
 *
 * @param ind array of size nind*2, giving the edges' definition
 * @param num_edges number of edges
 * @param n number of vertices
 * @param deg [out] array of size n
 */
void Cget_graph_node_degrees(
    const ssize_t* ind,
    ssize_t num_edges,
    ssize_t n,
    ssize_t* deg)
{
    for (ssize_t i=0; i<n; ++i)
        deg[i] = 0;

    for (ssize_t i=0; i<num_edges; ++i) {
        ssize_t u = ind[2*i+0];
        ssize_t v = ind[2*i+1];
        if (u<0 || v<0)
            continue; // represents a no-edge → ignore
        if (u>=n || v>=n)
            throw std::domain_error("Detected an element not in {0, ..., n-1}");
        if (u == v)
            throw std::domain_error("Self-loops are not allowed");

        deg[u]++;
        deg[v]++;
    }
}


/*!  The Genie+ Clustering Algorithm
 *
 *   Gagolewski M., Bartoszuk M., Cena A.,
 *   Genie: A new, fast, and outlier-resistant hierarchical clustering algorithm,
 *   Information Sciences 363, 2016, pp. 8-23. doi:10.1016/j.ins.2016.05.003
 *
 *   A new hierarchical clustering linkage criterion: the Genie algorithm
 *   links two clusters in such a way that a chosen economic inequity measure
 *   (here, the Gini index) of the cluster sizes does not increase drastically
 *   above a given threshold. Benchmarks indicate a high practical
 *   usefulness of the introduced method: it most often outperforms
 *   the Ward or average linkage, k-means, spectral clustering,
 *   DBSCAN, Birch, and others in terms of the clustering
 *   quality while retaining the single linkage speed.
 *
 *   This is a new implementation of the O(n sqrt(n))-time version
 *   of the original algorithm. Additionally, MST leaves can be
 *   marked as noise points (if `noise_leaves==True`). This is useful,
 *   if the Genie algorithm is applied on the MST with respect to
 *   the HDBSCAN-like mutual reachability distance.
 */
template <class T>
class CGenie {
protected:
    T* mst_d;         //<! n-1 weights
    ssize_t* mst_i;   //<! n-1 edges of the MST (given by (n-1)*2 indices)
    ssize_t n;        //<! number of points
    bool noise_leaves;//<! mark leaves as noise points?

    std::vector<ssize_t> deg; //<! deg[i] denotes the degree of the i-th vertex

    ssize_t noise_count; //<! now many noise points are there (leaves)
    std::vector<ssize_t> denoise_index; //<! which noise point is it?
    std::vector<ssize_t> denoise_index_rev; //!< reverse look-up for denoise_index


    // When the Genie correction is on, some MST edges will be chosen
    // in non-consecutive order. An array-based skiplist will speed up
    // searching within the not-yet-consumed edges.
    // Also, if there are noise points, then the skiplist allows the algorithm
    // to naturally ignore edges that connect the leaves.
    std::vector<ssize_t> next_edge; //<! skip-list of non-yet-visited edges...
    std::vector<ssize_t> prev_edge; //<! ...a doubly linked list it is.
    ssize_t curidx;
    ssize_t lastidx;

    CGiniDisjointSets ds;


    /*! Initializes curidx, lastidx, next_edge, and prev_edge */
    void skiplist_init() {
        if (noise_leaves) {
            // start with a list that skips all edges that lead to noise points
            curidx = -1;
            lastidx = -1;
            for (ssize_t i=0; i<n-1; ++i) {
                ssize_t i1 = mst_i[2*i+0];
                ssize_t i2 = mst_i[2*i+1];
                if (deg[i1] > 1 && deg[i2] > 1) {
                    // no-leaves, i.e., 2 non-noise points
                    if (curidx < 0) {
                        curidx = i; // the first non-leaf edge
                        prev_edge[i] = -1;
                    }
                    else {
                        next_edge[lastidx] = i;
                        prev_edge[i] = lastidx;
                    }
                    lastidx = i;
                }
            }

            next_edge[lastidx] = n-1;
            lastidx = curidx; // first non-leaf
        }
        else {
            // no noise leaves
            curidx  = 0;
            lastidx = 0;
            for (ssize_t i=0; i<n-1; ++i) {
                next_edge[i] = i+1;
                prev_edge[i] = i-1;
            }
        }
    }


    void do_genie(ssize_t n_clusters, double gini_threshold)
    {
        if (n-noise_count-n_clusters <= 0) {
            throw std::runtime_error("The requested number of clusters is too large \
                with this many detected noise points");
        }

        ds = CGiniDisjointSets(n-noise_count);

        ssize_t lastm = 0; // last minimal cluster size
        for (ssize_t i=0; i<n-noise_count-n_clusters; ++i) {

            // determine the pair of vertices to merge
            ssize_t i1;
            ssize_t i2;

            if (ds.get_gini() > gini_threshold) {
                // the Genie correction for inequity of cluster sizes
                ssize_t m = ds.get_smallest_count();
                if (m != lastm || lastidx < curidx)
                    lastidx = curidx;
                //assert 0 <= lastidx < n-1

                while (ds.get_count(denoise_index_rev[mst_i[2*lastidx+0]]) != m
                    && ds.get_count(denoise_index_rev[mst_i[2*lastidx+1]]) != m)
                {
                    lastidx = next_edge[lastidx];
                    //assert 0 <= lastidx < n-1
                }

                i1 = mst_i[2*lastidx+0];
                i2 = mst_i[2*lastidx+1];

                //assert lastidx >= curidx
                if (lastidx == curidx) {
                    curidx = next_edge[curidx];
                    lastidx = curidx;
                }
                else {
                    ssize_t previdx;
                    previdx = prev_edge[lastidx];
                    lastidx = next_edge[lastidx];
                    //assert 0 <= previdx
                    //assert previdx < lastidx
                    //assert lastidx < n
                    next_edge[previdx] = lastidx;
                    prev_edge[lastidx] = previdx;
                }
                lastm = m;
            }
            else { // single linkage-like
                // assert 0 <= curidx < n-1
                i1 = mst_i[2*curidx+0];
                i2 = mst_i[2*curidx+1];
                curidx = next_edge[curidx];
            }

            ds.merge(denoise_index_rev[i1], denoise_index_rev[i2]);
        }
    }

    /*! Propagate res with clustering results
     *
     * @param res [out] array of length n
     */
    void get_labels(int* res) {
        std::vector<int> res_cluster_id(n, -1);
        int c = 0;
        for (ssize_t i=0; i<n; ++i) {
            if (denoise_index_rev[i] >= 0) {
                // a non-noise point
                ssize_t j = denoise_index[ds.find(denoise_index_rev[i])];
                // assert 0 <= j < n
                if (res_cluster_id[j] < 0) {
                    res_cluster_id[j] = c;
                    ++c;
                }
                res[i] = res_cluster_id[j];
            }
            else {
                // a noise point
                res[i] = -1;
            }
        }
    }


public:
    CGenie(T* mst_d, ssize_t* mst_i, ssize_t n, bool noise_leaves)
        : deg(n), denoise_index(n), denoise_index_rev(n),
        next_edge(n), prev_edge(n)
    {
        this->mst_d = mst_d;
        this->mst_i = mst_i;
        this->n = n;
        this->noise_leaves = noise_leaves;

        for (ssize_t i=1; i<n-1; ++i)
            if (mst_d[i-1] > mst_d[i])
                throw std::domain_error("mst_d unsorted");

        // set up this->deg:
        Cget_graph_node_degrees(mst_i, n-1, n, this->deg.data());

        // Create the non-noise points' translation table (for GiniDisjointSets)
        // and count the number of noise points
        if (noise_leaves) {
            this->noise_count = 0;
            ssize_t j = 0;
            for (ssize_t i=0; i<n; ++i) {
                if (deg[i] == 1) { // a leaf
                    ++noise_count;
                    denoise_index_rev[i] = -1;
                }
                else {             // a non-leaf
                    denoise_index[j] = i;
                    denoise_index_rev[i] = j;
                    ++j;
                }
            }
            if (!(noise_count >= 2))
                throw std::runtime_error("ASSERT FAIL (noise_count >= 2)");
            if (!(j + noise_count == n))
                throw std::runtime_error("ASSERT FAIL (j + noise_count == n)");
        }
        else { // there are no noise points
            this->noise_count = 0;
            for (ssize_t i=0; i<n; ++i) {
                denoise_index[i]     = i; // identity
                denoise_index_rev[i] = i;
            }
        }
    }

    CGenie() : CGenie(NULL, NULL, 0, false) { }


    /*! Run the Genie+ algorithm
     *
     * @param n_clusters number of clusters to find
     * @param gini_threshold the Gini index threshold
     * @param res [out] array of length n, will give cluster labels
     */
    void apply_genie(ssize_t n_clusters, double gini_threshold, int* res) {
        skiplist_init();
        do_genie(n_clusters, gini_threshold);
        get_labels(res);
    }
};


#endif