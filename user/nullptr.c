#include "types.h"
#include "user.h"

int main() {
	printf(1, "Testing nullptr dereference...\n");
	int* const p = 0;
	printf(1, "*(%x) = %x\n", p, *p);
	exit();
}