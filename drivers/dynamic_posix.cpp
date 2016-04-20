/*
 * Copyright (c) 2016 Thomas Keh
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

//
// This is a meta driver which links 
//

//#include "message.h"
#include "logger.h"

#include <cstdlib>
#include <string>
#include <cassert>
#include <dlfcn.h>
#include <mutex>

//extern "C" typedef kaco::Message Message;

/// This struct represents a CANOpen message.
extern "C" struct Message {

	/// Message ID aka COB-ID
	uint16_t cob_id;

	/// Remote transmission request (0 if it's not an RTR message, 1 if it is an RTR message)
	uint8_t rtr;

	/// Message's length (0 to 8)
	uint8_t len;

	/// Data bytes
	uint8_t data[8];

};

extern "C" struct CANBoard {
	const char * busname;
	const char * baudrate;
};

struct Instance {

	void* lib = nullptr;
	void* handle = nullptr;

	void*   (*open)            (CANBoard*);
	int32_t (*close)           (void*);
	uint8_t (*receive)         (void*, Message*);
	uint8_t (*send)            (void*, Message const*);
	uint8_t (*change_baudrate) (void*, char*);

};

template<typename F>
bool link(void* lib, F* function, const std::string& name) {
	assert(lib);
	*(void **) function = dlsym(lib, name.c_str());
	char* error = dlerror();
	if (error) {
		ERROR("Cannot find function \""<<name<<"\".");
		return false;
	}
	ERROR(dlerror());
	return true;
}

extern "C" void* canOpen_driver(CANBoard* board) {

	Instance* instance = new Instance;

	const char* path = std::getenv("CAN");
	if (!path) {
		ERROR("Please set the environment variable CAN to the path to the driver to use.")
		return nullptr;
	}

	ERROR(dlerror());

	instance->lib = dlopen(path, RTLD_LAZY);
	//lib = dlopen(path, RTLD_NOW);
	if (!instance->lib) {
		ERROR("Library \""<<path<<"\" cannot be opened.");
		ERROR("dlerror() = "<<dlerror());
		return nullptr;
	}

	ERROR(dlerror());

	if (!link(instance->lib, &instance->open, "canOpen_driver")) {
		dlclose(instance->lib);
		instance->lib = nullptr;
		return nullptr;
	}

	if (!link(instance->lib, &instance->close, "canClose_driver")) {
		dlclose(instance->lib);
		instance->lib = nullptr;
		return nullptr;
	}

	if (!link(instance->lib, &instance->receive, "canReceive_driver")) {
		dlclose(instance->lib);
		instance->lib = nullptr;
		return nullptr;
	}

	if (!link(instance->lib, &instance->send, "canSend_driver")) {
		dlclose(instance->lib);
		instance->lib = nullptr;
		return nullptr;
	}

	if (!link(instance->lib, &instance->change_baudrate, "canChangeBaudRate_driver")) {
		dlclose(instance->lib);
		instance->lib = nullptr;
		return nullptr;
	}
	
	ERROR(dlerror());

	instance->handle = instance->open(board);
	return instance;

}

extern "C" int32_t canClose_driver(void* handle) {
	assert(handle);
	Instance* instance = (Instance*) handle;
	int32_t result = instance->close(instance->handle);
	dlclose(instance->lib);
	delete instance;
	handle = nullptr;
	return result;
}

extern "C" uint8_t canReceive_driver(void* handle, Message* message) {
	assert(handle);
	Instance* instance = (Instance*) handle;
	return instance->receive(instance->handle, message);
}

extern "C" uint8_t canSend_driver(void* handle, Message const* message) {
	assert(handle);
	Instance* instance = (Instance*) handle;
	return instance->send(instance->handle, message);
}

extern "C" uint8_t canChangeBaudRate_driver(void* handle, char* baudrate) {
	assert(handle);
	Instance* instance = (Instance*) handle;
	return instance->change_baudrate(instance->handle, baudrate);
}