#include "UserId.h"

NetWork::userId::userId(): _user_ip("null"), _user_port("null")
{
}

NetWork::userId::userId(std::string ip, std::string port): _user_ip(ip), _user_port(port)
{
}

NetWork::userId::~userId()
{
}
