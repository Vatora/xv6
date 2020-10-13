#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char* argv[])
{
  if(argc != 2){
    printf(1, "Usage: smallfile <filename>\n");
    exit();
  }

  const int fd = open(argv[1], O_CREATE | O_SMALLFILE | O_RDWR);
  printf(1, "Small file opened (fd: %d)\n", fd);

  char buf[] = "EXAMPLE TEXT";
  printf(1, "Writing \"%s\" to the small file\n", buf);
  const int written = write(fd, buf, sizeof(buf));

  printf(1, "Wrote %d bytes\n", written);
  close(fd);

  exit();
}