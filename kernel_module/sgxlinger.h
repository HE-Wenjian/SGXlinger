#ifndef H_SGXLINGER_HEADER_
#define H_SGXLINGER_HEADER_

#include <linux/types.h>

struct SGXlingerDataEntry{
	__u64 interrupt_timestamp;
	__u16 interrupt_delay;
} __attribute__ ((packed));

#define ALLOC_1M     (1024*1024)
#define ALLOC_16M    (  16*ALLOC_1M)
#define ALLOC_128M   ( 128*ALLOC_1M)
#define ALLOC_256M   ( 256*ALLOC_1M)
#define ALLOC_512M   ( 512*ALLOC_1M)
#define ALLOC_1G     (1024*ALLOC_1M)
#define ALLOC_AMOUNT (ALLOC_128M)

static const size_t TSC_BUF_SIZE=ALLOC_AMOUNT/sizeof(struct SGXlingerDataEntry);

static inline __u64 rdtscp( void )
{
    __u64 rax,rdx;
    asm volatile ( "rdtscp\n" : "=a" (rax), "=d" (rdx) : :"%ecx");
    return (rdx << 32) + rax;
}

#endif

