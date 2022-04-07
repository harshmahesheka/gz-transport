/*
 * Copyright (C) 2022 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "ParamCommandAPI.hh"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <ignition/msgs/boolean.pb.h>
#include <ignition/msgs/empty.pb.h>
#include <ignition/msgs/parameter.pb.h>
#include <ignition/msgs/parameter_declarations.pb.h>
#include <ignition/msgs/parameter_name.pb.h>
#include <ignition/msgs/parameter_value.pb.h>

#include <ignition/transport/parameters/Client.hh>

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
extern "C" void cmdParametersList(const char * _ns)
{
  parameters::ParametersClient client{_ns};

  std::cout << std::endl << "Listing parameters, registry namespace [" << _ns
            << "]..." << std::endl << std::endl;

  msgs::ParameterDeclarations res;
  try {
    res = client.ListParameters();
  } catch (const std::exception & ex) {
    std::cerr << "Failed to list parameters: " << ex.what() << std::endl;
  }

  if (!res.parameter_declarations_size()) {
    std::cout << "No parameters available" << std::endl;
    return;
  }
  for (const auto & decl : res.parameter_declarations()) {
    std::cout << decl.name() << "            [" << decl.type() << "]"
              << std::endl;
  }
}

//////////////////////////////////////////////////
extern "C" void cmdParameterGet(const char * _ns, const char *_paramName) {
  parameters::ParametersClient client{_ns};

  std::cout << std::endl << "Getting parameter [" << _paramName
            << "] for registry namespace [" << _ns << "]..." << std::endl;

  std::unique_ptr<google::protobuf::Message> value;
  try {
    value = client.GetParameter(_paramName);
  } catch (const std::exception & ex) {
    std::cerr << "Failed to get parameter: " << ex.what() << std::endl;
  }

  std::string msgType = "ign_msgs.";
  msgType += value->GetDescriptor()->name();
  std::cout << "Parameter type [" << msgType << "]" << std::endl << std::endl
            << "------------------------------------------------"
            << std::endl;
  {
    google::protobuf::io::OstreamOutputStream fos{&std::cout};
    if (!google::protobuf::TextFormat::Print(*value, &fos)) {
      std::cerr << "failed to convert the parameter value to a string"
                << std::endl;
      return;
    }
  }
  std::cout << "------------------------------------------------"
            << std::endl;
}

extern "C" void cmdParameterSet(
    const char * _ns, const char *_paramName, const char * _paramType,
    const char *_paramValue)
{
  parameters::ParametersClient client{_ns};

  std::cout << std::endl << "Setting parameter [" << _paramName
            << "] for registry namespace [" << _ns << "]..." << std::endl;

  auto msg = ignition::msgs::Factory::New(_paramType, _paramValue);
  if (!msg) {
    // try again, to check if the type name was valid
    auto defaultMsg = ignition::msgs::Factory::New(_paramType);
    std::cerr << "Could not create a message of type [" << _paramType << "]."
              << std::endl;
    if (!defaultMsg) {
      std::cerr << "The message type may be invalid." << std::endl;
      return;
    }
    std::cerr << "The message string representation may be invalid." << std::endl;
    return;
  }

  try {
    client.SetParameter(_paramName, *msg);
  } catch (const std::exception & ex) {
    std::cerr << "Failed to set parameter: " << ex.what() << std::endl;
  }
  std::cout << "Parameter successfully set!" << std::endl;
}
