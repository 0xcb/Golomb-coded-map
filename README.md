# Golomb-coded map

The Golomb-coded map (GCM) is a [smaller](#smaller), [slower](#slower) alternative to the better-known Bloom filter. This version also has the virtue of [associativity](#associative), whence the designation *map*, rather than *set*.

Implemented from the description of a Golomb-compressed set on pgs. 7-8 of [Cache-, Hash- and Space-Efficient Bloom filters](http://algo2.iti.kit.edu/singler/publications/cacheefficientbloomfilters-wea2007.pdf). Works under Visual Studio and GCC.

## <a name="smaller"/> How much smaller?

That depends on the false positive chance.

The information-theoretic minimum number of bits needed to represent ![equation](http://latex.codecogs.com/gif.latex?n) patterns with false positive chance ![equation](http://latex.codecogs.com/gif.latex?%5Cepsilon) is ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20n%5Clog_2%7B%5E1%2F_%5Cepsilon). A Bloom filter, however, requires approximately ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20n%5Clog_2%7B%5E1%2F_%5Cepsilon%7D%5Clog_%7B2%7De) bits, a penalty just north of 44%. The GCM, on the other hand, needs ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20n%5C%28%5Clog_2%7B%5E1%2F_%5Cepsilon%7D%2B%281-%5Csqrt%5B%5Cepsilon%5D%7B1-%5Cepsilon%7D%29%5E%7B-1%7D%5C%29) bits. The difference in size is therefore contingent on the false positive chance, which determines whether adding ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%281-%5Csqrt%5B%5Cepsilon%5D%7B1-%5Cepsilon%7D%29%5E%7B-1%7D) or multiplying by ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5Clog_%7B2%7De) is greater.

With a false positive chance above 8.3%, give or take<sup>1</sup>, the GCM is larger than a Bloom filter because adding ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%281-%5Csqrt%5B%5Cepsilon%5D%7B1-%5Cepsilon%7D%29%5E%7B-1%7D) results in a larger value than multiplying by ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5Clog_%7B2%7De). But the smaller the false positive chance, the more the GCM approaches the information-theoretic minimum. At a false positive rate of 1 in 2<sup>40</sup> (false positive chance of 0.00000000000091%), for example, the Bloom filter needs 57.71 bits per entry, while the GCM needs only 41.59.

1. For completeness' sake: ![equation](http://latex.codecogs.com/png.latex?%5Cinline%20%5Cepsilon%20%3D%20%5Cleft%20%28%202%5E%7B%5Cfrac%7B%281-%5Csqrt%5B%5Cepsilon%5D%7B1-%5Cepsilon%20%7D%29%5E%7B-1%7D%7D%7B%5Clog_2%28e%29-1%7D%7D%20%5Cright%20%29%5E%7B-1%7D%20%5Capprox%200.083)

## <a name="slower"/> How much slower?

The GCM stores patterns as coded integers. In order to be queried, these integers have to be not only decoded, but decoded *in order*. This is obviously inefficient — the average number of probes is ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5En%2F_2) — so it is useful to divide the map into ![equation](http://latex.codecogs.com/gif.latex?i) bins, assign each entry ![equation](http://latex.codecogs.com/gif.latex?h) to a bin according to ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5Clfloor%5Eh%2F_%5Cfrac%7Bn%7D%7Bi%7D%5Crfloor), and record the bit offset of the first item in each bin in the corresponding index of a lookup table. The average number of probes is then ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5En%2F_%7B2i%7D).

Making GCM queries faster is thus straightforwardly a matter of increasing ![equation](http://latex.codecogs.com/gif.latex?i), but of course increasing ![equation](http://latex.codecogs.com/gif.latex?i) also increases the size of the GCM (at the modest rate of 8 bytes per — 4 bytes for the lookup table and 4 bytes for associativity. This is the only cost for associativity and can be eliminated if not needed). Between ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5En%2F_{75}) and ![equation](http://latex.codecogs.com/gif.latex?%5Cinline%20%5En%2F_{20}) are effective values for ![equation](http://latex.codecogs.com/gif.latex?i), but a Bloom filter will still be marginally faster. To tune ![equation](http://latex.codecogs.com/gif.latex?i), use the `table_size` constructor parameter.

## <a name="associative"/> How can it be used associatively?

Easily! Note the parameter `uint32_t* index` in `gc_map_query`. On a successful query, `index` will point to the position of the entry in the map and can be used to associate data with that entry. How you do so is entirely up to you, but indexing into either an array or hash table is bound to make the most sense.
