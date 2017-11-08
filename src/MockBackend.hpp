/*
 * MockBackend.hpp
 *
 *  Created on: Nov 8, 2017
 *      Author: al1
 */

#ifndef SRC_MOCKBACKEND_HPP_
#define SRC_MOCKBACKEND_HPP_

#include <XenStoreMock.hpp>

#include <xen/be/Log.hpp>
#include <xen/be/Utils.hpp>

extern "C" {
#include <xenctrl.h>
#include <xen/io/xenbus.h>
}

class MockBackend
{
public:
	MockBackend(domid_t beDomId, domid_t feDomId);

private:
	domid_t mBeDomId;
	domid_t mFeDomId;
	XenBackend::AsyncContext mAsync;
	XenBackend::Log mLog;

	std::string mVdisplFePath;
	std::string mVdisplBePath;

	std::string mVkbdFePath;
	std::string mVkbdBePath;

	void setupVdispl();
	void setupVkbd();

	void onWriteXenStore(const std::string& path, const std::string& value);

	void onVdisplBeStateChanged(XenbusState state);
	void setVdisplFeState(XenbusState state);

	void onVkbdBeStateChanged(XenbusState state);
	void setVkbdFeState(XenbusState state);
};

#endif /* SRC_MOCKBACKEND_HPP_ */
