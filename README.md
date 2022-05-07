# laminarCommCharacterize
Zenodo Concept DOI: [![Concept DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.6526365.svg)](https://doi.org/10.5281/zenodo.6526365)

Characterize Communication Patterns used by Laminar

This is primarily a revision and extension of the core-core benchmark in https://github.com/ucb-cyarp/benchmarking

It is written purely in C to best match the code emitted by Laminar.  It is also simplified.

It focuses on measuring the achieved data rates of the communication style used by Laminar.

## Versions:
**NOTE: Different benchmarking techniques are stored in different branches of the git repository, current versions include:**
  - main: Standard FIFO with `__builtin_memcpy_inline`
  - builtinMemcpyInlined_tmpAligned: Standard FIFO with `__builtin_memcpy_inline` and aligned temporary
  - myMemcpyAligned: Standard FIFO with my implementation of memcpy with aligned vector intrinsics
  - myMemcpyUnalignedWithAlignedTmp: Standard FIFO with my implementation of memcpy with unaligned vector intrinsics but with an aligned temporary
  - myNonTemporalMemcpyAligned: FIFO using my non-temporal implementation of memcpy
  - myNonTemporalMemcpyAligned-seperateLoadStore: FIFO using my non-temporal implementation of memcpy with seperate functions for load and store to take advantage of elements stored in cache

*Note that duty cycle branches are unfinished.* 

**NOTE to Zenodo Users:** Archives are stored with the branch name appended to the version number.  Use the concept DOI [10.5281/zenodo.6526365](https://doi.org/10.5281/zenodo.6526365) and look for the version you are interested in.

## Citing This Software:
If you would like to reference this software, please cite Christopher Yarp's Ph.D. thesis.

*At the time of writing, the GitHub CFF parser does not properly generate thesis citations.  Please see the bibtex entry below.*

```bibtex
@phdthesis{yarp_phd_2022,
	title = {High Speed Software Radio on General Purpose CPUs},
	school = {University of California, Berkeley},
	author = {Yarp, Christopher},
	year = {2022},
}
```
