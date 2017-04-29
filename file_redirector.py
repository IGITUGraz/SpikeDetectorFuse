import os
import sys
from contextlib import contextmanager

@contextmanager
def stdout_redirected(to=os.devnull):
  '''
  import os

  with stdout_redirected(to=filename):
    print("from Python")
    os.system("echo non-Python applications are also supported")
  '''

  os_stdout_fd = sys.stdout.fileno()
  assert os_stdout_fd == 1, "Doesn't work if stdout is not the actual __stdout__"

  sys.stdout.flush() # + implicit flush()
  old_stdout = sys.stdout
  old_stdout_fd_dup = os.dup(sys.__stdout__.fileno())

  ##### assert that Python and C stdio write using the same file descriptor
  ####assert libc.fileno(ctypes.c_void_p.in_dll(libc, "stdout")) == os_stdout_fd == 1

  def _redirect_stdout(to):
    os.dup2(to.fileno(), os_stdout_fd) # os_stdout_fd writes to 'to' file
    sys.stdout = os.fdopen(os_stdout_fd, 'w') # Python writes to os_stdout_fd

  def _revert_stdout():
    sys.stdout.close()
    os.dup2(old_stdout_fd_dup, os_stdout_fd)
    sys.stdout = old_stdout

  with open(to, 'w') as file:
    _redirect_stdout(to=file)
    try:
      yield # allow code to be run with the redirected stdout
    finally:
      _revert_stdout()