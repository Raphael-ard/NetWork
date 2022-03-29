#pragma once
#ifndef _USER_ID_H_
#define _USER_ID_H_

#include <string>

namespace NetWork
{
	class userId
	{
	public:
		userId(void);
		userId(std::string ip, std::string port);
		~userId(void);

	private:
		std::string			_user_ip;
		std::string			_user_port;
	};
}

#endif // !_USER_ID_H_
