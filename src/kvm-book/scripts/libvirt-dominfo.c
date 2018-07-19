/**
 * Get domain information via libvirt C API.
 * Tested with python 2.6 and libvirt-devel-0.9.10 on a RHEL 6.3 host system.
 */

#include <stdio.h>
#include <libvirt/libvirt.h>

int getDomainInfo(int id) {
	virConnectPtr conn = NULL; /* the hypervisor connection */
	virDomainPtr dom = NULL;   /* the domain being checked */
	virDomainInfo info;        /* the information being fetched */

	/* NULL means connect to local QEMU/KVM hypervisor */
	conn = virConnectOpenReadOnly(NULL);
	if (conn == NULL) {
		fprintf(stderr, "Failed to connect to hypervisor\n");
		return 1;
	}

	/* Find the domain by its ID */
	dom = virDomainLookupByID(conn, id);
	if (dom == NULL) {
		fprintf(stderr, "Failed to find Domain %d\n", id);
		virConnectClose(conn);
		return 1;
	}

	/* Get virDomainInfo structure of the domain */
	if (virDomainGetInfo(dom, &info) < 0) {
		fprintf(stderr, "Failed to get information for Domain %d\n", id);
		virDomainFree(dom);
		virConnectClose(conn);
		return 1;
	}

	/* Print some info of the domain */
	printf("Domain ID: %d\n", id);
	printf("    vCPUs: %d\n", info.nrVirtCpu);
	printf("   maxMem: %d KB\n", info.maxMem);
	printf("   memory: %d KB\n", info.memory);

	if (dom != NULL)
		virDomainFree(dom);
	if (conn != NULL)
		virConnectClose(conn);
	
	return 0;
}

int main(int argc, char **argv)
{
	int dom_id = 3;
	printf("-----Get domain info by ID via libvirt C API -----\n");
	getDomainInfo(dom_id);
	return 0;
}
