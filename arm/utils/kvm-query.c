/*
 * kvm-query.c
 *
 * A simple utility to query KVM capabilities.
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>

#include <linux/kvm.h>

int get_max_vcpu(void)
{
	int fd = open("/dev/kvm", O_RDWR);
	if (fd>0) {
		int ret = ioctl(fd, KVM_CHECK_EXTENSION, KVM_CAP_MAX_VCPUS);
		printf("%d\n", ret > 0 ? ret : 0);
		close(fd);
		return 0;
	} else {
		return -1;
	}
}

int main(int argc, char **argv)
{
	for (int i=0; i<argc; i++) {
		char *arg = argv[i];
		if (strcmp(arg, "max_vcpu") == 0) {
			return get_max_vcpu();
		}
	}

	return  -1;
}
