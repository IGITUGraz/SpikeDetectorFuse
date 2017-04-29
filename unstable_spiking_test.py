#!/usr/bin/env python3
import numpy as np
import tempfile
import nest

from file_redirector import stdout_redirected

nest.Install('spikedetfusemodule')

params_dict_items = [
    ('N_src_array', np.array([100, 800])),
    ('N_threads_array', np.array([1, 4, 12])),
    ('rate_array', np.array([20., 60., 100.])),  # Hz
    ('freq_thresh_array', np.array([20., 60., 100.])),  # Hz
    ('length_thresh_array', np.array([100., 200.])),  # ms
]

params_dict_vals = [x[1] for x in params_dict_items]
params_dict_vals_meshgrid = np.meshgrid(*params_dict_vals, indexing='ij')
params_dict_vals_cartprod = [x.ravel() for x in params_dict_vals_meshgrid]
params_dict_items_cartprod = [(pname, pcartprod)
                              for (pname, _), pcartprod in zip(params_dict_items, params_dict_vals_cartprod)]

nest.SetKernelStatus({'total_num_virtual_procs': 12})
spike_gen = nest.Create('spike_generator', params={})
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

n_iters = len(params_dict_vals_cartprod[0])

for i, (N_src,
        N_threads, 
        rate, 
        freq_thresh, 
        length_thresh) in enumerate(zip(*params_dict_vals_cartprod)):

    with stdout_redirected('nestdump.txt'):
        nest.ResetKernel()
        nest.SetKernelStatus({'total_num_virtual_procs': N_threads})
    spike_gen = nest.Create('poisson_generator', params={'rate':rate})
    parrot_neurons = nest.Create('parrot_neuron', N_src)
    spike_det = nest.Create('spike_detector_fuse', params={'frequency_thresh': freq_thresh,
                                                           'length_thresh': length_thresh,
                                                           'n_connected_neurons': N_src})

    # Performing Relevant Connections
    nest.Connect(spike_gen, parrot_neurons)
    nest.Connect(parrot_neurons, spike_det)

    print("")
    print("Run Number    : {}".format(i))
    print("N_src         : {}".format(N_src))
    print("N_threads     : {}".format(N_threads))
    print("rate          : {}".format(rate))
    print("freq_thresh   : {}".format(freq_thresh))
    print("length_thresh : {}".format(length_thresh))

    try:
        with stdout_redirected('nestdump.txt'):
            nest.Simulate(500)
    except nest.NESTError as E:
        E_msg = E.args[0]
        if E_msg.startswith('UnstableSpiking'):
            assert rate >= freq_thresh, \
                "Test FAILED. The Unstable Spiking was caught even though rate <= freq_thresh"
            print("  UNSTABLE at {:.4f} ms".format(nest.GetKernelStatus()['time']))
        else:
            raise
    except:
        raise
    else:
        assert rate <= freq_thresh, \
            "Test FAILED. The Unstable Spiking was not caught even though rate > freq_thresh"
        print("  STABLE")

print("ALL TESTS PASSED SUCCESSFULLY")
