# distutils: language=c++
# cython: boundscheck=False
# cython: cdivision=True
# cython: nonecheck=False
# cython: wraparound=False
# cython: language_level=3


"""The Genie+ clustering algorithm (with extras)

Copyright (C) 2018-2020 Marek Gagolewski (https://www.gagolewski.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

cimport cython
cimport numpy as np
import numpy as np

from . cimport c_gini_disjoint_sets
from . cimport c_genie


ctypedef fused floatT:
    float
    double




cpdef np.ndarray[ssize_t] get_graph_node_degrees(ssize_t[:,::1] ind, ssize_t n):
    """Given an adjacency list representing an undirected simple graph over
    vertex set {0,...,n-1}, return an array deg with deg[i] denoting
    the degree of the i-th vertex. For instance, deg[i]==1 marks a leaf node.


    Parameters
    ----------

    ind : ndarray, shape (m,2)
        A 2-column matrix such that {ind[i,0], ind[i,1]} represents
        one of m undirected edges. Negative indexes are ignored.
    n : int
        Number of vertices.


    Returns
    -------

    deg : ndarray, shape(n,)
        An integer array of length n.
    """
    cdef ssize_t num_edges = ind.shape[0]
    assert ind.shape[1] == 2
    cdef np.ndarray[ssize_t] deg = np.empty(n, dtype=np.intp)

    c_genie.Cget_graph_node_degrees(&ind[0,0], num_edges, n, &deg[0])

    return deg




#############################################################################
# The Genie+ Clustering Algorithm (internal)
#############################################################################

cpdef np.ndarray[int] genie_from_mst(
        floatT[::1] mst_d,
        ssize_t[:,::1] mst_i,
        ssize_t n_clusters=2,
        double gini_threshold=0.3,
        bint noise_leaves=False):
    """Compute a k-partition based on a precomputed MST.

    The Genie+ Clustering Algorithm (with extensions)

    Gagolewski M., Bartoszuk M., Cena A.,
    Genie: A new, fast, and outlier-resistant hierarchical clustering algorithm,
    Information Sciences 363, 2016, pp. 8-23. doi:10.1016/j.ins.2016.05.003

    A new hierarchical clustering linkage criterion: the Genie algorithm
    links two clusters in such a way that a chosen economic inequity measure
    (here, the Gini index) of the cluster sizes does not increase drastically
    above a given threshold. Benchmarks indicate a high practical
    usefulness of the introduced method: it most often outperforms
    the Ward or average linkage, k-means, spectral clustering,
    DBSCAN, Birch, and others in terms of the clustering
    quality while retaining the single linkage speed.

    This is a new implementation of the O(n sqrt(n))-time version
    of the original algorithm. Additionally, MST leaves can be
    marked as noise points (if `noise_leaves==True`). This is useful,
    if the Genie algorithm is applied on the MST with respect to
    the HDBSCAN-like mutual reachability distance.


    If gini_threshold==1.0 and noise_leaves==False, then basically this
    is the single linkage algorithm. Set gini_threshold==1.0 and
    noise_leaves==True to get a HDBSCAN-like behavior (and make sure
    the MST is computed w.r.t. the mutual reachability distance).


    Parameters
    ----------

    mst_d, mst_i : ndarray
        Minimal spanning tree defined by a pair (mst_i, mst_d),
        see genieclust.mst.
    n_clusters : int, default=2
        Number of clusters the data is split into.
    gini_threshold : float, default=0.3
        The threshold for the Genie correction
    noise_leaves : bool
        Mark leaves as noise;
        Prevents forming singleton-clusters.


    Returns
    -------

    labels_ : ndarray, shape (n,)
        Predicted labels, representing a partition of X.
        labels_[i] gives the cluster id of the i-th input point.
        If noise_leaves==True, then label -1 denotes a noise point.
    """
    cdef ssize_t n = mst_i.shape[0]+1

    if not 1 <= n_clusters <= n:
        raise ValueError("incorrect n_clusters")
    if not n-1 == mst_d.shape[0]:
        raise ValueError("ill-defined MST")
    if not 0.0 <= gini_threshold <= 1.0:
        raise ValueError("incorrect gini_threshold")

    cdef np.ndarray[ssize_t] res = np.empty(n, dtype=np.intp)
    cdef c_genie.CGenie[floatT] g
    g = c_genie.CGenie[floatT](&mst_d[0], &mst_i[0,0], n, noise_leaves)
    g.apply_genie(n_clusters, gini_threshold, &res[0])

    return res



#############################################################################
# The Genie+Information Criterion (G+IC) Clustering Algorithm
# (experimental, under construction)
#############################################################################

cpdef np.ndarray[int] gic_from_mst(
        floatT[::1] mst_d,
        ssize_t[:,::1] mst_i,
        double n_features,
        ssize_t n_clusters=2,
        ssize_t add_clusters=0,
        double[::1] gini_thresholds=None,
        bint noise_leaves=False):
    """Compute a k-partition based on a pre-computed MST.

    The Genie+Information Criterion (G+IC)
    Clustering Algorithm (experimental edition)
    by Anna Cena

    TODO: describe


    References
    ==========

    [1] Cena A., Adaptive hierarchical clustering algorithms based on
    data aggregation methods, PhD Thesis, Systems Research Institute,
    Polish Academy of Sciences 2018.

    [2] Mueller A., Nowozin S., Lampert C.H., Information Theoretic
    Clustering using Minimum Spanning Trees, DAGM-OAGM 2012.


    Parameters
    ----------

    mst_d, mst_i : ndarray
        Minimal spanning tree defined by a pair (mst_i, mst_d),
        see genieclust.mst.
    n_features : double
        number of features in the data set
        [can be fractional if you know what you're doing]
    n_clusters : int, default=2
        Number of clusters the data is split into.
    add_clusters: int, default=0
        Number of additional clusters to work with internally.
    gini_thresholds : ndarray or None for the default
        TODO: describe
        if of length 0, add_clusters is ignored and the procedure
        starts from a weak clustering (all are singletons) == Agglomerative-IC (ICA)
    noise_leaves : bool
        Mark leaves as noise;
        Prevents forming singleton-clusters.


    Returns
    -------

    labels_ : ndarray, shape (n,)
        Predicted labels, representing a partition of X.
        labels_[i] gives the cluster id of the i-th input point.
        If noise_leaves==True, then label -1 denotes a noise point.
    """
    cdef ssize_t n = mst_i.shape[0]+1

    if not 1 <= n_clusters <= n:
        raise ValueError("incorrect n_clusters")
    if not n-1 == mst_d.shape[0]:
        raise ValueError("ill-defined MST")

    if gini_thresholds is None:
        gini_thresholds = np.r_[0.3, 0.5, 0.7]

    cdef np.ndarray[ssize_t] res = np.empty(n, dtype=np.intp)
    cdef c_genie.CGenie[floatT] g
    g = c_genie.CGenie[floatT](&mst_d[0], &mst_i[0,0], n, noise_leaves)
    g.apply_gic(n_clusters, add_clusters, n_features,
        &gini_thresholds[0], gini_thresholds.shape[0], &res[0])

    return res
