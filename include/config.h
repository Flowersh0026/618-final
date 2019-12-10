#ifndef _CONFIG_H_
#define _CONFIG_H_

// Cache line size is obtained from `$ getconf LEVEL1_DCACHE_LINESIZE`
#define CACHELINE_SIZE 64

#ifdef ENABLE_CACHELINE_ALIGNMENT
#define ALIGNED alignas(CACHELINE_SIZE)
#else
#define ALIGNED
#endif

#endif  // _CONFIG_H_
