#ifndef _CONTROLLER_IP_H
#define _CONTROLLER_IP_H
#include <cstdlib>
#include <string>

/*控制器默认IP地址,唯一定义处*/
#define CONTROLLER_IP "192.168.57.2"
/******************************/

//控制器ip地址取自环境变量ROBOT_IP,未设置或为空时使用默认CONTROLLER_IP
inline std::string robot_ip_from_env(){
    const char* ip = std::getenv("ROBOT_IP");
    return (ip != nullptr && ip[0] != '\0') ? std::string(ip) : std::string(CONTROLLER_IP);
}

#endif
