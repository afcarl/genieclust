/*  Adjusted- and Nonadjusted Rand Score,
 *  Adjusted- and Nonadjusted Fowlkes-Mallows Score,
 *  Adjusted-, Normalised and Nonadjusted Mutual Information Score
 *  (for vectors of "small" ints)
 *
 *  References
 *  ==========
 *
 *  Hubert L., Arabie P., Comparing Partitions,
 *  Journal of Classification 2(1), 1985, pp. 193-218, esp. Eqs. (2) and (4)
 *
 *  Vinh N.X., Epps J., Bailey J.,
 *  Information theoretic measures for clusterings comparison:
 *  Variants, properties, normalization and correction for chance,
 *  Journal of Machine Learning Research 11, 2010, pp. 2837-2854.
 *
 *
 *  Copyright (C) 2018-2020 Marek Gagolewski (https://www.gagolewski.com)
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


#ifndef __c_compare_partitions_h
#define __c_compare_partitions_h

#include "c_common.h"
#include <algorithm>
#include <cmath>
#include <vector>


/*! (t choose 2)
 *
 * @param t
 * @return t*(t-1.0)*0.5
 */
inline double Ccomb2(double t)
{
    return t*(t-1.0)*0.5;
}


/*! Computes both the minimum and the maximum in an array
 *
 * @param x c_contiguous array of length n
 * @param n length of x
 * @param xmin [out] the minimum of x
 * @param xmax [out] the maximum of x
 */
template<class T>
void Cminmax(const T* x, ssize_t n, T* xmin, T* xmax)
{
    *xmin = x[0];
    *xmax = x[0];

    for (ssize_t i=1; i<n; ++i) {
        if      (x[i] < *xmin) *xmin = x[i];
        else if (x[i] > *xmax) *xmax = x[i];
    }
}



/*!
 * Stores AR, FM and MI scores as well as their adjusted/normalised versions.
 */
struct CComparePartitionsResult {
    double ar;
    double r;
    double fm;
    double afm;
    double mi;
    double nmi;
    double ami;
};


/*! Applies partial pivoting to a given confusion matrix - permutes the columns
 *  so as to have the largest elements in each row on the main diagonal.
 *
 *  This comes in handy whenever C actually summarises the results generated
 *  by clustering algorithms, where actual label values do not matter.
 *
 *
 *
 * @param C [in/out] a c_contiguous confusion matrix of size xc*yc
 * @param xc number of rows in C
 * @param yc number of columns in C
 *
 * Note that C is modified in-place (overwritten).
 */
void Capply_pivoting(ssize_t* C, ssize_t xc, ssize_t yc)
{
    for (ssize_t i=0; i<std::min(xc-1, yc-1); ++i) {
        ssize_t w = i;
        for (ssize_t j=i+1; j<yc; ++j) {
            // find w = argmax C[i,w], w=i,i+1,...yc-1
            if (C[i*yc+w] < C[i*yc+j]) w = j;
        }
        for (ssize_t j=0; j<xc; ++j) {
            // swap columns i and w
            std::swap(C[j*yc+i], C[j*yc+w]);
        }
    }
}


/*! Computes the confusion matrix (as a dense matrix) - a 2-way contingency table
 *
 * @param C [out] a c_contiguous matrix of size xc*yc
 *      where C[i-xmin,j-ymin] is the number of k such that x[k]==i,y[k]==j;
 * @param xc number of rows in C
 * @param yc number of columns in C
 * @param xmin the minimum of x
 * @param ymin the minimum of y
 * @param x,y c_contiguous arrays of length n with x[i], y[i] being integers
 * in [xmin, xmin+xc) and [ymin, ymin+yc), respectively,
 * denoting the class/cluster of the i-th observation
 * @param n length of the two arrays
 *
 * The elements in C are modified in-place.
 */
void Ccontingency_table(ssize_t* C, ssize_t xc, ssize_t yc,
        ssize_t xmin, ssize_t ymin,
        ssize_t* x, ssize_t* y, ssize_t n)
{
    for (ssize_t j=0; j<xc*yc; ++j)
        C[j] = 0;

    for (ssize_t i=0; i<n; ++i)
        C[(x[i]-xmin)*yc +(y[i]-ymin)]++;
}


/*! Computes the adjusted and nonadjusted Rand- and FM scores
 *  based on a given confusion matrix.
 *
 *  References
 *  ==========
 *
 *  Hubert L., Arabie P., Comparing Partitions,
 *  Journal of Classification 2(1), 1985, pp. 193-218, esp. Eqs. (2) and (4)
 *
 *  Vinh N.X., Epps J., Bailey J.,
 *  Information theoretic measures for clusterings comparison:
 *  Variants, properties, normalization and correction for chance,
 *  Journal of Machine Learning Research 11, 2010, pp. 2837-2854.
 *
 *  @param C a c_contiguous confusion matrix of size xc*yc
 *  @param xc number of rows in C
 *  @param yc number of columns in C
 *
 *  @return the computed scores
 */
CComparePartitionsResult Ccompare_partitions(const ssize_t* C, ssize_t xc, ssize_t yc)
{
    ssize_t n = 0; // total sum (length of the underlying x and y = number of points)
    for (ssize_t ij=0; ij<xc*yc; ++ij)
        n += C[ij];

    std::vector<double> sum_x(xc);
    std::vector<double> sum_y(yc);

    double sum_comb_x = 0.0, sum_comb_y = 0.0, sum_comb = 0.0;
    double h_x = 0.0, h_y = 0.0, h_x_cond_y = 0.0, h_x_y = 0.0;

    for (ssize_t i=0; i<xc; ++i) {
        double t = 0.0;
        for (ssize_t j=0; j<yc; ++j) {
            if (C[i*yc+j] > 0) h_x_y += C[i*yc+j]*log((double)C[i*yc+j]/(double)n);
            t += C[i*yc+j];
            sum_comb += Ccomb2(C[i*yc+j]);
        }
        sum_comb_x += Ccomb2(t);
        sum_x[i] = t;
        if (t > 0) h_y += t*log(t/(double)n);
    }

    for (ssize_t j=0; j<yc; ++j) {
        double t = 0.0;
        for (ssize_t i=0; i<xc; ++i) {
            if (C[i*yc+j] > 0) h_x_cond_y += C[i*yc+j]*log(C[i*yc+j]/sum_x[i]);
            t += C[i*yc+j];
        }
        sum_comb_y += Ccomb2(t);
        sum_y[j] = t;
        if (t > 0) h_x += t*log(t/(double)n);
    }

    h_x = -h_x/(double)n;
    h_y = -h_y/(double)n;
    h_x_cond_y = -h_x_cond_y/(double)n;
    h_x_y = -h_x_y/(double)n;

    double prod_comb = (sum_comb_x*sum_comb_y)/n/(n-1.0)*2.0; // expected sum_comb,
                                        // see Eq.(2) in (Hubert, Arabie, 1985)
    double mean_comb = (sum_comb_x+sum_comb_y)*0.5;
    double e_fm = prod_comb/sqrt(sum_comb_x*sum_comb_y); // expected FM (variant)

    double e_mi = 0.0;
    for (ssize_t i=0; i<xc; ++i) {
        double fac0 = lgamma(sum_x[i]+1.0)+lgamma(n-sum_x[i]+1.0)-lgamma(n+1.0);
        for (ssize_t j=0; j<yc; ++j) {
            double fac1 = log(n/sum_x[i]/sum_y[j]);
            double fac2 = fac0+lgamma(sum_y[j]+1.0)+lgamma(n-sum_y[j]+1.0);

            for (ssize_t nij=std::max(1.0, sum_x[i]+sum_y[j]-n);
                        nij<=std::min(sum_x[i],sum_y[j]); nij++) {
                double fac3 = fac2;
                fac3 -= lgamma(nij+1.0);
                fac3 -= lgamma(sum_x[i]-nij+1.0);
                fac3 -= lgamma(sum_y[j]-nij+1.0);
                fac3 -= lgamma(n-sum_x[i]-sum_y[j]+nij+1.0);
                e_mi += nij*(fac1+log(nij))*exp(fac3);
            }
        }
    }
    e_mi = e_mi/(double)n;

    CComparePartitionsResult res;
    res.ar  = (sum_comb-prod_comb)/(mean_comb-prod_comb);
    res.r   = 1.0 + (2.0*sum_comb - (sum_comb_x+sum_comb_y))/n/(n-1.0)*2.0;
    res.fm  = sum_comb/sqrt(sum_comb_x*sum_comb_y);
    res.afm = (res.fm - e_fm)/(1.0 - e_fm); // Eq.(4) in (Hubert, Arabie, 1985)
    res.mi = h_x-h_x_cond_y;
    res.nmi = res.mi/(0.5*(h_x+h_y)); // NMI_sum in (Vinh et al., 2010)
    res.ami = (res.mi - e_mi)/(0.5*(h_x+h_y) - e_mi); // AMI_sum in (Vinh et al., 2010)

    return res;
}

#endif
