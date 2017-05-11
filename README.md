# Spike Detector Fuse for NEST
This module monitors the spiking activity of the network, and throws a python exception if the spiking activity goes
beyond the given threshold for the given length of time. See the [comments in the
code](https://github.com/IGITUGraz/SpikeDetectorFuse/blob/master/spike_detector_fuse.h#L39) for more details.

It is a module for the [NEST simulator](http://www.nest-simulator.org). 

# Installation
With the appropriate `nest-config` in your PATH run `./install.sh` in the root directory.

Once the installation is done, add `/path/to/nest/installation/lib/nest/` to your `LD_LIBRARY_PATH` environment
variable.

# Usage
```
import nest
nest.Install('spikedetfusemodule')
...  # Create your neurons and connect up your network
spike_det = nest.Create('spike_detector_fuse')
nest.Connect(neurons, spike_det)
# Setting termination criterion
nest.SetStatus(spike_det, {'frequency_thresh':200.0, 'length_thresh':50.0, 'n_connected_neurons':len(exc_neurons)})
...  # Run your simulation
try:
    nest.Simulate(200)
except nest.NESTError as E:
    E_msg = E.args[0]
    if E_msg.startswith('UnstableSpiking'):
        print(E_msg)
    else:
        raise
```
In this example, if your average network activity stays beyond 200Hz for longer than 50ms, a nest.NESTError will be
thrown with message 'UnstableSpiking in Simulate_d: The Network seems to be in a regime of unstable spiking, terminating
simulation'

# Important notes
* nest.Simulate() cannot be run again without resetting the kernel by running nest.ResetKernel() once an unstable spiking exception is thrown
* Data upto the simulation slice where the exception is thrown can be safely retrieved and parsed even if the above exception is thrown.
