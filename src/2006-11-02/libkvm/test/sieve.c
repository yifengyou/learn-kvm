#include "vm.h"

void print(const char *text);

void printi(int n)
{
    char buf[10], *p = buf;
    int s = 0, i;
    
    if (n < 0) {
	n = -n;
	s = 1;
    }

    while (n) {
	*p++ = '0' + n % 10;
	n /= 10;
    }
    
    if (s)
	*p++ = '-';

    if (p == buf)
	*p++ = '0';
    
    for (i = 0; i < (p - buf) / 2; ++i) {
	char tmp;

	tmp = buf[i];
	buf[i] = p[-1-i];
	p[-1-i] = tmp;
    }

    *p = 0;

    print(buf);
}

int sieve(char* data, int size)
{
    int i, j, r = 0;

    for (i = 0; i < size; ++i)
	data[i] = 1;

    data[0] = data[1] = 0;

    for (i = 2; i < size; ++i)
	if (data[i]) {
	    ++r;
	    for (j = i*2; j < size; j += i)
		data[j] = 0;
	}
    return r;
}

void test_sieve(const char *msg, char *data, int size)
{
    int r;

    print(msg);
    print(": ");
    r = sieve(data, size);
    printi(r);
    print("\n");
}

#define STATIC_SIZE 1000000
#define VSIZE 100000000
char static_data[STATIC_SIZE];

int main()
{
    void *v;
    int i;

    print("starting sieve\n");
    test_sieve("static", static_data, STATIC_SIZE);
    setup_vm();
    print("mapped: ");
    test_sieve("mapped", static_data, STATIC_SIZE);
    for (i = 0; i < 30; ++i) {
	v = vmalloc(VSIZE);
	test_sieve("virtual", v, VSIZE);
	vfree(v);
    }
    
    return 0;
}
