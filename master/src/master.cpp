/*
 * Copyright (c) 2015, Thomas Keh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "master.h"
#include "core.h"
#include "logger.h"

#include <memory>

namespace kaco {

Master::Master(const bool add_alive_callback) :
	m_id_first(0), m_id_last(0)
{
	m_aliveCallbackRegistered = add_alive_callback;
	m_device_alive_callback_functional = std::bind(&Master::device_alive_callback, this, std::placeholders::_1);
	if (add_alive_callback) {
		core.nmt.register_device_alive_callback(m_device_alive_callback_functional);
	}
}

Master::~Master() {
	if (m_running) {
		stop();
	}
}

bool Master::start(const std::string busname, const std::string& baudrate) {
	bool success = core.start(busname, baudrate);
	if (!success) {
		return false;
	}
	m_running = true;

	if (m_aliveCallbackRegistered)
		core.nmt.discover_nodes();

	return true;
}

bool Master::start(const std::string busname, const unsigned baudrate) {
	bool success = core.start(busname, baudrate);
	if (!success) {
		return false;
	}
	m_running = true;

	if (m_aliveCallbackRegistered)
		core.nmt.discover_nodes();

	return true;
}

void Master::stop() {
	m_running = false;
	core.stop();
}

size_t Master::num_devices() const {
	return m_devices.size();
}

Device& Master::get_device(size_t index) const {
	assert(m_devices.size()>index);
	return *(m_devices.at(index).get());
}

void Master::addDevice(Device* device, const bool overwrite) {
	if (!m_device_alive.test(device->get_node_id())) {
		m_devices.emplace_back(device);
	} else {
		if (overwrite) {
			removeDevice(device->get_node_id());
			m_devices.emplace_back(device);
			return;
		}
		WARN("Device with node ID "<<device->get_node_id()<<" already exists. Ignoring...");
	}
}

void Master::removeDevice(const size_t node_id) {
	for (size_t i = 0; i < m_devices.size(); i++) {
		const Device& dev = get_device(i);
		if (dev.get_node_id() == node_id) {
			m_devices.erase(m_devices.begin() + i);
			return;
		}
	}	
}

std::vector<uint8_t>& Master::discover_nodes(const size_t id_first, const size_t id_last) {
	const auto pause = std::chrono::milliseconds(CONSECUTIVE_SEND_PAUSE_MS);
	m_id_first = id_first;
	m_id_last = id_last > 238 ? 239 : id_last + 1;
	m_discoveredNodes.clear();
	m_device_callback_functional = std::bind(&Master::device_callback, this, std::placeholders::_1);
	core.register_receive_callback(m_device_callback_functional);
	for (size_t node_id = m_id_first; node_id < m_id_last; ++node_id) {
		// Protocol node guarding. See CiA 301. All devices will answer with their state via NMT.
		uint16_t cob_id = 0x600 + node_id;
		Message message = { cob_id, false, 8, {0x40,0,0x10,0,0,0,0,0} };
		core.send(message);
		std::this_thread::sleep_for(pause);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	core.unregister_receive_callback(m_device_callback_functional);
	m_id_first = m_id_last = 0;
	return m_discoveredNodes;
}

void Master::device_alive_callback(const uint8_t node_id) {
	if (!m_device_alive.test(node_id)) {
		m_device_alive.set(node_id);
		m_devices.emplace_back(new Device(core, node_id));
	} 
}

void Master::device_callback(const Message& message) {
	if (message.get_function_code() == 11) {
		if (message.len > 2 && (message.data[0] & 0x43) == 0x43 && message.data[1] == 0 && message.data[2] == 0x10) {
			uint8_t node_id = message.get_node_id();
			if (node_id > m_id_first && node_id < m_id_last) {
				for (uint8_t id : m_discoveredNodes) {
					if (id == node_id) {
						return;
					}
				}
			
				m_discoveredNodes.push_back(node_id);					
			}
		}
	}
}

} // end namespace kaco
