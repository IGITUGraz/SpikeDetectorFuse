/*
 *  spike_detector.cpp
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

#include "spike_detector_fuse.h"

// C++ includes:
#include <numeric>

// Includes from libnestutil:
#include "compose.hpp"
#include "logging.h"

// Includes from nestkernel:
#include "event_delivery_manager_impl.h"
#include "kernel_manager.h"
#include "sibling_container.h"

// Includes from sli:
#include "arraydatum.h"
#include "dict.h"
#include "dictutils.h"
#include "doubledatum.h"
#include "integerdatum.h"

std::string
mynest::UnstableSpiking::message() const
{
  return std::string(
      "The Network seems to be in a regime of unstable spiking, terminating simulation");
}

mynest::spike_detector_fuse::spike_detector_fuse()
    : nest::Node()
    // record time and gid
    , device_( *this, nest::RecordingDevice::SPIKE_DETECTOR, "gdf", true, true )
    , has_proxies_( false )
    , local_receiver_( true )
    , P_()
    , V_()
    , S_()
{
}

mynest::spike_detector_fuse::spike_detector_fuse( const spike_detector_fuse& n )
    : nest::Node( n )
    , device_( *this, n.device_ )
    , has_proxies_( false )
    , local_receiver_( true )
    , P_(n.P_)
    , V_(n.V_)
    , S_(n.S_)
{
}

mynest::spike_detector_fuse::Parameters_::Parameters_()
    : frequency_thresh(0.0)
    , length_thresh(0.0)
    , n_connected_neurons(0)
{}

mynest::spike_detector_fuse::State_::State_()
    : is_receiving_spikes(false)
    , n_current_spikes(0)
    , is_unstable(false)
    , danger_level(0.0)
{}

mynest::spike_detector_fuse::Variables_::Variables_()
    : danger_decay_factor(0.0)
    , danger_increment_step(0.0)
{}

void mynest::spike_detector_fuse::Parameters_::set(const DictionaryDatum &d)
{
  updateValue<double>(d, "frequency_thresh", frequency_thresh);
  updateValue<double>(d, "length_thresh", length_thresh);
  updateValue<long>(d, "n_connected_neurons", n_connected_neurons);

  if (frequency_thresh < 0 || length_thresh < 0 || n_connected_neurons < 0) {
    throw nest::BadParameter("length_thresh, frequency_thresh, and n_connected_neurons must be non-negative");
  }
}

void mynest::spike_detector_fuse::Parameters_::get(DictionaryDatum &d) const
{
  def<double>(d, "frequency_thresh", frequency_thresh);
  def<double>(d, "length_thresh", length_thresh);
  def<long>(d, "n_connected_neurons", n_connected_neurons);
}

void
mynest::spike_detector_fuse::init_state_( const nest::Node& np )
{
  const spike_detector_fuse& sd = dynamic_cast< const spike_detector_fuse& >( np );
  device_.init_state( sd.device_ );
  P_ = sd.P_;
  S_ = sd.S_;
  V_ = sd.V_;
  init_buffers_();
}

void
mynest::spike_detector_fuse::init_buffers_()
{
  device_.init_buffers();

  std::vector< std::vector< nest::Event* > > tmp( 2, std::vector< nest::Event* >() );
  B_.spikes_.swap( tmp );
}

void
mynest::spike_detector_fuse::calibrate()
{

  if ( nest::kernel().event_delivery_manager.get_off_grid_communication()
       and not device_.is_precise_times_user_set() )
  {
    device_.set_precise_times( true );
    std::string msg = String::compose(
        "Precise neuron models exist: the property precise_times "
            "of the %1 with gid %2 has been set to true",
        get_name(),
        get_gid() );

    if ( device_.is_precision_user_set() )
    {
      // if user explicitly set the precision, there is no need to do anything.
      msg += ".";
    }

    else
    {
      // it makes sense to increase the precision if precise models are used.
      device_.set_precision( 15 );
      msg += ", precision has been set to 15.";
    }

    LOG( nest::M_INFO, "spike_detector_fuse::calibrate", msg );
  }

  // Validate Parameters
  if (P_.length_thresh == 0 || P_.frequency_thresh == 0  || P_.n_connected_neurons == 0) {
    // This is the case where no termination is performed
    V_.danger_decay_factor = 0.0;
    V_.danger_increment_step = 0.0;

    if (get_thread() == 0) {
      std::string msg;
      msg += "GID: ";
      msg += std::to_string(this->get_gid());
      msg += " Spike Detector Not Fusing";
      LOG( nest::M_WARNING, "spike_detector_fuse::calibrate", msg);
    }
  }
  else {
    // Calibrate the decay and increment parameters based on input parameters
    double steps_per_ms = nest::kernel().simulation_manager.get_clock().delay_ms_to_steps(1);
    double min_delay = nest::kernel().connection_manager.get_min_delay();
    size_t n_siblings = nest::kernel().node_manager.get_thread_siblings( get_gid() )->num_thread_siblings();

    // Discretizing length in terms of simulation update steps
    int length_update_steps = int(P_.length_thresh * steps_per_ms / min_delay + 0.5);

    // Calculating decay_factor from the following transient equation describing convergence of danger to maximum /
    // steady state:
    //
    //     danger_decay_factor^length_update_steps = 0.01
    V_.danger_decay_factor = std::pow(0.01, 1.0/length_update_steps);

    // Calculating scale factor by requiring that the steady state danger for a network spiking at frequency_thresh
    // is 1
    //
    // i.e. (P_.frequency_thresh*(P_.n_connected_neurons/n_siblings)*V_.danger_increment_step)/(1-V_.danger_decay_factor) = 1
    V_.danger_increment_step = (1 - V_.danger_decay_factor)*n_siblings/(P_.frequency_thresh*P_.n_connected_neurons*1e-3);
  }

  device_.calibrate();
}

void
mynest::spike_detector_fuse::update( nest::Time const&, const long, const long )
{

  for ( std::vector< nest::Event* >::iterator e =
      B_.spikes_[ nest::kernel().event_delivery_manager.read_toggle() ].begin();
        e != B_.spikes_[ nest::kernel().event_delivery_manager.read_toggle() ].end();
        ++e )
  {
    assert( *e != 0 );
    device_.record_event( **e );
    delete *e;
  }

  // do not use swap here to clear, since we want to keep the reserved()
  // memory for the next round
  B_.spikes_[ nest::kernel().event_delivery_manager.read_toggle() ].clear();

  /*
   * Implementing the fusing mechanics.
   *
   * For successful raising of exceptions with multiple threads, the exception must
   * be thrown by all threads at the same time slice else nest enters a deadlock
   * situation.
   *
   * The way this is accomplished is as follows:
   *
   * 1.  There are 4 state variables per sibling that are necessary:
   *
   *     a.  danger_level
   *     b.  n_current_spikes
   *     c.  is_receiving_spikes
   *     d.  is_unstable
   *
   * 2.  Whenever spikes arrive at a sibling, they are counted into the variable
   *     `n_current_spikes`.
   *
   * 3.  The variable `is_receiving_spikes` represents the state of the spike
   *     recorder. It is useful for the spike handler to know the first time it is
   *     called in a slice, as well as for the update function to know if the
   *     counter was reset or not this slice.
   *
   * 4.  The update function updates the danger level based on the number of spikes
   *     in the current slice.
   *
   * 5.  Then the spike handler observes this danger level and sets the
   *     `is_unstable` flag
   *
   * 6.  The update function is then responsible for observing the is_unstable flag
   *     of all its siblings and firing if any one is set. This way all the siblings
   *     throw an exceptions at the same time step and the deadlock is avoided
   */
  S_.danger_level *= V_.danger_decay_factor;

  // The count is only considered if the S_.is_receiving_spikes is true indicating that the count is actually the
  // count of this time step. this avoids using un-cleared counts which might happen if no spikes are received in
  // a given simulation slice
  if (S_.is_receiving_spikes) {
    S_.danger_level += V_.danger_increment_step * S_.n_current_spikes;
  }

  bool any_are_unstable = false;
  const nest::SiblingContainer* siblings =
      nest::kernel().node_manager.get_thread_siblings( get_gid() );
  std::vector< nest::Node* >::const_iterator sibling;
  for ( sibling = siblings->begin(); sibling != siblings->end();
        ++sibling ) {
    const spike_detector_fuse &sib_spike_detector_fuse = downcast<spike_detector_fuse>(*(*sibling));
    any_are_unstable |= sib_spike_detector_fuse.S_.is_unstable;
  }

  if (any_are_unstable) {
    throw UnstableSpiking();
  }

  // Update relevant state flags
  S_.is_receiving_spikes = false;
}

void
mynest::spike_detector_fuse::get_status( DictionaryDatum& d ) const
{
  P_.get(d);

  // get the data from the device
  device_.get_status( d );

  // if we are the device on thread 0, also get the data from the
  // siblings on other threads
  if ( local_receiver_ && get_thread() == 0 )
  {
    const nest::SiblingContainer* siblings =
        nest::kernel().node_manager.get_thread_siblings( get_gid() );
    std::vector< nest::Node* >::const_iterator sibling;
    for ( sibling = siblings->begin() + 1; sibling != siblings->end();
          ++sibling )
      ( *sibling )->get_status( d );
  }
}

void
mynest::spike_detector_fuse::set_status( const DictionaryDatum& d )
{
  Parameters_ Ptemp;
  Ptemp.set(d); // This is to ensure that parameterss are not illegally overridden in case of an exception
  P_ = Ptemp;
  device_.set_status( d );
}

void
mynest::spike_detector_fuse::handle( nest::SpikeEvent& e )
{
  // accept spikes only if detector was active when spike was
  // emitted
  if ( device_.is_active( e.get_stamp() ) )
  {
    assert( e.get_multiplicity() > 0 );

    // If this is the first call of handle in the current simulation step, then reset the spike counter
    // for the current simulation step.
    if (not S_.is_receiving_spikes) {
      S_.n_current_spikes = 0;
      if (S_.danger_level > 1.0)
        S_.is_unstable = true;
      S_.is_receiving_spikes = true;
    }

    // Add The Spikes to the counter to be used to calculate danger level
    S_.n_current_spikes += e.get_multiplicity();

    long dest_buffer;
    if ( nest::kernel()
        .modelrange_manager.get_model_of_gid( e.get_sender_gid() )
        ->has_proxies() )
      // events from central queue
      dest_buffer = nest::kernel().event_delivery_manager.read_toggle();
    else
      // locally delivered events
      dest_buffer = nest::kernel().event_delivery_manager.write_toggle();

    for ( int i = 0; i < e.get_multiplicity(); ++i )
    {
      // We store the complete events
      nest::Event* event = e.clone();
      B_.spikes_[ dest_buffer ].push_back( event );
    }
  }
}
