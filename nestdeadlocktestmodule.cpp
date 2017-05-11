/*
 *  swtamodule.cpp
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

// include necessary NEST headers
#include "config.h"

#include "connection_manager_impl.h"
#include "connector_model_impl.h"
#include "dynamicloader.h"
#include "exceptions.h"
#include "genericmodel.h"
#include "genericmodel_impl.h"
#include "kernel_manager.h"
#include "model.h"
#include "model_manager_impl.h"
#include "nestmodule.h"
#include "target_identifier.h"


// include headers with your own stuff
#include "nestdeadlocktestmodule.h"
#include "throw_exception_node.h"

// -- Interface to dynamic module loader ---------------------------------------

/*
 * The dynamic module loader must be able to find your module.
 * You make the module known to the loader by defining an instance of your
 * module class in global scope. This instance must have the name
 *
 * <modulename>_LTX_mod
 *
 * The dynamicloader can then load modulename and search for symbol "mod" in it.
 */

mynest::NestExceptionDeadlockTestModule nestdeadlocktestmodule_LTX_mod;

// -- DynModule functions ------------------------------------------------------

mynest::NestExceptionDeadlockTestModule::NestExceptionDeadlockTestModule()
{
#ifdef LINKED_MODULE
  // register this module at the dynamic loader
  // this is needed to allow for linking in this module at compile time
  // all registered modules will be initialized by the main app's dynamic loader
  nest::DynamicLoaderModule::registerLinkedModule(this);
#endif
}

mynest::NestExceptionDeadlockTestModule::~NestExceptionDeadlockTestModule()
{}

const std::string mynest::NestExceptionDeadlockTestModule::name(void) const
{
  return std::string("Exception Deadlock Test Module"); // Return name of the module
}

//-------------------------------------------------------------------------------------

void mynest::NestExceptionDeadlockTestModule::init( SLIInterpreter* i )
{
  /* Register a neuron or device model.
     Give node type as template argument and the name as second argument.
     The first argument is always a reference to the network.
     Return value is a handle for later unregistration.
  */
  nest::kernel().model_manager.register_node_model<mynest::throw_exception_node>("throw_exception_node");

}  // NestExceptionDeadlockTestModule::init()
