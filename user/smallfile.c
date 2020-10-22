#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char* argv[])
{
  if(argc < 2){
    printf(1, "Usage: smallfile <filename>\n");
    exit();
  }

  const int fd = open(argv[1], O_CREATE | O_SMALLFILE | O_RDWR);
  printf(1, "Small file opened (fd: %d)\n", fd);

  // int bytes_written = 0;
  // char space[] = " ";
  // for (int i = 2; i < argc; ++i) {
  //   const int status = write(fd, argv[i], strlen(argv[i]));

  //   if (i < (argc - 1)) {
  //     write(fd, space, 1);
  //   }

  //   if (status < 0) {
  //     printf(1, "Error writing to file: %d\n", status);
  //     exit();
  //   }
  //   else {
  //     bytes_written += status;
  //   }
  // }
  char buf[] = "EXAMPLE TEXT";
  printf(1, "Writing \"%s\" to the small file\n", buf);
  int written = write(fd, buf, sizeof(buf)-1);

  printf(1, "Wrote %d bytes\n", written);

  char buf2[] = "_";
  printf(1, "Filling the small file\n");

  written = 0;
  for (int i = 0; i < 52-(sizeof(buf)-1); ++i) {
    written += write(fd, buf2, 1);
  }

  printf(1, "Wrote %d bytes\n", written);

  printf(1, "Writing more bytes to the small file\n");
  written = write(fd, buf, sizeof(buf));
  printf(1, "Wrote %d bytes\n", written);

  close(fd);

  exit();
}