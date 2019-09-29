#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define IS_DEBUG 1
#else
#define IS_DEBUG 0
#endif

#define DPRINTF if(IS_DEBUG)printf

#endif
