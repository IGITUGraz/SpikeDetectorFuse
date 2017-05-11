# Deadlock Test

##  How to install this test

This test creates a deadlock in NEST v2.12.0 by throwing an error from exactly one thread. It is created as a module 'nestdeadlocktestmodule' which can be compiled and installed via the accompanying installation script 'install.sh', To use it use the Python 'nest.Install' or the sli equivalent.

##  Installation requirements

1.  The test is in python3 so it requires the appropriately compiled NEST before installation.
2.  The `nest-config` executable fof the appropriate nest must be in path
2.  In order to use the modules in python, the modules directory must be a part of LD_LIBRARY_PATH

##  How to run

After having installed via './install.sh', simply run the python script via python3, it should print a success message and then hang on the next simulation. Note that once this happens, the only way to kill the process is using the `kill` command.