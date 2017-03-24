# Spike Detector Fuse for NEST
This module monitors the spiking activity of the network, and throws a python exception if the spiking activity goes
beyond the given threshold for the given length of time.

It is a module for the [NEST simulator](http://www.nest-simulator.org).

# Installation
With the appropriate `nest-config` in your PATH:
1. First create a directory for the build, say `cmake-build`
2. `cd cmake-build && cmake ..`
3. `make && make install`
4. Run the line `export LD_LIBRARY_PATH=/path/to/nest/installation/lib/nest/:$LD_LIBRARY_PATH` or add it to your
   `~/.bashrc`/`~/.zshrc`/`~/.profile`

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
```
In this example, if your average network activity stays beyond 200Hz for longer than 50ms, a nest.NESTError will be
thrown with message 'UnstableSpiking in Simulate_d: The Network seems to be in a regime of unstable spiking, terminating
simulation'
