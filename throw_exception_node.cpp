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

#include "throw_exception_node.h"

// C++ includes:
#include <numeric>

// Includes from sli:
#include "dict.h"
#include "dictutils.h"

std::string
mynest::TestException::message() const
{
  return std::string(
      "Test Exception Thrown");
}

mynest::throw_exception_node::throw_exception_node()
    : nest::Node()
    // record time and gid
    , has_proxies_( false )
    , local_receiver_( true )
{
}

mynest::throw_exception_node::throw_exception_node( const throw_exception_node& n )
    : nest::Node( n )
    , has_proxies_( false )
    , local_receiver_( true )
{
}

void
mynest::throw_exception_node::init_state_( const nest::Node& np ) {}

void
mynest::throw_exception_node::init_buffers_() {}

void
mynest::throw_exception_node::calibrate() {}

void
mynest::throw_exception_node::update( nest::Time const& Now, const long, const long)
{
  if(this->get_thread() == 0)
  {
    throw TestException();
  }
}

void
mynest::throw_exception_node::get_status( DictionaryDatum& d ) const {}

void
mynest::throw_exception_node::set_status( const DictionaryDatum& d ) {}