/*
 * MockBackend.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: al1
 */

#include "MockBackend.hpp"

using std::bind;
using std::string;
using std::to_string;

using namespace std::placeholders;

MockBackend::MockBackend(domid_t beDomId, domid_t feDomId) :
	mBeDomId(beDomId),
	mFeDomId(feDomId),
	mLog("MockBackend")
{
	XenStoreMock::writeValue("domid", to_string(mBeDomId));

	XenStoreMock::setDomainPath(mFeDomId, "/local/domain/" + to_string(mFeDomId));
	XenStoreMock::setDomainPath(mBeDomId, "/local/domain/" + to_string(mBeDomId));

	setupVdispl();
	setupVkbd();

	XenStoreMock::setWriteValueCbk(bind(&MockBackend::onWriteXenStore, this,
									_1, _2));

	LOG(mLog, DEBUG) << "Create";
}

void MockBackend::setupVdispl()
{
	mVdisplFePath = XenStoreMock::getDomainPath(mFeDomId);
	mVdisplFePath += "/device/vdispl/0";

	mVdisplBePath = XenStoreMock::getDomainPath(mBeDomId);
	mVdisplBePath += "/backend/vdispl/" + to_string(mFeDomId) + "/0";

	XenStoreMock::writeValue(mVdisplBePath + "/frontend", mVdisplFePath);

	XenStoreMock::writeValue(mVdisplFePath + "/state",
						 to_string(XenbusStateInitialising));
	XenStoreMock::writeValue(mVdisplBePath + "/state",
						 to_string(XenbusStateInitialising));

	XenStoreMock::writeValue(mVdisplFePath + "/0/id", "1000");
	XenStoreMock::writeValue(mVdisplFePath + "/0/resolution", "800x600");
}

void MockBackend::setupVkbd()
{
	mVkbdFePath = XenStoreMock::getDomainPath(mFeDomId);
	mVkbdFePath += "/device/vkbd/0";

	mVkbdBePath = XenStoreMock::getDomainPath(mBeDomId);
	mVkbdBePath += "/backend/vkbd/" + to_string(mFeDomId) + "/0";

	XenStoreMock::writeValue(mVkbdBePath + "/frontend", mVkbdFePath);

	XenStoreMock::writeValue(mVkbdFePath + "/state",
						 to_string(XenbusStateInitialising));
	XenStoreMock::writeValue(mVkbdBePath + "/state",
						 to_string(XenbusStateInitialising));

	XenStoreMock::writeValue(mVkbdFePath + "/id", "k:0;p:0;t:0");
}

void MockBackend::onWriteXenStore(const string& path, const string& value)
{
	string vdisplStatePath = mVdisplBePath + "/state";
	string vkbdStatePath = mVkbdBePath + "/state";

	if (path == vdisplStatePath)
	{
		onVdisplBeStateChanged(static_cast<XenbusState>(stoi(value)));
	}

	if (path == vkbdStatePath)
	{
		onVkbdBeStateChanged(static_cast<XenbusState>(stoi(value)));
	}
}

void MockBackend::onVdisplBeStateChanged(XenbusState state)
{
	switch(state)
	{
	case XenbusStateInitialising:
		mAsync.call(bind(&MockBackend::setVdisplFeState, this,
					XenbusStateInitialising));
		break;
	case XenbusStateInitWait:
		mAsync.call(bind(&MockBackend::setVdisplFeState, this,
					XenbusStateInitialised));
		break;
	default:
		break;
	}
}

void MockBackend::setVdisplFeState(XenbusState state)
{
	LOG(mLog, DEBUG) << "Set vdispl FE state: " << state;

	if (state == XenbusStateInitialised)
	{
		XenStoreMock::writeValue(mVdisplFePath + "/0/evt-event-channel", "1");
		XenStoreMock::writeValue(mVdisplFePath + "/0/evt-ring-ref", "100");

		XenStoreMock::writeValue(mVdisplFePath + "/0/req-event-channel", "2");
		XenStoreMock::writeValue(mVdisplFePath + "/0/req-ring-ref", "200");
	}

	XenStoreMock::writeValue(mVdisplFePath + "/state", to_string(state));
}

void MockBackend::onVkbdBeStateChanged(XenbusState state)
{
	switch(state)
	{
	case XenbusStateInitialising:
		mAsync.call(bind(&MockBackend::setVkbdFeState, this,
					XenbusStateInitialising));
		break;
	case XenbusStateInitWait:
		mAsync.call(bind(&MockBackend::setVkbdFeState, this,
					XenbusStateInitialised));
		break;
	default:
		break;
	}
}

void MockBackend::setVkbdFeState(XenbusState state)
{
	LOG(mLog, DEBUG) << "Set vkbd FE state: " << state;

	if (state == XenbusStateInitialised)
	{
		XenStoreMock::writeValue(mVkbdFePath + "/event-channel", "5");
		XenStoreMock::writeValue(mVkbdFePath + "/page-gref", "500");
	}

	XenStoreMock::writeValue(mVkbdFePath + "/state", to_string(state));
}

