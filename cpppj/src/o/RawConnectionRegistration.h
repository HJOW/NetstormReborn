// 실제 연결 등록/재배치와 pending 제거의 원문 store 순서를 보존한다.
#pragma once
#include "o/RawConnectionCleanup.h"

namespace netstorm::o { //포인터 숫자는 같은 실제 PacketConnection 조회 장치에만 전달한다.
// 원문이 선택한 진단과 buffered 설정만 외부 효과로 받는다.
struct ConnectionRegistrationHooks {
    std::function<PacketConnection&(std::uint32_t)> node; //현재 연결의 실제 sender 필드를 반환한다.
    std::function<void(std::uint32_t)> buffered; //004df470(node,1)의 현재 연결 설정이다.
    std::function<void(std::uint32_t)> assertion; //원문 줄번호b0/b3/b6/bb/c2 진단이다.
};
//004e2870: 현재 대상칸 진단→새 연결 store→동일 노드 sender store다.
void RegisterTransceiver(std::int32_t id,std::uint32_t token,ConnectionCleanupGlobals globals,const ConnectionRegistrationHooks& hooks);
//004df4f0: 현재 pending[-id]0→현재 개수 감소이며 입력 검증을 앞당기지 않는다.
void RemovePendingConnection(std::int32_t id,ConnectionCleanupGlobals globals);
//004e27b0: 서버/대상 진단→현재 pending을 양수 표에 store→sender→buffered→pending 제거다.
std::int32_t AssignPendingConnection(std::int32_t pending,std::int32_t target,std::uint32_t server,ConnectionCleanupGlobals globals,const ConnectionRegistrationHooks& hooks);
}
