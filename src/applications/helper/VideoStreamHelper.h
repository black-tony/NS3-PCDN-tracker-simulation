/**
 * Author: Yinghao Yang <blacktonyrl@gmail.com>
 * 
*/
#ifndef VIDEO_STREAM_HELPER_H
#define VIDEO_STREAM_HELPER_H
#include <ns3/object-factory.h>
#include <ns3/node-container.h>
#include <ns3/application-container.h>


namespace ns3 {


/**
 * \ingroup applications
 * Helper to make it easier to instantiate an VideoStreamClient on a set of nodes.
 */
class VideoStreamClientHelper
{
public:
  /**
   * Create a VideoStreamClientHelper to make it easier to work with VideoStreamClient
   * applications.
   * \param address The address of the remote server node to send traffic to.
   */
  VideoStreamClientHelper (const Address &address);

  /**
   * Helper function used to set the underlying application attributes, but
   * *not* the socket attributes.
   * \param name The name of the application attribute to set.
   * \param value The value of the application attribute to set.
   */
  void SetAttribute (const std::string &name,
                     const AttributeValue &value);

  /**
   * Install a VideoStreamClient on each node of the input container configured with
   * all the attributes set with SetAttribute().
   * \param c NodeContainer of the set of nodes on which an VideoStreamClient
   *          will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install a VideoStreamClient on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param node The node on which an VideoStreamClient will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install a VideoStreamClient on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param nodeName The name of the node on which an VideoStreamClient
   *                 will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

private:
  /**
   * \internal
   * Install a VideoStreamClient on the node configured with all the
   * attributes set with SetAttribute().
   * \param node The node on which an VideoStreamClient will be installed.
   * \return Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  /// Used to instantiate an VideoStreamClient instance.
  ObjectFactory m_factory;

}; // end of `class VideoStreamClientHelper`



/**
 * \ingroup http
 * Helper to make it easier to instantiate an VideoStreamServer on a set of nodes.
 */
class VideoStreamServerHelper
{
public:
  /**
   * Create a VideoStreamServerHelper to make it easier to work with
   * VideoStreamServer applications.
   * \param address The address of the server.
   */
  VideoStreamServerHelper (const Address &address);

  /**
   * Helper function used to set the underlying application attributes, but
   * *not* the socket attributes.
   * \param name The name of the application attribute to set.
   * \param value The value of the application attribute to set.
   */
  void SetAttribute (const std::string &name,
                     const AttributeValue &value);

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param c NodeContainer of the set of nodes on which an VideoStreamServer
   *              will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param node The node on which an VideoStreamServer will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param nodeName The name of the node on which an VideoStreamServer
   *                             will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

private:
  /**
   * \internal
   * Install an VideoStreamServer on the node configured with all the
   * attributes set with SetAttribute().
   * \param node The node on which an VideoStreamServer will be installed.
   * \return Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  /// Used to instantiate a VideoStreamServer instance.
  ObjectFactory m_factory;

}; // end of `class VideoStreamServerHelper`
/**
 * \ingroup http
 * Helper to make it easier to instantiate an VideoStreamServer on a set of nodes.
 */
class VideoStreamPCDNHelper
{
public:
  /**
   * Create a VideoStreamServerHelper to make it easier to work with
   * VideoStreamServer applications.
   * \param address The address of the server.
   */
  VideoStreamPCDNHelper (const Address &localAddress,const Address& P2PAddress, const Address &remoteServerAddress);

  /**
   * Helper function used to set the underlying application attributes, but
   * *not* the socket attributes.
   * \param name The name of the application attribute to set.
   * \param value The value of the application attribute to set.
   */
  void SetAttribute (const std::string &name,
                     const AttributeValue &value);

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param c NodeContainer of the set of nodes on which an VideoStreamServer
   *              will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param node The node on which an VideoStreamServer will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an VideoStreamServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param nodeName The name of the node on which an VideoStreamServer
   *                             will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

private:
  /**
   * \internal
   * Install an VideoStreamServer on the node configured with all the
   * attributes set with SetAttribute().
   * \param node The node on which an VideoStreamServer will be installed.
   * \return Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  /// Used to instantiate a VideoStreamServer instance.
  ObjectFactory m_factory;

}; // end of `class VideoStreamServerHelper`

} // end of `namespace ns3`

#endif