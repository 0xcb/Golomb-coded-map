# Golomb-coded map

The Golomb-coded map (GCM) is a [smaller](#smaller), [slower](#slower) alternative to the better-known Bloom filter. This version also has the virtue of [associativity](#associative), whence the designation *map*, rather than *set*.

Implemented from the description of a Golomb-compressed set on pgs. 7-8 of [Cache-, Hash- and Space-Efficient Bloom filters](http://algo2.iti.kit.edu/singler/publications/cacheefficientbloomfilters-wea2007.pdf). Works under Visual Studio and GCC.

## <a name="smaller"/> How much smaller?

That depends on the false positive chance.

The information-theoretic minimum number of bits needed to represent $n$ patterns with false positive chance $\epsilon$ is $n\log_2{\frac{1}{\epsilon}}$. A Bloom filter, however, requires approximately $n\log_2{\frac{1}{\epsilon}}\log_{2}e$ bits, a penalty just north of 44%. The GCM, on the other hand, needs $n\left(\log_2{\frac{1}{\epsilon}}+\frac{e}{e-1}\right)$ bits. The difference in size is therefore contingent on the false positive chance, which determines whether adding $\frac{e}{e-1}$ or multiplying by $\log_{2}e$ is greater.

With a false positive chance above 8.4%, give or take<sup>1</sup>, the GCM is larger than a Bloom filter because adding $\frac{e}{e-1}$ results in a larger value than multiplying by $\log_{2}e$. But the smaller the false positive chance, the more the GCM approaches the information-theoretic minimum. At a false positive rate of 1 in 2<sup>40</sup> (false positive chance of 0.00000000000091%), for example, the Bloom filter needs 57.71 bits per entry, while the GCM needs only 41.59.

1. $\left(^\frac{e}{e-1}/_{\log_{2}e}\right)^{-1}\approx 0.084$

## <a name="slower"/> How much slower?

The GCM stores patterns as coded integers. In order to be queried, these integers have to be not only decoded, but decoded *in order*. This is obviously inefficient — the average number of probes is $\frac{n}{2}$ — so it is useful to divide the map into $i$ bins, assign each entry $h$ to a bin according to $\lfloor ^h/_\frac{n}{i} \rfloor$, and record the bit offset of the first item in each bin in the corresponding index of a lookup table. The average number of probes is then $^n/_{2i}$.

Making GCM queries faster is thus straightforwardly a matter of increasing $i$, but of course increasing $i$ also increases the size of the GCM (at the modest rate of 8 bytes per — 4 bytes for the lookup table and 4 bytes for associativity. This is the only cost for associativity and can be eliminated if not needed). Between $10$ and $40$ are effective values for $i$, but decoding hasn't been thoroughly optimized and a Bloom filter may still be marginally faster. To tune $i$, use the `table_size` constructor parameter.

## <a name="associative"/> How can it be used associatively?

Easily! Note the parameter `uint32_t* index` in `gc_map_query`. On a successful query, `index` will point to the position of the entry in the map and can be used to associate data with that entry. How you do so is entirely up to you, but indexing into either an array or hash table is bound to make the most sense.
