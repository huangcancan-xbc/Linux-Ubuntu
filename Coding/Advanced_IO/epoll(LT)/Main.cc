#include "EpollServer.hpp"

#include <memory>

int main()
{
    std::cout <<"fd_set bits num : " << sizeof(fd_set) * 8 << std::endl;

    std::unique_ptr<EpollServer> svr(new EpollServer(8080));
    svr->Init();
    svr->Start();

    return 0;
}