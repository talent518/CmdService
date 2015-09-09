#ifndef SERVICE_EXCEPTION_H
#define SERVICE_EXCEPTION_H

#include <exception>
#include <string>

using std::exception;
using std::string;

class ServiceException : public exception {
public:
	ServiceException(const string &what = "")
		:_what("ServiceException:" + what) {}

	virtual const char* what() const throw() {
		return _what.c_str();
	}
private:
	string _what;
};

#endif/*SERVICE_EXCEPTION_H*/