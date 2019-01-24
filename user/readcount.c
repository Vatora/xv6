 #include "types.h"
 #include "stat.h"
 #include "user.h"

int main() {
	printf(1, "There have been %d read() system calls\n", getreadcount());
	exit();
}