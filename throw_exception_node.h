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

#ifndef THROW_EXCEPTION_NODE_H
#define THROW_EXCEPTION_NODE_H


// C++ includes:
#include <vector>

// Includes from nestkernel:
#include "exceptions.h"
#include "node.h"

/* BeginDocumentation

Name: throw_exception_node - Device for throwing an exception to test for nest deadlock

Description:

 The only thing this node does is throw an exception on update if the thread is thread
 number 0, In the current scenario this should result in NEST entering a deadlock.

 In order to test the deadlock th NEST run must be multi-threaded bot NOT Multi-process
 (i.e. it should not be run with more than one process)
*/


namespace mynest
{

/**
 * Exception to be thrown as Test
 * @ingroup nest::KernelExceptions
 */
class TestException : public nest::KernelException
{
public:
  TestException()
      : nest::KernelException( "TestException" )
  {
  }
  ~TestException() throw()
  {
  }

  std::string message() const;
};

/**
 * Test class that represents a local receiver without proxies i.e. contains sibling nodes
 * across threads. The only thing this node does is throw an exception on update if the
 * thread is thread number 0, In the current scenario this should result in NEST entering
 * a deadlock.
 *
 * @ingroup Devices
 */
class throw_exception_node : public nest::Node
{

public:
  throw_exception_node();
  throw_exception_node( const throw_exception_node& );

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

  void get_status( DictionaryDatum& ) const;
  void set_status( const DictionaryDatum& );

protected:

  virtual void calibrate();
  virtual void init_state_(const nest::Node &node);
  virtual void init_buffers_();

private:

  /**
   * Throw exception if thread 0
   * @see nest::RecordingDevice
   */
  void update( nest::Time const&, const long, const long );

  bool has_proxies_;
  bool local_receiver_;
};

inline void
throw_exception_node::set_has_proxies( const bool hp )
{
  has_proxies_ = hp;
}

inline void
throw_exception_node::set_local_receiver( const bool lr )
{
  local_receiver_ = lr;
}

} // namespace

#endif /* #ifndef SPIKE_DETECTOR_H */
