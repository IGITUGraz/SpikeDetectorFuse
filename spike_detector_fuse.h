/*
 *  spike_detector.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SPIKE_DETECTOR_FUSE_H
#define SPIKE_DETECTOR_FUSE_H


// C++ includes:
#include <vector>

// Includes from nestkernel:
#include "event.h"
#include "exceptions.h"
#include "nest_types.h"
#include "node.h"
#include "recording_device.h"

/* BeginDocumentation

Name: spike_detector_fuse - Device for detecting single spikes, In addition, it also
      "fuses" the simulation on too much spiking

Description:

The following portion of the documentation is written for the spike detection
features (Borrowed from the documentation of the spike_detector device)

 The spike_detector_fuse device is a recording device. It is used to record
spikes from a single neuron, or from multiple neurons at once. Data
is recorded in memory or to file as for all RecordingDevices.
By default, GID and time of each spike is recorded.

The spike detector can also record spike times with full precision
from neurons emitting precisely timed spikes. Set /precise_times to
achieve this. If there are precise models and /precise_times is not
set, it will be set to True at the start of the simulation and
/precision will be increased to 15 from its default value of 3.

Any node from which spikes are to be recorded, must be connected to
the spike detector using a normal connect command. Any connection weight
and delay will be ignored for that connection.

Simulations progress in cycles defined by the minimum delay. During each
cycle, the spike detector records (stores in memory or writes to screen/file)
the spikes generated during the previous cycle. As a consequence, any
spikes generated during the cycle immediately preceding the end of the
simulation time will not be recorded. Setting the /stop parameter to at the
latest one min_delay period before the end of the simulation time ensures that
all spikes desired to be recorded, are recorded.

 Spike are not necessarily written to file in chronological order.

This portion is for the "fuse" portion of the documentation.

There are 3 important parameters for the spike_detector_fuse:

1.  frequency_thresh - Frequency Threshold
2.  length_thresh - Length of spiking to which this frequency threshold is acceptable
                    [full dynamics explained below]
3.  n_connected_neurons - Number of neurons connected to the spike_detector_fuse

The algorithm calculates a "danger trace" as follows:

 d[i+1] = d[i]*alpha + n_spikes_in_current_slice*delta

NOTE that the updating of danger is done once per slice and not once per step

The parameters alpha and delta are calculated as follows:

1.  A consistent firing of rate 'frequency_thresh' converges to a steady state danger level of 1
2.  A consistent firing of rate 'frequency_thresh' converges to 0.99 in length_thresh time
3.  The parameter n_connected_neurons must be specified from the user as NEST does not make this
    information available to the Node. This parameter is required to calculate the frequency of
    firing of individual neurons

If the value of the "Danger Trace" exceeds 1, an UnstableSpiking exception is thrown and the
simulation is aborted. Once the simulation is aborted in this manner, it can no longer be resumed.
The data for the simulation on the last run may be inconsistent in the sense that some neurons may
not have run their update function for the slice, but the spike data for that slice will be stored.

Receives: nest::SpikeEvent

SeeAlso: spike_detector, Device, nest::RecordingDevice
*/


namespace mynest
{

/**
 * Exception to be thrown if the wrong argument type
 * is given to a function
 * @ingroup nest::KernelExceptions
 */
class UnstableSpiking : public nest::KernelException
{
public:
  UnstableSpiking()
      : nest::KernelException( "UnstableSpiking" )
  {
  }
  ~UnstableSpiking() throw()
  {
  }

  std::string message() const;
};

/**
 * Spike detector class with checks to detect unstable spiking.
 *
 * This class manages spike recording for normal and precise spikes. It
 * receives spikes via its handle(nest::SpikeEvent&) method, buffers them, and
 * stores them via its nest::RecordingDevice in the update() method.
 *
 * Spikes are buffered in a two-segment buffer. We need to distinguish between
 * two types of spikes: those delivered from the global event queue (almost all
 * spikes) and spikes delivered locally from devices that are replicated on VPs
 * (has_proxies() == false).
 * - Spikes from the global queue are delivered by deliver_events() at the
 *   beginning of each update cycle and are stored only until update() is called
 *   during the same update cycle. Global queue spikes are thus written to the
 *   read_toggle() segment of the buffer, from which update() reads.
 * - Spikes delivered locally may be delivered before or after
 *   spike_detector::update() is executed. These spikes are therefore buffered
 *   in the write_toggle() segment of the buffer and output during the next
 *   cycle.
 * - After all spikes are recorded, update() clears the read_toggle() segment
 *   of the buffer.
 *
 * @ingroup Devices
 */
class spike_detector_fuse : public nest::Node
{

public:
  spike_detector_fuse();
  spike_detector_fuse( const spike_detector_fuse& );

  void set_has_proxies( const bool hp );
  bool
  has_proxies() const
  {
    return has_proxies_;
  }
  bool
  potential_global_receiver() const
  {
    return true;
  }
  void set_local_receiver( const bool lr );
  bool
  local_receiver() const
  {
    return local_receiver_;
  }

  /**
   * Import sets of overloaded virtual functions.
   * @see Technical Issues / Virtual Functions: Overriding, Overloading, and
   * Hiding
   */
  using nest::Node::handle;
  using nest::Node::handles_test_event;
  using nest::Node::receives_signal;

  void handle( nest::SpikeEvent& );

  nest::port handles_test_event( nest::SpikeEvent&, nest::rport );

  nest::SignalType receives_signal() const;

  void get_status( DictionaryDatum& ) const;
  void set_status( const DictionaryDatum& );

private:
  void init_state_( nest::Node const& );
  void init_buffers_();
  void calibrate();
  void finalize();

  /**
   * Update detector by recording spikes.
   *
   * All spikes in the read_toggle() half of the spike buffer are
   * recorded by passing them to the nest::RecordingDevice, which then
   * stores them in memory or outputs them as desired.
   *
   * @see nest::RecordingDevice
   */
  void update( nest::Time const&, const long, const long );

  /**
   * Buffer for incoming spikes.
   *
   * This data structure buffers all incoming spikes until they are
   * passed to the nest::RecordingDevice for storage or output during update().
   * update() always reads from spikes_[Network::get_network().read_toggle()]
   * and deletes all events that have been read.
   *
   * Events arriving from locally sending nodes, i.e., devices without
   * proxies, are stored in spikes_[Network::get_network().write_toggle()], to
   * ensure order-independent results.
   *
   * Events arriving from globally sending nodes are delivered from the
   * global event queue by Network::deliver_events() at the beginning
   * of the time slice. They are therefore written to
   * spikes_[Network::get_network().read_toggle()]
   * so that they can be recorded by the subsequent call to update().
   * This does not violate order-independence, since all spikes are delivered
   * from the global queue before any node is updated.
   */
  struct Buffers_
  {
    std::vector< std::vector< nest::Event* > > spikes_;
  };

  struct Parameters_
  {
    double frequency_thresh;
    double length_thresh;
    long n_connected_neurons;

    Parameters_();

    void get(DictionaryDatum &) const;  //!< Store current values in dictionary
    void set(const DictionaryDatum &);  //!< Set values from dictionary
  };

  struct State_
  {
    double danger_level;
    bool is_receiving_spikes;
    unsigned int n_current_spikes;

    State_();
  };

  struct Variables_
  {
    double danger_decay_factor;
    double danger_increment_step;

    Variables_();
  };

  nest::RecordingDevice device_;
  Buffers_ B_;

  Parameters_ P_;
  State_ S_;
  Variables_ V_;

  bool has_proxies_;
  bool local_receiver_;
};

inline void
spike_detector_fuse::set_has_proxies( const bool hp )
{
  has_proxies_ = hp;
}

inline void
spike_detector_fuse::set_local_receiver( const bool lr )
{
  local_receiver_ = lr;
}

inline nest::port
spike_detector_fuse::handles_test_event( nest::SpikeEvent&, nest::rport receptor_type )
{
  if ( receptor_type != 0 )
    throw nest::UnknownReceptorType( receptor_type, get_name() );
  return 0;
}

inline void
spike_detector_fuse::finalize()
{
  device_.finalize();
}

inline nest::SignalType
spike_detector_fuse::receives_signal() const
{
  return nest::ALL;
}

} // namespace

#endif /* #ifndef SPIKE_DETECTOR_H */
