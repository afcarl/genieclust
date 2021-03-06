

---
title: "Genieclust Tutorial"
subtitle: "Plotting Dendrograms"
author: "Marek Gagolewski"
---


> **This is a draft of the tutorial distributed
> in the hope that it will be useful.**

The `genieclust` package generates auxiliary data
that allows for constructing linkage matrices
compatible with those used in
[`scipy.cluster.hierarchy`](https://docs.scipy.org/doc/scipy/reference/cluster.hierarchy.html).






```python
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import os.path
import genieclust
```

# Example 1


Create toy dataset:


```python
X = np.array(
    [[0, 0], [0, 1], [1, 0],
     [0, 4], [0, 3], [1, 4],
     [4, 0], [3, 0], [4, 1],
     [4, 4], [3, 4], [4, 3]])
```

Apply Genie:


```python
g = genieclust.Genie(n_clusters=4)
labels = g.fit_predict(X)
```

Create the linkage matrix, see
[`scipy.cluster.hierarchy.linkage`](https://docs.scipy.org/doc/scipy/reference/generated/scipy.cluster.hierarchy.linkage.html):


```python
# create the linkage matrix, see
Z = np.column_stack((g.children_, g.distances_, g.counts_))
print(Z)
```

Plot the dataset and the dendrogram


```python
plt.rcParams["figure.figsize"] = (8,4)
import scipy.cluster.hierarchy
plt.subplot(121)
genieclust.plots.plot_scatter(X, labels=labels)
plt.axis("equal")
```

```python
plt.subplot(122)
scipy.cluster.hierarchy.dendrogram(Z)
```

```python
plt.show()
```

![plot of chunk X4](dendrogram-figures/X4-1.png)


# Example 2



Load data:


```python
path = os.path.join("..", "benchmark_data")
dataset = "jain"

# Load an example 2D dataset
X = np.loadtxt(os.path.join(path, "%s.data.gz" % dataset), ndmin=2)

# Load the corresponding reference labels.
# The original labels are in {1,2,..,k} and 0 denotes the noise cluster.
# Let's make them more Python-ish by subtracting 1
# (and hence the noise cluster will be marked as -1).
labels_true = np.loadtxt(os.path.join(path, "%s.labels0.gz" % dataset), dtype=np.intp)-1
n_clusters = len(np.unique(labels_true))-(np.min(labels_true)==-1)
# do not count the "noise" cluster (if present) as a separate entity

# Centre and scale (proportionally in all the axes) all the points.
# Note: this is NOT a standardization of all the variables.
X = ((X-X.mean(axis=0))/X.std(axis=None, ddof=1))
```

Apply Genie:


```python
g = genieclust.Genie(n_clusters=n_clusters)
labels = g.fit_predict(X)
print(labels)
```

Draw the true and detected labels:


```python
plt.rcParams["figure.figsize"] = (8,4)
plt.subplot("121")
genieclust.plots.plot_scatter(X, labels=labels_true)
plt.title("%s (n=%d, true n_clusters=%d)"%(dataset, X.shape[0], n_clusters))
plt.axis("equal")
```

```python
plt.subplot("122")
genieclust.plots.plot_scatter(X, labels=labels)
plt.title("%s Genie g=%g"%(dataset, g.gini_threshold))
plt.axis("equal")
```

```python
plt.show()
```

![plot of chunk jain3](dendrogram-figures/jain3-1.png)

Draw the dendrogram with `scipy`:


```python
# create the linkage matrix, see scipy.cluster.hierarchy.linkage
Z = np.column_stack((g.children_, g.distances_, g.counts_))
# correct for possible departures from ultrametricity:
Z[:,2] = genieclust.tools.cummin(Z[::-1,2])[::-1]
import scipy.cluster.hierarchy
scipy.cluster.hierarchy.dendrogram(Z)
```

```python
plt.show()
```

![plot of chunk jain4](dendrogram-figures/jain4-1.png)


