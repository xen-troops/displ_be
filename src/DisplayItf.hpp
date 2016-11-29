/*
 *  Display Interface
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#ifndef SRC_DISPLAYITF_HPP_
#define SRC_DISPLAYITF_HPP_

#include <memory>

/***************************************************************************//**
 * @defgroup display_itf Display interface
 * Abstract classes for display implementation.
 ******************************************************************************/

/***************************************************************************//**
 * Exception generated by Drm.
 * @ingroup display_itf
 ******************************************************************************/
class DisplayItfException : public std::exception
{
public:
	/**
	 * @param msg error message
	 */
	explicit DisplayItfException(const std::string& msg) : mMsg(msg) {};
	virtual ~DisplayItfException() {}

	/**
	 * returns error message
	 */
	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

/***************************************************************************//**
 * Provides display buffer functionality.
 * @ingroup display_itf
 ******************************************************************************/
class DisplayBufferItf
{
public:

	virtual ~DisplayBufferItf() {}

	/**
	 * Returns buffer size
	 */
	virtual size_t getSize() const = 0;

	/**
	 * Returns pointer to the data buffer
	 */
	virtual void* getBuffer() const = 0;
};

/***************************************************************************//**
 * Provides frame buffer functionality.
 * @ingroup display_itf
 ******************************************************************************/
class FrameBufferItf
{
public:

	virtual ~FrameBufferItf() {};

	/**
	 * Returns pointer to the display buffer
	 */
	virtual std::shared_ptr<DisplayBufferItf> getDisplayBuffer() = 0;
};

/***************************************************************************//**
 * Provides connector functionality.
 * @ingroup display_itf
 ******************************************************************************/
class ConnectorItf
{
public:

	/**
	 * Callback which is called when page flip is done
	 */
	typedef std::function<void()> FlipCallback;

	/**
	 * @param conId connector id
	 */
	ConnectorItf(int conId) : mConId(conId) {}
	virtual ~ConnectorItf() {};

	/**
	 * Returns connector id
	 */
	uint32_t getId() const { return mConId; }

	/**
	 * Checks if the connector is connected
	 * @return <i>true</i> if connected
	 */
	virtual bool isConnected() const = 0;

	/**
	 * Checks if the connector is initialized
	 * @return <i>true</i> if initialized
	 */
	virtual bool isInitialized() const = 0;

	/**
	 * Initializes connector
	 * @param x           horizontal offset
	 * @param y           vertical offset
	 * @param width       width
	 * @param height      height
	 * @param bpp         bits per pixel
	 * @param frameBuffer frame buffer
	 */
	virtual void init(uint32_t x, uint32_t y,
					  uint32_t width, uint32_t height, uint32_t bpp,
					  std::shared_ptr<FrameBufferItf> frameBuffer) = 0;

	/**
	 * Releases initialized connector
	 */
	virtual void release() = 0;

	/**
	 * Performs page flip
	 * @param frameBuffer frame buffer to flip
	 * @param cbk         callback
	 */
	virtual void pageFlip(std::shared_ptr<FrameBufferItf> frameBuffer,
						  FlipCallback cbk) = 0;

private:

	uint32_t mConId;
};

/***************************************************************************//**
 * Display interface class.
 * @ingroup display_itf
 ******************************************************************************/
class DisplayItf
{
public:

	virtual ~DisplayItf() {}

	/**
	 * Starts events handling
	 */
	virtual void start() = 0;

	/**
	 * Stops events handling
	 */
	virtual void stop() = 0;

	/**
	 * Returns connector by id
	 * @param id connector id
	 */
	virtual std::shared_ptr<ConnectorItf> getConnectorById(uint32_t id) = 0;

	/**
	 * Creates display buffer
	 * @param width  width
	 * @param height height
	 * @param bpp    bits per pixel
	 * @return shared pointer to the display buffer
	 */
	virtual std::shared_ptr<DisplayBufferItf> createDisplayBuffer(
			uint32_t width, uint32_t height, uint32_t bpp) = 0;

	/**
	 * Creates frame buffer
	 * @param displayBuffer pointer to the display buffer
	 * @param width         width
	 * @param height        height
	 * @param pixelFormat   pixel format
	 * @return shared pointer to the frame buffer
	 */
	virtual std::shared_ptr<FrameBufferItf> createFrameBuffer(
			std::shared_ptr<DisplayBufferItf> displayBuffer,
			uint32_t width,uint32_t height, uint32_t pixelFormat) = 0;
};

#endif /* SRC_DISPLAYITF_HPP_ */
