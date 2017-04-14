import numpy as np

import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt

import nest
from nest import raster_plot

nest.Install('spikedetfusemodule')

nest.SetKernelStatus({'total_num_virtual_procs': 12})

exc_neurons = nest.Create('iaf_neuron', 100)
current_gen = nest.Create('noise_generator', params={'mean': 400.0, 'std': 100.0})    
spike_det = nest.Create('spike_detector_fuse')

try:
    # This should return an exception that lists BadParameter as the C++ exception name and should
    # be the exception complaining about the assignment of illegal parameters to the spike detector
    nest.SetStatus(spike_det, {'frequency_thresh':-0.1})
except nest.NESTError as E:
    E_msg = E.args[0]
    if E_msg.startswith('BadParameter') and 'frequency_thresh' in E_msg:
        print("SUCCESSfull raised following NEST exception:\n")
        print("    ", E_msg)
        print()
    else:
        raise
except:
    raise
else:
    raise RuntimeError("Test FAILED. NEST incorrectly ran the spike_detector_w_check with illegal"
                       " (negative) parameters.")

# Performing Relevant Connections
weight_EE = 200.0
delay_EE = 1

nest.Connect(exc_neurons, exc_neurons, syn_spec={'weight':weight_EE, 'delay':delay_EE})
nest.Connect(exc_neurons, spike_det)
nest.Connect(current_gen, exc_neurons[0:6])

# Setting termination criterion
nest.SetStatus(spike_det, {'frequency_thresh':200.0, 'length_thresh':50.0, 'n_connected_neurons':len(exc_neurons)})

try:
    nest.Simulate(200)
except nest.NESTError as E:
    E_msg = E.args[0]
    if E_msg.startswith('UnstableSpiking'):
        print("SUCCESSfully caught unstable spiking and returned folowing exception:\n")
        print("    ", E_msg)
        print()
    else:
        raise
except:
    raise
else:
    raise RuntimeError("Test FAILED. NEST incorrectly continued running the network despite"
                       " unstable activity")

print("Plotting and saving spike data")
plt.figure()
spike_det_data = nest.GetStatus(spike_det, 'events')[0]
raster_plot.from_data(np.vstack((spike_det_data['senders'], spike_det_data['times'])).T)
plt.savefig('raster_plot.png', format='png', dpi=400)
