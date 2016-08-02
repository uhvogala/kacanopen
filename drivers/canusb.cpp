/*
 * Copyright (c) 2016 Adrian Weiler
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

//------------------------------------------------------//
// This is a driver for FTDI-based CAN <-> USB devices. //
// Windows only. Happy Linuxers have slcan for this ;-) //
//------------------------------------------------------//

#include <windows.h>
// windows.h defines ERROR, we use that in a different way
#undef ERROR

#include "message.h"
#include "logger.h"

#include <chrono>
#include <string>
#include <map>
#include <thread>


/// CAN message type
using kaco::Message;


namespace {
	/**
	 * kudos: http://hdrlab.org.nz/articles/windows-development/a-c-class-for-controlling-a-comm-port-in-windows/
	 * A very simple serial port control class that does NOT require MFC / AFX.
	 *
	 * License : This source code can be used and / or modified without restrictions.
	 * It is provided as is and the author disclaims all warranties, expressed
	 * or implied, including, without limitation, the warranties of
	 * merchantability and of fitness for any purpose.The user must assume the
	 * entire risk of using the Software.
	 *
	 * @author Hans de Ruiter
	 *
	 * @version 0.1 -- 28 October 2008
	 */
	class Serial {
		HANDLE commHandle;

	public:
		Serial(char const* commPortName, int bitRate = 115200)
		{
			commHandle = CreateFile(commPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

			if (commHandle == INVALID_HANDLE_VALUE) {
				throw ("ERROR: Could not open com port");
			} else {
				// set timeouts. Changed to use 100ms timeout (was: MAXDWORD)
				COMMTIMEOUTS cto = { 100, 0, 0, 0, 0 };
				DCB dcb;

				if (!SetCommTimeouts(commHandle, &cto)) {
					throw ("ERROR: Could not set com port time-outs");
				}

				// set DCB
				memset(&dcb, 0, sizeof(dcb));
				dcb.DCBlength = sizeof(dcb);
				dcb.BaudRate = bitRate;
				dcb.fBinary = 1;
				dcb.fDtrControl = DTR_CONTROL_ENABLE;
				dcb.fRtsControl = RTS_CONTROL_ENABLE;

				dcb.Parity = NOPARITY;
				dcb.StopBits = ONESTOPBIT;
				dcb.ByteSize = 8;

				if (!SetCommState(commHandle, &dcb)) {
					throw ("ERROR: Could not set com port parameters");
				}
			}
		}


		virtual ~Serial()
		{
			CloseHandle(commHandle);
		}

		/**
		 * Writes a NULL terminated string.
		 *
		 * @param buffer the string to send
		 *
		 * @return int the number of characters written
		 */
		int write(const char buffer[]) noexcept
		{
			DWORD numWritten;
			WriteFile(commHandle, buffer, strlen(buffer), &numWritten, NULL);

			return numWritten;
		}

		/*
		 * Writes a string of bytes to the serial port.
		 *
		 * @param buffer pointer to the buffer containing the bytes
		 * @param buffLen the number of bytes in the buffer
		 *
		 * @return int the number of bytes written
		 */
		int write(const char* buffer, int buffLen) noexcept
		{
			DWORD numWritten;
			WriteFile(commHandle, buffer, buffLen, &numWritten, NULL);

			return numWritten;
		}

		/** Reads a string of bytes from the serial port.
		 *
		 * @param buffer pointer to the buffer to be written to
		 * @param buffLen the size of the buffer
		 * @param nullTerminate if set to true it will null terminate the string
		 *
		 * @return int the number of bytes read
		 */
		int read(char* buffer, int buffLen, bool nullTerminate = true) noexcept
		{
			DWORD numRead;

			if (nullTerminate) {
				--buffLen;
			}

			BOOL ret = ReadFile(commHandle, buffer, buffLen, &numRead, NULL);

			if (!ret) {
				return 0;
			}

			if (nullTerminate) {
				buffer[numRead] = '\0';
			}

			return numRead;
		}
	};

}

/// This struct contains C-strings for
/// busname and baudrate
struct CANBoard {

	/// Bus name
	const char* busname;

	/// Baudrate
	const char* baudrate;

};


/// Handle type which should represent the driver instance.
/// You can just some constant != 0 if only one instance is supported.
/// 0 is interpreted as failed initialization.
/// KaCanOpen uses just one instance.
using CANHandle = Serial*;


/// Change the bus baudrate.
/// The baudrate is given as a C-string.
/// Supported values are 1M, 500K, 250K, 125K, 100K, 50K, 20K, 10K, 5K, and none.
/// Return 0 on success.
extern "C" uint8_t __declspec(dllexport) canChangeBaudRate_driver(CANHandle handle, char const* baudrate)
{
	PRINT("canChangeBaudRate_driver: " << baudrate);

	static std::map<std::string, int> baudRates = {
		{ "10K", 0 },
		{ "10000", 0 },
		{ "20K", 1 },
		{ "20000", 1 },
		{ "50K", 2 },
		{ "50000", 2 },
		{ "100K", 3 },
		{ "100000", 3 },
		{ "125K", 4 },
		{ "125000", 4 },
		{ "250K", 5 },
		{ "250000", 5 },
		{ "500K", 6 },
		{ "500000", 6 },
		{ "800K", 7},
		{ "800000", 7 },
		{ "1M", 8},
		{ "1000000", 8 }
	};

	auto it = baudRates.find(baudrate);

	if (it == baudRates.end()) {
		return 1;
	}

	// Set baud rate and start the interface
	char buf[10];
	std::snprintf(buf, sizeof buf, "C\rS%d\rO\r", (*it).second);
	handle->write(buf);
	return 0;
}


/// Initialize the driver and return some handle.
/// The board argument can be used for configuration.
extern "C" CANHandle __declspec(dllexport) canOpen_driver(CANBoard* board)
{
	PRINT("canOpen_driver");

	try {
		// Note: Plain 'COMn' works only for n = {1..9}. General syntax for COM-devices under Windows is: '\\.\COMn'
		auto serialDevice = new Serial((std::string{ "\\\\.\\" } +board->busname).c_str());

		// Trigger serial line auto-baud detection by sending some characters.
		serialDevice->write("\r\r\r");

		canChangeBaudRate_driver(serialDevice, board->baudrate);

		return (CANHandle)serialDevice;
	} catch (char const*) {
		return 0;
	}
}

/// Destruct the driver.
/// Return 0 on success.
extern "C" int32_t __declspec(dllexport) canClose_driver(CANHandle handle)
{
	PRINT("canClose_driver");

	if (handle == nullptr) {
		return -1;
	}

	handle->write("C\r");

	delete handle;
	return 0;
}


/// Receive a message.
/// This should be a blocking call and wait for any message.
/// Return 0 on success.
extern "C" uint8_t __declspec(dllexport) canReceive_driver(CANHandle handle, Message* message)
{
	//PRINT("canReceive_driver");
	std::memset(message->data, 0, 8);

	enum {
		nothing,
		bff,
		eff
	} received = nothing;

	char buf[30];

	// will be initialized below
	int len;

	do {
		message->rtr = 0;
		len = handle->read(buf, sizeof buf);

		if ((--len > 0) && (buf[len] == '\r')) {
			buf[len] = 0;

			auto p = buf;

			switch (*p++) {
				case 'r':
					message->rtr = 1;

				// Intended fall through to next case
				case 't':
					received = bff;
					break;

				case 'R':
					message->rtr = 1;

				// Intended fall through to next case
				case 'T':
					received = eff;
					break;

				default:
					received = nothing;
					continue;
			}

			auto dlcPos = received == bff ? 3 : 8;

			if ((len < dlcPos) || (p[dlcPos] < '0') || (p[dlcPos] > '8')) {
				received = nothing;
				continue;
			}

			auto dlc = p[dlcPos] & 0x0f;

			auto requiredLen = (message->rtr ? dlcPos : dlcPos + dlc * 2) + 1;

			if (len < requiredLen) {
				received = nothing;
				continue;
			}

			// Make terminator for COB-ID
			p[dlcPos] = 0;

			char* endPtr = nullptr;
			message->cob_id = static_cast < decltype(message->cob_id) > (::strtoul(p, &endPtr, 16));

			if (endPtr != &p[dlcPos]) {
				received = nothing;
				continue;
			}

			p = endPtr + 1;

			auto payload = ::strtoull(p, &endPtr, 16);

			message->len = dlc;

			while (dlc > 0) {
				message->data[--dlc] = payload & 0xff;
				payload >>= 8;
			}
		}
	} while (received == nothing);


	return 0;
}

/// Send a message
/// Return 0 on success.
extern "C" uint8_t __declspec(dllexport) canSend_driver(CANHandle handle, Message const* message)
{
	PRINT("Transmitting PDO, len " << message->len << " data " << std::hex << message->data[0] << std::dec);

	if (message->len > sizeof message->data) {
		return 1;
	}

	(void) handle;

	char buf[30];

	// kaco doesn't support eff, so always transmit bff
	std::snprintf(buf, sizeof buf, "%c%03x%1x", message->rtr ? 'r' : 't',  message->cob_id, message->len);


	auto p = &buf[5];
	auto end = buf + sizeof buf;

	for (auto i = 0; i < message->len; ++i) {
		std::snprintf(p, end - p, "%02x", message->data[i]);
		p += 2;
	}

	*p++ = '\r';
	handle->write(buf, p - buf);
	return 0;
}
