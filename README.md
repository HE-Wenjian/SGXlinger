SGXlinger: A New Side-channel Attack Vector Based on Interrupt Latency against Enclave Execution
========

This repository contains the source code of SGXlinger -- the interrupt latency attack framework accompanying our ICCD 2018 paper.

> Wenjian He, Wei Zhang, Sanjeev Das and Yang Liu. SGXlinger: A New Side-channel Attack Vector Based on Interrupt Latency against Enclave Execution. In Proceedings of the 36th IEEE International Conference on Computer Design (ICCD'18).
> 
> PDF (to be appeared) | [PPT](https://github.com/HE-Wenjian/SGXlinger/raw/master/ICCD_PPT.pdf)

## Abstract

Software Guard Extension (SGX) is a new security feature that has been released in recent Intel commodity processors. It is designed to provide a user program with a strongly shielded environment against other components in the system, including the OS, firmware and hardware peripherals. With SGX, developers can securely deploy critical applications on untrusted remote platforms without the concern of information leakage. However, researchers have found several attacks against SGX, promoting the need for a comprehensive study on the security property of SGX. In this paper, we discover a new attack vector SGXlinger to disclose information inside the protected program. Our attack monitors the interrupt latency of the SGX-protected program. The evaluation shows we can learn coarse-grained information inside the shielded environment. In an experimental setting, we measure that the information leakage rate of the proposed side-channel can reach up to 35 Kbps.


## Building and Running

#### Prerequisites

1. An SGX-capable machine. Ensure SGX is enabled in BIOS.
1. OS: Ubuntu 16.04 LTS
1. Install the SGX driver, SGX SDK and SGX PSW.


### 1. Generate and Install the SGXlinger kernel

1. To manipulate the LAPIC and have accurate measurements of interrupt delay, SGXlinger instruments the kernel. The SGXlinger kernel is based on the Ubuntu kernel version `Ubuntu-hwe-4.8.0-52.55_16.04.1`. All the modifications are described in the patch file `sgxlinger_kernel.patch`. To use our patch, please download the original Ubuntu kernel source code and then proceed to apply the patch, as shown below.

    ```bash
    $ git clone git://kernel.ubuntu.com/ubuntu/ubuntu-xenial.git
    $ cd ubuntu-xenial
    $ git checkout -b SGXlingerKernel Ubuntu-hwe-4.8.0-52.55_16.04.1
    $ git apply ../sgxlinger_kernel.patch
    ```

1. Compile the modified kernel:
    ```bash
    $ fakeroot debian/rules clean
    $ fakeroot debian/rules binary-headers binary-generic
    ```

1. Install the modified kernel and reboot to it.
    ```bash
    $ cd ..
    $ sudo dpkg -i linux-*.deb
    $ sudo reboot
    ```

1. After rebooting, you can use command `$ uname -a` to check if you have booted to a correct kernel. If you use the patch in this repository, the command should display:

    ```
    $ uname -a
    Linux [PC Name] 4.8.0-52-generic #55~16.04.1~SGXlinger SMP [Time] x86_64 x86_64 x86_64 GNU/Linux
    ```


### 2. Build and Load the SGXlinger Loadable Kernel Module

The SGXlinger loadable kernel module bridges the code in the kernel and the malicious privileged attacker. To build and load the SGXlinger kernel module,
```bash
$ cd kernel_module
$ make
$ sudo insmod sgxlinger.ko core_id=3
```
Change `core_id` to the core you want to attack.


### 3. Use SGXlinger Loadable Kernel Module

The SGXlinger loadable kernel module exposes the runtime interface using the debugfs filesystem. You will be required to be a super user to view the debugfs directories. To navigate to the SGXlinger folder, 

```bash
$ sudo -i
$ cd /sys/kernel/debug/sgxlinger/
```

In the folder, you will see the following files.

| File Name      | Permission | Description  |
|-----|:-----:|------| 
| `enabled`        | RW | When set to `1`, indicates the latency monitoring is enabled; When set to `0`, the monitoring is disabled.
| `deadline_delta` | RW | Read/configure the SGXlinger interrupt interval. <br>When `enabled=1`, the LAPIC raises an interrupt every `deadline_delta` clock cycles. *WARN: Setting this value too small may cause kernel panic.*
| `data_pos`       | RW | Read: report how many measurements are stored in `monitor_data`; <br>Write: write `1` to reset the internal buffer.
| `monitor_data`   | R  | This file stores the SGXlinger measurements in binary form. <br>See `retrieve_sgxlinger_data/retrieve_data.c` in this repository to learn how to read and parse the data.


#### Usage example

Try the the demo in the folder `victim_demo`. To run the demo,

```bash
$ cd victim_demo
$ ./run_demo.sh
```


