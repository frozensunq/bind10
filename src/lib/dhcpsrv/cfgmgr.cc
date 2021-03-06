// Copyright (C) 2012-2014 Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <asiolink/io_address.h>
#include <dhcp/iface_mgr.h>
#include <dhcp/libdhcp++.h>
#include <dhcpsrv/cfgmgr.h>
#include <dhcpsrv/dhcpsrv_log.h>
#include <string>

using namespace isc::asiolink;
using namespace isc::util;

namespace isc {
namespace dhcp {

CfgMgr&
CfgMgr::instance() {
    static CfgMgr cfg_mgr;
    return (cfg_mgr);
}

void
CfgMgr::addOptionSpace4(const OptionSpacePtr& space) {
    if (!space) {
        isc_throw(InvalidOptionSpace,
                  "provided option space object is NULL.");
    }
    OptionSpaceCollection::iterator it = spaces4_.find(space->getName());
    if (it != spaces4_.end()) {
        isc_throw(InvalidOptionSpace, "option space " << space->getName()
                  << " already added.");
    }
    spaces4_.insert(make_pair(space->getName(), space));
}

void
CfgMgr::addOptionSpace6(const OptionSpacePtr& space) {
    if (!space) {
        isc_throw(InvalidOptionSpace,
                  "provided option space object is NULL.");
    }
    OptionSpaceCollection::iterator it = spaces6_.find(space->getName());
    if (it != spaces6_.end()) {
        isc_throw(InvalidOptionSpace, "option space " << space->getName()
                  << " already added.");
    }
    spaces6_.insert(make_pair(space->getName(), space));
}

void
CfgMgr::addOptionDef(const OptionDefinitionPtr& def,
                     const std::string& option_space) {
    // @todo we need better validation of the provided option space name here.
    // This will be implemented when #2313 is merged.
    if (option_space.empty()) {
        isc_throw(BadValue, "option space name must not be empty");
    } else if (!def) {
        // Option definition must point to a valid object.
        isc_throw(MalformedOptionDefinition, "option definition must not be NULL");

    } else if (getOptionDef(option_space, def->getCode())) {
        // Option definition must not be overriden.
        isc_throw(DuplicateOptionDefinition, "option definition already added"
                  << " to option space " << option_space);

    // We must not override standard (assigned) option for which there is a
    // definition in libdhcp++. The standard options belong to dhcp4 or dhcp6
    // option space.
    } else if ((option_space == "dhcp4" &&
                LibDHCP::isStandardOption(Option::V4, def->getCode()) &&
                LibDHCP::getOptionDef(Option::V4, def->getCode())) ||
               (option_space == "dhcp6" &&
                LibDHCP::isStandardOption(Option::V6, def->getCode()) &&
                LibDHCP::getOptionDef(Option::V6, def->getCode()))) {
        isc_throw(BadValue, "unable to override definition of option '"
                  << def->getCode() << "' in standard option space '"
                  << option_space << "'.");

    }
    // Actually add a new item.
    option_def_spaces_.addItem(def, option_space);
}

OptionDefContainerPtr
CfgMgr::getOptionDefs(const std::string& option_space) const {
    // @todo Validate the option space once the #2313 is implemented.

    return (option_def_spaces_.getItems(option_space));
}

OptionDefinitionPtr
CfgMgr::getOptionDef(const std::string& option_space,
                     const uint16_t option_code) const {
    // @todo Validate the option space once the #2313 is implemented.

    // Get a reference to option definitions for a particular option space.
    OptionDefContainerPtr defs = getOptionDefs(option_space);
    // If there are no matching option definitions then return the empty pointer.
    if (!defs || defs->empty()) {
        return (OptionDefinitionPtr());
    }
    // If there are some option definitions for a particular option space
    // use an option code to get the one we want.
    const OptionDefContainerTypeIndex& idx = defs->get<1>();
    const OptionDefContainerTypeRange& range = idx.equal_range(option_code);
    // If there is no definition that matches option code, return empty pointer.
    if (std::distance(range.first, range.second) == 0) {
        return (OptionDefinitionPtr());
    }
    // If there is more than one definition matching an option code, return
    // the first one. This should not happen because we check for duplicates
    // when addOptionDef is called.
    return (*range.first);
}

Subnet6Ptr
CfgMgr::getSubnet6(const std::string& iface,
                   const isc::dhcp::ClientClasses& classes) {

    if (!iface.length()) {
        return (Subnet6Ptr());
    }

    // If there is more than one, we need to choose the proper one
    for (Subnet6Collection::iterator subnet = subnets6_.begin();
         subnet != subnets6_.end(); ++subnet) {

        // If client is rejected because of not meeting client class criteria...
        if (!(*subnet)->clientSupported(classes)) {
            continue;
        }

        if (iface == (*subnet)->getIface()) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
                      DHCPSRV_CFGMGR_SUBNET6_IFACE)
                .arg((*subnet)->toText()).arg(iface);
            return (*subnet);
        }
    }
    return (Subnet6Ptr());
}

Subnet6Ptr
CfgMgr::getSubnet6(const isc::asiolink::IOAddress& hint,
                   const isc::dhcp::ClientClasses& classes,
                   const bool relay) {

    // If there is more than one, we need to choose the proper one
    for (Subnet6Collection::iterator subnet = subnets6_.begin();
         subnet != subnets6_.end(); ++subnet) {

        // If client is rejected because of not meeting client class criteria...
        if (!(*subnet)->clientSupported(classes)) {
            continue;
        }

        // If the hint is a relay address, and there is relay info specified
        // for this subnet and those two match, then use this subnet.
        if (relay && ((*subnet)->getRelayInfo().addr_ == hint) ) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
                      DHCPSRV_CFGMGR_SUBNET6_RELAY)
                .arg((*subnet)->toText()).arg(hint.toText());
            return (*subnet);
        }

        if ((*subnet)->inRange(hint)) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_SUBNET6)
                      .arg((*subnet)->toText()).arg(hint.toText());
            return (*subnet);
        }
    }

    // sorry, we don't support that subnet
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_NO_SUBNET6)
              .arg(hint.toText());
    return (Subnet6Ptr());
}

Subnet6Ptr CfgMgr::getSubnet6(OptionPtr iface_id_option,
                              const isc::dhcp::ClientClasses& classes) {
    if (!iface_id_option) {
        return (Subnet6Ptr());
    }

    // Let's iterate over all subnets and for those that have interface-id
    // defined, check if the interface-id is equal to what we are looking for
    for (Subnet6Collection::iterator subnet = subnets6_.begin();
         subnet != subnets6_.end(); ++subnet) {

        // If client is rejected because of not meeting client class criteria...
        if (!(*subnet)->clientSupported(classes)) {
            continue;
        }

        if ( (*subnet)->getInterfaceId() &&
             ((*subnet)->getInterfaceId()->equal(iface_id_option))) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
                      DHCPSRV_CFGMGR_SUBNET6_IFACE_ID)
                .arg((*subnet)->toText());
            return (*subnet);
        }
    }
    return (Subnet6Ptr());
}

void CfgMgr::addSubnet6(const Subnet6Ptr& subnet) {
    /// @todo: Check that this new subnet does not cross boundaries of any
    /// other already defined subnet.
    /// @todo: Check that there is no subnet with the same interface-id
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_ADD_SUBNET6)
              .arg(subnet->toText());
    subnets6_.push_back(subnet);
}

Subnet4Ptr
CfgMgr::getSubnet4(const isc::asiolink::IOAddress& hint,
                   const isc::dhcp::ClientClasses& classes,
                   bool relay) const {
    // Iterate over existing subnets to find a suitable one for the
    // given address.
    for (Subnet4Collection::const_iterator subnet = subnets4_.begin();
         subnet != subnets4_.end(); ++subnet) {

        // If client is rejected because of not meeting client class criteria...
        if (!(*subnet)->clientSupported(classes)) {
            continue;
        }

        // If the hint is a relay address, and there is relay info specified
        // for this subnet and those two match, then use this subnet.
        if (relay && ((*subnet)->getRelayInfo().addr_ == hint) ) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
                      DHCPSRV_CFGMGR_SUBNET4_RELAY)
                .arg((*subnet)->toText()).arg(hint.toText());
            return (*subnet);
        }

        // Let's check if the client belongs to the given subnet
        if ((*subnet)->inRange(hint)) {
            LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
                      DHCPSRV_CFGMGR_SUBNET4)
                      .arg((*subnet)->toText()).arg(hint.toText());
            return (*subnet);
        }
    }

    // sorry, we don't support that subnet
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_NO_SUBNET4)
              .arg(hint.toText());
    return (Subnet4Ptr());
}

Subnet4Ptr
CfgMgr::getSubnet4(const std::string& iface_name,
                   const isc::dhcp::ClientClasses& classes) const {
    Iface* iface = IfaceMgr::instance().getIface(iface_name);
    // This should never happen in the real life. Hence we throw an exception.
    if (iface == NULL) {
        isc_throw(isc::BadValue, "interface " << iface_name <<
                  " doesn't exist and therefore it is impossible"
                  " to find a suitable subnet for its IPv4 address");
    }
    IOAddress addr("0.0.0.0");
    // If IPv4 address assigned to the interface exists, find a suitable
    // subnet for it, else return NULL pointer to indicate that no subnet
    // could be found.
    return (iface->getAddress4(addr) ? getSubnet4(addr, classes) : Subnet4Ptr());
}

void CfgMgr::addSubnet4(const Subnet4Ptr& subnet) {
    /// @todo: Check that this new subnet does not cross boundaries of any
    /// other already defined subnet.
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_ADD_SUBNET4)
              .arg(subnet->toText());
    subnets4_.push_back(subnet);
}

void CfgMgr::deleteOptionDefs() {
    option_def_spaces_.clearItems();
}

void CfgMgr::deleteSubnets4() {
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_DELETE_SUBNET4);
    subnets4_.clear();
}

void CfgMgr::deleteSubnets6() {
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_DELETE_SUBNET6);
    subnets6_.clear();
}


std::string CfgMgr::getDataDir() {
    return (datadir_);
}

void
CfgMgr::addActiveIface(const std::string& iface) {

    size_t pos = iface.find("/");
    std::string iface_copy = iface;

    if (pos != std::string::npos) {
        std::string addr_string = iface.substr(pos + 1);
        try {
            IOAddress addr(addr_string);
            iface_copy = iface.substr(0,pos);
            unicast_addrs_.insert(make_pair(iface_copy, addr));
        } catch (...) {
            isc_throw(BadValue, "Can't convert '" << addr_string
                      << "' into address in interface defition ('"
                      << iface << "')");
        }
    }

    if (isIfaceListedActive(iface_copy)) {
        isc_throw(DuplicateListeningIface,
                  "attempt to add duplicate interface '" << iface_copy << "'"
                  " to the set of interfaces on which server listens");
    }
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE, DHCPSRV_CFGMGR_ADD_IFACE)
        .arg(iface_copy);
    active_ifaces_.push_back(iface_copy);
}

void
CfgMgr::activateAllIfaces() {
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
              DHCPSRV_CFGMGR_ALL_IFACES_ACTIVE);
    all_ifaces_active_ = true;
}

void
CfgMgr::deleteActiveIfaces() {
    LOG_DEBUG(dhcpsrv_logger, DHCPSRV_DBG_TRACE,
              DHCPSRV_CFGMGR_CLEAR_ACTIVE_IFACES);
    active_ifaces_.clear();
    all_ifaces_active_ = false;

    unicast_addrs_.clear();
}

bool
CfgMgr::isActiveIface(const std::string& iface) const {

    // @todo Verify that the interface with the specified name is
    // present in the system.

    // If all interfaces are marked active, there is no need to check that
    // the name of this interface has been explicitly listed.
    if (all_ifaces_active_) {
        return (true);
    }
    return (isIfaceListedActive(iface));
}

bool
CfgMgr::isIfaceListedActive(const std::string& iface) const {
    for (ActiveIfacesCollection::const_iterator it = active_ifaces_.begin();
         it != active_ifaces_.end(); ++it) {
        if (iface == *it) {
            return (true);
        }
    }
    return (false);
}

const isc::asiolink::IOAddress*
CfgMgr::getUnicast(const std::string& iface) const {
    UnicastIfacesCollection::const_iterator addr = unicast_addrs_.find(iface);
    if (addr == unicast_addrs_.end()) {
        return (NULL);
    }
    return (&(*addr).second);
}

void
CfgMgr::setD2ClientConfig(D2ClientConfigPtr& new_config) {
    d2_client_mgr_.setD2ClientConfig(new_config);
}

bool
CfgMgr::ddnsEnabled() {
    return (d2_client_mgr_.ddnsEnabled());
}

const D2ClientConfigPtr&
CfgMgr::getD2ClientConfig() const {
    return (d2_client_mgr_.getD2ClientConfig());
}

D2ClientMgr&
CfgMgr::getD2ClientMgr() {
    return (d2_client_mgr_);
}

CfgMgr::CfgMgr()
    : datadir_(DHCP_DATA_DIR),
      all_ifaces_active_(false), echo_v4_client_id_(true),
      d2_client_mgr_() {
    // DHCP_DATA_DIR must be set set with -DDHCP_DATA_DIR="..." in Makefile.am
    // Note: the definition of DHCP_DATA_DIR needs to include quotation marks
    // See AM_CPPFLAGS definition in Makefile.am
}

CfgMgr::~CfgMgr() {
}

}; // end of isc::dhcp namespace
}; // end of isc namespace
