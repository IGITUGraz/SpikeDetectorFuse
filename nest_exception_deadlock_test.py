#!/usr/bin/env python3
import nest

import ipdb

ipdb.set_trace()
with ipdb.launch_ipdb_on_exception():
    nest.Install('nestdeadlocktestmodule')

    nest.SetKernelStatus({'total_num_virtual_procs': 1})
    # spike_detector
    exception_throwing_neuron = nest.Create('throw_exception_node')
    try:
        nest.Simulate(10)
    except nest.NESTError as E:
        E_msg = E.args[0]
        if "TestException" in E_msg:
            print("\nSUCCESS: The Single Threaded Example Passed")
        else:
            raise
    except Exception:
        raise
    else:
        assert False, "The Exception Wasn't Thrown"

    nest.ResetKernel()
    nest.SetKernelStatus({'total_num_virtual_procs': 12})
    exception_throwing_neuron = nest.Create('throw_exception_node', 10)
    print("\nThe following simulation should lead to a deadlock and hang. \n"
          "Note that I have found no clean way to test for this hang and exit")
    nest.Simulate(10)
