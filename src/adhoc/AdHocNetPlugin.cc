#include <gazebo/common/Assert.hh>
#include <gazebo/common/Events.hh>

#include "adhoc/CommonTypes.hh"
#include "adhoc/AdHocNetPlugin.hh"
#include "quadrotor_tunnel_nav/protobuf/datagram.pb.h"

using namespace gazebo;

GZ_REGISTER_WORLD_PLUGIN(AdHocNetPlugin)

/////////////////////////////////////////////////
void AdHocNetPlugin::Load(physics::WorldPtr _world, sdf::ElementPtr /*_sdf*/)
{
  GZ_ASSERT(_world, "AdHocNetPlugin world pointer is NULL");
  this->world = _world;

  this->updateConnection = event::Events::ConnectWorldUpdateBegin(
          std::bind(&AdHocNetPlugin::OnUpdate, this));

  gzmsg << "Starting Ad Hoc Net server" << std::endl;

  // Advertise a oneway service for centralizing all message requests.
  if (!node.Advertise(kBrokerService, &AdHocNetPlugin::OnMessage, this))
  {
    gzerr << "Error advertising service [" << kBrokerService << "]"
          << std::endl;
    return;
  }
}

/////////////////////////////////////////////////
void AdHocNetPlugin::OnUpdate()
{
  // ToDo: Step the comms model.

  std::lock_guard<std::mutex> lk(this->mutex);
  this->ProcessIncomingMsgs();
}

/////////////////////////////////////////////////
void AdHocNetPlugin::ProcessIncomingMsgs()
{
  while (!this->incomingMsgs.empty())
  {
    // Forward the messages.
    auto const &msg = this->incomingMsgs.front();
    auto endPoint = msg.dst_address() + ":" + std::to_string(msg.dst_port());
    this->node.Request(endPoint, msg);
    this->incomingMsgs.pop();
  }
}

/////////////////////////////////////////////////
void AdHocNetPlugin::OnMessage(const adhoc::msgs::Datagram &_req)
{
  // Just save the message, it will be processed later.
  std::lock_guard<std::mutex> lk(this->mutex);
  this->incomingMsgs.push(_req);
}
