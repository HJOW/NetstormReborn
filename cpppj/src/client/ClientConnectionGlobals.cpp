// 장기 연결표가 기존 동기화 DWORD의 실제 owner를 함께 유지한다.
#include "client/ClientConnectionGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { //새 동기화 사본이나 Session별0표를 만들지 않는다.
ClientConnectionGlobals::ClientConnectionGlobals(std::shared_ptr<ClientLocalStateGlobals> local) {
    if (!local) throw std::invalid_argument("연결표의 실제 Client 동기화 owner 누락");auto& synchronization=local->ClockSynchronization().connection;BindSynchronization(synchronization,std::move(local));
}
}
