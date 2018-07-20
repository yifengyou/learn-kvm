<!-- TOC depthFrom:1 depthTo:6 withLinks:1 updateOnSave:1 orderedList:0 -->

- [Qemu-KVM基本用法](#qemu-kvm基本用法)
	- [Qemu命令基本格式](#qemu命令基本格式)
	- [Qemu标准选项](#qemu标准选项)
		- [qemu -name选项](#qemu-name选项)
		- [qemu -M选项](#qemu-m选项)
		- [qemu -cpu model选项](#qemu-cpu-model选项)
		- [qemu -smp选项](#qemu-smp选项)
		- [qemu -numa选项](#qemu-numa选项)
		- [qemu -name选项](#qemu-name选项)
		- [qemu -name选项](#qemu-name选项)
		- [Qemu-CPU配置](#qemu-cpu配置)
		- [Qemu-内存配置](#qemu-内存配置)
		- [Qemu-存储器配置](#qemu-存储器配置)
		- [Qemu-网络配置](#qemu-网络配置)
		- [Qemu-图形界面配置](#qemu-图形界面配置)
	- [END](#end)

<!-- /TOC -->
# Qemu-KVM基本用法

## Qemu命令基本格式

Qemu基本格式

```
qemu-kvm [options] [disk_image]
```

获取Qemu版本

```
root@android:~/qemu-kvm# qemu-system-x86_64 --version
QEMU emulator version 2.11.1(Debian 1:2.11+dfsg-1ubuntu7.4)
Copyright (c) 2003-2017 Fabrice Bellard and the QEMU Project developers
```
选项众多
* 标准选项
* USB选项
* 显示选项
* 网络选项
* 字符设备选项
* 蓝牙选项
* Linux引导专用选项
* 调试/砖家模式选项
* PowerPC专用选项
* Sparc32专用选项
* i386平台专用选项

## Qemu标准选项

### qemu -name选项

-name:执行虚拟机名字，可重复，不一定唯一，仅为人标记

### qemu -M选项

* -M machine:指定要模拟的主机类型

```
root@android:~/qemu-kvm# qemu-system-x86_64 -M ?     
Supported machines are:
pc-i440fx-zesty      Ubuntu 17.04 PC (i440FX + PIIX, 1996)
pc-i440fx-yakkety    Ubuntu 16.10 PC (i440FX + PIIX, 1996)
pc-i440fx-xenial     Ubuntu 16.04 PC (i440FX + PIIX, 1996)
pc-i440fx-wily       Ubuntu 15.04 PC (i440FX + PIIX, 1996)
pc-i440fx-trusty     Ubuntu 14.04 PC (i440FX + PIIX, 1996)
ubuntu               Ubuntu 18.04 PC (i440FX + PIIX, 1996) (alias of pc-i440fx-bionic)
pc-i440fx-bionic     Ubuntu 18.04 PC (i440FX + PIIX, 1996) (default)
pc-i440fx-bionic-hpb Ubuntu 18.04 PC (i440FX + PIIX, +host-phys-bits=true, 1996)
pc-i440fx-artful     Ubuntu 17.10 PC (i440FX + PIIX, 1996)
pc-i440fx-2.9        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.8        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.7        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.6        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.5        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.4        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.3        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.2        Standard PC (i440FX + PIIX, 1996)
pc                   Standard PC (i440FX + PIIX, 1996) (alias of pc-i440fx-2.11)
pc-i440fx-2.11       Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.10       Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.1        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-2.0        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-1.7        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-1.6        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-1.5        Standard PC (i440FX + PIIX, 1996)
pc-i440fx-1.4        Standard PC (i440FX + PIIX, 1996)
pc-1.3               Standard PC (i440FX + PIIX, 1996)
pc-1.2               Standard PC (i440FX + PIIX, 1996)
pc-1.1               Standard PC (i440FX + PIIX, 1996)
pc-1.0               Standard PC (i440FX + PIIX, 1996)
pc-0.15              Standard PC (i440FX + PIIX, 1996)
pc-0.14              Standard PC (i440FX + PIIX, 1996)
pc-0.13              Standard PC (i440FX + PIIX, 1996)
pc-0.12              Standard PC (i440FX + PIIX, 1996)
pc-0.11              Standard PC (i440FX + PIIX, 1996)
pc-0.10              Standard PC (i440FX + PIIX, 1996)
pc-q35-zesty         Ubuntu 17.04 PC (Q35 + ICH9, 2009)
pc-q35-yakkety       Ubuntu 16.10 PC (Q35 + ICH9, 2009)
pc-q35-xenial        Ubuntu 16.04 PC (Q35 + ICH9, 2009)
pc-q35-bionic        Ubuntu 18.04 PC (Q35 + ICH9, 2009)
pc-q35-bionic-hpb    Ubuntu 18.04 PC (Q35 + ICH9, +host-phys-bits=true, 2009)
pc-q35-artful        Ubuntu 17.10 PC (Q35 + ICH9, 2009)
pc-q35-2.9           Standard PC (Q35 + ICH9, 2009)
pc-q35-2.8           Standard PC (Q35 + ICH9, 2009)
pc-q35-2.7           Standard PC (Q35 + ICH9, 2009)
pc-q35-2.6           Standard PC (Q35 + ICH9, 2009)
pc-q35-2.5           Standard PC (Q35 + ICH9, 2009)
pc-q35-2.4           Standard PC (Q35 + ICH9, 2009)
q35                  Standard PC (Q35 + ICH9, 2009) (alias of pc-q35-2.11)
pc-q35-2.11          Standard PC (Q35 + ICH9, 2009)
pc-q35-2.10          Standard PC (Q35 + ICH9, 2009)
isapc                ISA-only PC
none                 empty machine
xenfv                Xen Fully-virtualized PC
xenpv                Xen Para-virtualized PC
```

### qemu -cpu model选项

指定CPU模型

```
root@android:~/qemu-kvm# qemu-system-x86_64 -cpu ?
Available CPUs:
x86              486                                                  
x86   Broadwell-IBRS  Intel Core Processor (Broadwell, IBRS)          
x86 Broadwell-noTSX-IBRS  Intel Core Processor (Broadwell, no TSX, IBRS)  
x86  Broadwell-noTSX  Intel Core Processor (Broadwell, no TSX)        
x86        Broadwell  Intel Core Processor (Broadwell)                
x86           Conroe  Intel Celeron_4x0 (Conroe/Merom Class Core 2)   
x86        EPYC-IBPB  AMD EPYC Processor (with IBPB)                  
x86             EPYC  AMD EPYC Processor                              
x86     Haswell-IBRS  Intel Core Processor (Haswell, IBRS)            
x86 Haswell-noTSX-IBRS  Intel Core Processor (Haswell, no TSX, IBRS)    
x86    Haswell-noTSX  Intel Core Processor (Haswell, no TSX)          
x86          Haswell  Intel Core Processor (Haswell)                  
x86   IvyBridge-IBRS  Intel Xeon E3-12xx v2 (Ivy Bridge, IBRS)        
x86        IvyBridge  Intel Xeon E3-12xx v2 (Ivy Bridge)              
x86     Nehalem-IBRS  Intel Core i7 9xx (Nehalem Core i7, IBRS update)
x86          Nehalem  Intel Core i7 9xx (Nehalem Class Core i7)       
x86       Opteron_G1  AMD Opteron 240 (Gen 1 Class Opteron)           
x86       Opteron_G2  AMD Opteron 22xx (Gen 2 Class Opteron)          
x86       Opteron_G3  AMD Opteron 23xx (Gen 3 Class Opteron)          
x86       Opteron_G4  AMD Opteron 62xx class CPU                      
x86       Opteron_G5  AMD Opteron 63xx class CPU                      
x86           Penryn  Intel Core 2 Duo P9xxx (Penryn Class Core 2)    
x86 SandyBridge-IBRS  Intel Xeon E312xx (Sandy Bridge, IBRS update)   
x86      SandyBridge  Intel Xeon E312xx (Sandy Bridge)                
x86 Skylake-Client-IBRS  Intel Core Processor (Skylake, IBRS)            
x86   Skylake-Client  Intel Core Processor (Skylake)                  
x86 Skylake-Server-IBRS  Intel Xeon Processor (Skylake, IBRS)            
x86   Skylake-Server  Intel Xeon Processor (Skylake)                  
x86    Westmere-IBRS  Westmere E56xx/L56xx/X56xx (IBRS update)        
x86         Westmere  Westmere E56xx/L56xx/X56xx (Nehalem-C)          
x86           athlon  QEMU Virtual CPU version 2.5+                   
x86         core2duo  Intel(R) Core(TM)2 Duo CPU     T7700  @ 2.40GHz
x86          coreduo  Genuine Intel(R) CPU           T2600  @ 2.16GHz
x86            kvm32  Common 32-bit KVM processor                     
x86            kvm64  Common KVM processor                            
x86             n270  Intel(R) Atom(TM) CPU N270   @ 1.60GHz          
x86          pentium                                                  
x86         pentium2                                                  
x86         pentium3                                                  
x86           phenom  AMD Phenom(tm) 9550 Quad-Core Processor         
x86           qemu32  QEMU Virtual CPU version 2.5+                   
x86           qemu64  QEMU Virtual CPU version 2.5+                   
x86             base  base CPU model type with no features enabled    
x86             host  KVM processor with all supported host features (only available in KVM mode)
x86              max  Enables all features supported by the accelerator in the current host

Recognized CPUID flags:
  fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 pn clflush ds acpi mmx fxsr sse sse2 ss ht tm ia64 pbe
  pni pclmulqdq dtes64 monitor ds-cpl vmx smx est tm2 ssse3 cid fma cx16 xtpr pdcm pcid dca sse4.1 sse4.2 x2apic movbe popcnt tsc-deadline aes xsave osxsave avx f16c rdrand hypervisor
  fsgsbase tsc-adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm mpx avx512f avx512dq rdseed adx smap avx512ifma pcommit clflushopt clwb avx512pf avx512er avx512cd sha-ni avx512bw avx512vl
  avx512vbmi umip pku ospke avx512vbmi2 gfni vaes vpclmulqdq avx512vnni avx512bitalg avx512-vpopcntdq la57 rdpid
  avx512-4vnniw avx512-4fmaps spec-ctrl ssbd
  syscall nx mmxext fxsr-opt pdpe1gb rdtscp lm 3dnowext 3dnow
  lahf-lm cmp-legacy svm extapic cr8legacy abm sse4a misalignsse 3dnowprefetch osvw ibs xop skinit wdt lwp fma4 tce nodeid-msr tbm topoext perfctr-core perfctr-nb
  invtsc
  ibpb virt-ssbd
  xstore xstore-en xcrypt xcrypt-en ace2 ace2-en phe phe-en pmm pmm-en
  kvmclock kvm-nopiodelay kvm-mmu kvmclock kvm-asyncpf kvm-steal-time kvm-pv-eoi kvm-pv-unhalt kvm-pv-tlb-flush kvmclock-stable-bit



  npt lbrv svm-lock nrip-save tsc-scale vmcb-clean flushbyasid decodeassists pause-filter pfthreshold
  xsaveopt xsavec xgetbv1 xsaves
  arat
```

### qemu -smp选项

SMP的全称是"对称多处理"（Symmetrical Multi-Processing）技术，是指在一个计算机上汇集了一组处理器(多CPU),各CPU之间共享内存子系统以及总线结构。

![1532077356807.png](image/1532077356807.png)

```
-smp [cpus=]n[,cores=cores][,threads=threads][,sockets=sockets][,maxcpus=maxcpus]
```
* PC机上最多模拟255个CPU
* maxcpus用于指定热插入CPU个数


### qemu -numa选项

```
NUMA（Non Uniform Memory Access Architecture）技术可以使众多服务器像单一系统那样运转，同时保留小系统便于编程和管理的优点。基于电子商务应用对内存访问提出的更高的要求，NUMA也向复杂的结构设计提出了挑战。
```

### qemu -name选项
### qemu -name选项










### Qemu-CPU配置












### Qemu-内存配置





### Qemu-存储器配置


### Qemu-网络配置

### Qemu-图形界面配置



## END
