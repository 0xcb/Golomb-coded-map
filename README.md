#Golomb-coded map

The Golomb-coded map (GCM) is a [smaller](#smaller), [slower](#slower) alternative to the better-known Bloom filter. This version also has the virtue of [associativity](#associative), whence the designation *map*, rather than *set*.

Implemented from the description of a Golomb-compressed set on pgs. 7-8 of [Cache-, Hash- and Space-Efficient Bloom filters](http://algo2.iti.kit.edu/singler/publications/cacheefficientbloomfilters-wea2007.pdf). Works under Visual Studio and GCC.

## <a name="smaller"/> How much smaller?

That depends on the false positive chance.

The information-theoretic minimum number of bits needed to represent ![equation](http://latex.codecogs.com/gif.latex?n) patterns with false positive chance ![equation](http://latex.codecogs.com/gif.latex?%5Cvarepsilon) is ![equation](http://latex.codecogs.com/gif.latex?n%5Clog_2%7B%5E1%2F_%5Cvarepsilon). A Bloom filter, however, requires ![equation](http://latex.codecogs.com/gif.latex?n%5Clog_2%7B%5E1%2F_%5Cvarepsilon%7D%5Clog_%7B2%7De) bits, a penalty just north of 44%. The GCM, on the other hand, requires roughly ![equation](http://latex.codecogs.com/gif.latex?n%5Clog_2%7B%5E1%2F_%5Cvarepsilon%7D%2B1.5) bits. The difference in size is therefore contingent on the false positive chance, which determines whether adding 1.5 or multiplying by 1.44 is greater.

With a false positive chance above 9.5%, give or take<sup>1</sup>, the GCM is larger than a Bloom filter because adding 1.5 results in a larger value than multiplying by 1.44. But the smaller the false positive chance, the more the GCM approaches the information-theoretic minimum. At a false positive rate of 1 in 2<sup>40</sup> (false positive chance of 0.00000000000091%), for example, the Bloom filter needs 57.71 bits per entry, while the GCM needs only 41.5.

1. For completeness' sake:  ![equation](http://latex.codecogs.com/gif.latex?%5Cvarepsilon%3D%5E1%2F_%7B2%5E%5Cfrac%7B1.5%7D%7B0.4427%7D%7D), or 9.55%.

## <a name="slower"/> How much slower?

The GCM stores patterns as coded integers. In order to be queried, these integers have to be not only decoded, but decoded *in order*. This is obviously inefficient — the complexity is ![equation](http://latex.codecogs.com/gif.latex?O%28%5En%2F_2%29) — so it is useful to divide the map into ![equation](http://latex.codecogs.com/gif.latex?i) bins, assign each entry ![equation](http://latex.codecogs.com/gif.latex?h) to a bin according to ![equation](http://latex.codecogs.com/gif.latex?%5Clfloor%5Eh%2F_%5Cfrac%7Bn%7D%7Bi%7D%5Crfloor), and record the bit offset of the first item in each bin in the corresponding index of a lookup table. The complexity is then ![equation](http://latex.codecogs.com/gif.latex?O%28%5En%2F_%7B2i%7D%29).

Making GCM queries faster is thus straightforwardly a matter of increasing ![equation](http://latex.codecogs.com/gif.latex?i), but of course increasing ![equation](http://latex.codecogs.com/gif.latex?i) also increases the size of the GCM (at the modest rate of 8 bytes per — 4 bytes for the lookup table and 4 bytes for associativity. This is the only cost for associativity and can be eliminated if not needed). Between ![equation](http://latex.codecogs.com/gif.latex?%5En%2F_{75}) and ![equation](http://latex.codecogs.com/gif.latex?%5En%2F_{20}) are effective values for ![equation](http://latex.codecogs.com/gif.latex?i), but a Bloom filter will still be 2-3x faster. To tune ![equation](http://latex.codecogs.com/gif.latex?i), use the `table_size` constructor parameter.

## <a name="associative"/> How can it be used associatively?

Easily! Note the parameter `uint32_t* index` in `gc_map_query`. On a successful query, `index` will point to the position of the entry in the map and can be used to associate data with that entry. How you do so is entirely up to you, but indexing into either an array or hash table is bound to make the most sense.
