/*
 * Exception.hpp
 *
 *  Created on: Dec 9, 2016
 *      Author: al1
 */

#ifndef SRC_DRMMAP_EXCEPTION_HPP_
#define SRC_DRMMAP_EXCEPTION_HPP_

#include <exception>

class DrmMapException : public std::exception
{
public:
	/**
	 * @param msg error message
	 */
	explicit DrmMapException(const std::string& msg) : mMsg(msg) {};
	virtual ~DrmMapException() {}

	/**
	 * returns error message
	 */
	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

#endif /* SRC_DRMMAP_EXCEPTION_HPP_ */
