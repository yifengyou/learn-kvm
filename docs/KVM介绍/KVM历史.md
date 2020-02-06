# KVM历史大事件

* 2005年11月，Intel发布奔腾四处理器第一次正式支持VT技术
* 2006年5月，AMD发布支持AMD-V处理器。
* KVM虚拟机最初是由一个以色列的创业公司Qumranet开发的，作为他们的VDI产品的虚拟机。为了简化开发，KVM的开发人员并没有选择从底层开始新写一个Hypervisor，而是选择了基于Linux kernel，通过加载新的模块从而使Linux Kernel本身变成一个Hypervisor。
* 2006年8月，在先后完成了基本功能、动态迁移以及主要的性能优化之后，Qumranet正式对外宣布了KVM的诞生并推向Linux内核社区。同年10月，KVM模块的源代码被正式接纳进入Linux Kernel，成为内核源代码的一部分。
* 2007年2月发布的Linux 2.6.20是第一个带有KVM模块的Linux内核正式发布版本。
* 2008年9月4日，同内核社区保持着很深渊源的著名Linux发行版提供商—Redhat公司出人意料地出资1亿700百万美金，收购了Qumranet，从而成为了KVM开源项目的新东家，投入较多资源在KVM虚拟化开发中。
* 2010年11月，Redhat公司推出了新的企业版Linux—RHEL 6，在这个发行版中集成了最新的KVM虚拟机，而去掉了在RHEL 5.x系列中集成的Xen。KVM成为RHEL默认的虚拟化方案
