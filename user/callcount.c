 #include "types.h"
 #include "stat.h"
 #include "user.h"

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf(1, "Invalid arguments\nUsage: callcount syscall_id\n");
		exit();
	}
	int id = atoi(argv[1]);
	printf(1, "The system call with ID %d has been called %d times\n", id, callcount(id));
	exit();
}