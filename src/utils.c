#include "shell.h"

void interruptHandler(int sig_num)
{
  (void)sig_num; // Unused
  const char *msg = "\n## 3230yash >> ";
  if (write(STDOUT_FILENO, msg, strlen(msg)) == -1)
  {
  }
}

bool get_snapshot(pid_t pid, Snapshot *dest)
{
#ifdef __linux__
  char path[PATH_MAX_LEN];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid);

  FILE *fptr = fopen(path, "r");
  if (fptr == NULL)
  {
    return false; // process terminated
  }

  // read the needed data
  int scanned = fscanf(fptr, "%*d %*s %c %*d %*d %*d %*d %*d %*u %lu %*lu %lu %*lu %lu %lu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %lu %*ld %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*d %d",
                       &dest->state,
                       &dest->minflt,
                       &dest->majflt,
                       &dest->utime,
                       &dest->stime,
                       &dest->vsize,
                       &dest->CPUID);
  fclose(fptr);
  if (scanned != 7)
  {
    return false; // could not read all fields
  }
  return true;
#else
  (void)pid;
  (void)dest;
  return false;
#endif
}
