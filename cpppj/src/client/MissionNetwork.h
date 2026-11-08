// 메타에서 일반 전투로 같은 실제 OS 노드/등록표를 인계하는 소유권 묶음이다.
#pragma once
#include <memory>

namespace netstorm::client { //재접속/주소 재작성/성공ACK 없이 같은 장치를 유지한다.
class EditorLocalTransport; //기존unbuffered9도실제현재연결이면동일device를인계한다.
class MetaConnectionRuntime; //실제 Source 식별은 별도 NativeBattleSource가 선택적으로 보관한다.
class LocalTcpTransport;class LocalUdpTransport; //실제 안정된 socket/PacketConnection 저장소의 소유자다.
class CompositeMissionTransport; //재진입은 기존 내부 끝도 보존하는 같은 전체 coordinator를 인계한다.
struct MissionNetwork { //빈 묶음은 기존 새 장치 생성이며 값이 있으면 같은 두 owner를 대여한다.
    std::shared_ptr<LocalTcpTransport> tcp; //현재9/10/11과 pending/listener의 실제 TCP owner다.
    std::shared_ptr<LocalUdpTransport> udp; //열려 있다면 현재12의 같은 데이터그램 owner다.
    std::shared_ptr<void> lifetime; //등록된 기존 메타 callback의 실제 전체 owner를 함께 pin한다.
    std::shared_ptr<CompositeMissionTransport> composite; //첫 메타 진입은 비어 있고 재진입은 기존 listener/내부 끝까지 같은 owner다.
    std::shared_ptr<EditorLocalTransport> editor; //있는경우기존9/pending의실제device를일반coordinator에함께인계한다.
};
}
