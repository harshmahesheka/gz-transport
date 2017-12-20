/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef IGN_TRANSPORT_NODESHARED_HH_
#define IGN_TRANSPORT_NODESHARED_HH_

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <google/protobuf/message.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include "ignition/transport/HandlerStorage.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Publisher.hh"
#include "ignition/transport/RepHandler.hh"
#include "ignition/transport/ReqHandler.hh"
#include "ignition/transport/SubscriptionHandler.hh"
#include "ignition/transport/TopicStorage.hh"
#include "ignition/transport/TransportTypes.hh"
#include "ignition/transport/Uuid.hh"

namespace ignition
{
  namespace transport
  {
    class Node;
    class NodePrivate;

    /// \brief Private data pointer
    class NodeSharedPrivate;

    /// \class NodeShared NodeShared.hh ignition/transport/NodeShared.hh
    /// \brief Private data for the Node class. This class should not be
    /// directly used. You should use the Node class.
    class IGNITION_TRANSPORT_VISIBLE NodeShared
    {
      /// \brief NodeShared is a singleton. This method gets the
      /// NodeShared instance shared between all the nodes.
      /// \return Pointer to the current NodeShared instance.
      public: static NodeShared *Instance();

      /// \brief Receive data and control messages.
      public: void RunReceptionTask();

      /// \brief Publish data.
      /// \param[in] _topic Topic to be published.
      /// \param[in] _data Data to publish.
      /// \param[in] _msgType Message type in string format.
      /// \return true when success or false otherwise.
      public: bool Publish(const std::string &_topic,
                           const std::string &_data,
                           const std::string &_msgType);

      /// \brief Method in charge of receiving the topic updates.
      public: void RecvMsgUpdate();

      /// \brief HandlerInfo contains information about callback handlers which
      /// is useful for local publishers and message receivers. You should only
      /// retrieve a HandlerInfo by calling CheckHandlerInfo(const std::string&)
      public: struct HandlerInfo
      {
        /// \brief This is a map of the standard local callback handlers. The
        /// key is the topic name, and the value is another map whose key is
        /// the node UUID and whose value is a smart pointer to the handler.
        public: std::map<std::string, ISubscriptionHandler_M> localHandlers;

        /// \brief This is a map of the raw local callback handlers. The key is
        /// the topic name, and the value is another map whose key is the node
        /// UUID and whose value is a smart pointer to the handler.
        public: std::map<std::string, RawSubscriptionHandler_M> rawHandlers;

        /// \brief True iff there are any standard local subscribers.
        public: bool haveLocal;

        /// \brief True iff there are any raw local subscribers
        public: bool haveRaw;

        // Friendship. This allows HandlerInfo to be created by
        // CheckHandlerInfo()
        friend class NodeShared;

        /// \brief Default constructor
        private: HandlerInfo() = default;
      };

      /// \brief Get information about the local and raw subscribers that are
      /// attached to this NodeShared.
      HandlerInfo CheckHandlerInfo(const std::string &_topic) const;

      /// \brief This struct provides information about the Subscribers of a
      /// Publisher. It should only be retrieved using CheckSubscriberInfo().
      /// The relevant subscriber info is a superset of the relevant HandlerInfo
      /// so we extend that struct.
      ///
      /// This struct is used internally by publishers to determine what kind of
      /// subscribers they have.
      public: struct SubscriberInfo : public HandlerInfo
      {
        /// \brief True iff this Publisher has any remote subscribers
        public: bool haveRemote;

        // Friendship declaration
        friend class NodeShared;

        /// \brief Default constructor.
        ///
        /// We do nothing here. CheckSubscriberInfo will fill this in. We make
        /// the constructor private to prevent us from having incorrectly
        /// initialized SubscriberInfo objects.
        private: SubscriberInfo() = default;
      };

      /// \brief Get information about the nodes that are subscribed to the
      /// publishers of this NodeShared.
      /// \return Information about subscribers.
      SubscriberInfo CheckSubscriberInfo(
          const std::string &_topic,
          const std::string &_msgType) const;

      /// \brief Call the SubscriptionHandler callbacks (local and raw) for this
      /// NodeShared.
      /// \param[in] _topic The topic name
      /// \param[in] _msgData The raw serialized data for the message
      /// \param[in] _msgType The name of the message type
      /// \param[in] _handlerInfo Information for the handlers of this node,
      /// as generated by CheckHandlerInfo(const std::string&)
      public: void TriggerSubscriberCallbacks(
        const std::string &_topic,
        const std::string &_msgData,
        const std::string &_msgType,
        const HandlerInfo &_handlerInfo);

      /// \brief Method in charge of receiving the control updates (when a new
      /// remote subscriber notifies its presence for example).
      public: void RecvControlUpdate();

      /// \brief Method in charge of receiving the service call requests.
      public: void RecvSrvRequest();

      /// \brief Method in charge of receiving the service call responses.
      public: void RecvSrvResponse();

      /// \brief Try to send all the requests for a given service call and a
      /// pair of request/response types.
      /// \param[in] _topic Topic name.
      /// \param[in] _reqType Type of the request in string format.
      /// \param[in] _repType Type of the response in string format.
      public: void SendPendingRemoteReqs(const std::string &_topic,
                                         const std::string &_reqType,
                                         const std::string &_repType);

      /// \brief Callback executed when the discovery detects new topics.
      /// \param[in] _pub Information of the publisher in charge of the topic.
      public: void OnNewConnection(const MessagePublisher &_pub);

      /// \brief Callback executed when the discovery detects disconnections.
      /// \param[in] _pub Information of the publisher in charge of the topic.
      public: void OnNewDisconnection(const MessagePublisher &_pub);

      /// \brief Callback executed when the discovery detects a new service call
      /// \param[in] _pub Information of the publisher in charge of the service.
      public: void OnNewSrvConnection(const ServicePublisher &_pub);

      /// \brief Callback executed when a service call is no longer available.
      /// \param[in] _pub Information of the publisher in charge of the service.
      public: void OnNewSrvDisconnection(const ServicePublisher &_pub);

      /// \brief Pass through to bool Publishers(const std::string &_topic,
      /// Addresses_M<Pub> &_publishers) const
      /// \param[in] _topic Service name.
      /// \param[out] _publishers Collection of service publishers.
      /// \return True if the service is found and
      //  there is at least one publisher.
      /// \sa bool Publishers(const std::string &_topic,
      /// Addresses_M<Pub> &_publishers) const
      public: bool TopicPublishers(const std::string &_topic,
                                   SrvAddresses_M &_publishers) const;

      /// \brief Pass through to bool Discovery::Discover(const std::string
      /// &_topic) const
      /// \param[in] _topic Service name.
      /// \return True if the method succeeded or false otherwise
      /// (e.g. if the discovery has not been started).
      /// \sa bool Discovery::Discover(const std::string &_topic) const
      public: bool DiscoverService(const std::string &_topic) const;

      /// \brief Pass through to bool Advertise(const Pub &_publisher)
      /// \param[in] _publisher Publisher's information to advertise.
      /// \return True if the method succeed or false otherwise
      /// (e.g. if the discovery has not been started).
      /// \sa Pass through to bool Advertise(const Pub &_publisher)
      public: bool AdvertisePublisher(const ServicePublisher &_publisher);

      /// \brief Constructor.
      protected: NodeShared();

      /// \brief Destructor.
      protected: virtual ~NodeShared();

      /// \brief Initialize all sockets.
      /// \return True when success or false otherwise. This function might
      /// return false if any operation on a ZMQ socket triggered an exception.
      private: bool InitializeSockets();

      /// \brief Timeout used for receiving messages (ms.).
      public: static const int Timeout = 250;

      //////////////////////////////////////////////////
      /////// Declare here other member variables //////
      //////////////////////////////////////////////////

      /// \brief Response receiver socket identity.
      public: Uuid responseReceiverId;

      /// \brief Replier socket identity.
      public: Uuid replierId;

      /// \brief Process UUID.
      public: std::string pUuid;

      /// \brief Timeout used for receiving requests.
      public: int timeout;

      /// \brief thread in charge of receiving and handling incoming messages.
      public: std::thread threadReception;

      /// \brief Mutex to guarantee exclusive access between all threads.
      public: mutable std::recursive_mutex mutex;

      /// \brief When true, the reception thread will finish.
      public: bool exit;

#ifdef _WIN32
      /// \brief True when the reception thread is finishing.
      public: bool threadReceptionExiting;
#endif

      /// \brief Port used by the message discovery layer.
      private: const int kMsgDiscPort = 11317;

      /// \brief Port used by the service discovery layer.
      private: const int kSrvDiscPort = 11318;

      /// \brief Mutex to guarantee exclusive access to the 'exit' variable.
      private: std::mutex exitMutex;

      /// \brief Remote connections for pub/sub messages.
      private: TopicStorage<MessagePublisher> connections;

      /// \brief List of connected zmq end points for request/response.
      private: std::vector<std::string> srvConnections;

      /// \brief Remote subscribers.
      public: TopicStorage<MessagePublisher> remoteSubscribers;

      /// \brief Ordinary local subscriptions.
      public: HandlerStorage<ISubscriptionHandler> localSubscriptions;

      /// \brief Raw local subscriptions. Keeping these separate from
      /// localSubscriptions allows us to avoid an unnecessary deserialization
      /// followed by an immediate reserialization.
      public: HandlerStorage<RawSubscriptionHandler> rawSubscriptions;

      /// \brief Service call repliers.
      public: HandlerStorage<IRepHandler> repliers;

      /// \brief Pending service call requests.
      public: HandlerStorage<IReqHandler> requests;

      /// \brief Print activity to stdout.
      public: int verbose;

      /// \brief My pub/sub address.
      public: std::string myAddress;

      /// \brief My pub/sub control address.
      public: std::string myControlAddress;

      /// \brief My requester service call address.
      public: std::string myRequesterAddress;

      /// \brief My replier service call address.
      public: std::string myReplierAddress;

      /// \brief IP address of this host.
      public: std::string hostAddr;

      /// \brief Internal data pointer.
      private: std::unique_ptr<NodeSharedPrivate> dataPtr;

      private: friend Node;
      private: friend NodePrivate;
    };
  }
}
#endif
