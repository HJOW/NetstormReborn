//Client 수명의 weak 실제 장치 경로와 배송 동안의 실제 전송 보유값을 구별한다.
#pragma once
#include "client/MissionNetwork.h"
#include "o/RawPacketDispatch.h"
#include <functional>
#include <optional>
#include <vector>

namespace netstorm::client { //대기 중 World/Session/Source를 강하게 잡지 않는 호스트 연결 장부다.
class MetaConnectionRuntime;class EditorLocalTransport; //실제 Meta와 기존 Editor의 물리 소유자다.
struct NativeBattleSource {
    MissionNetwork network; //같은 TCP/UDP/Composite와 그 실제 network 수명을 배송 동안만 인계한다.
    std::shared_ptr<MetaConnectionRuntime> meta; //있는 경우에만 실제 Meta callback owner를 별도로 식별한다.
    std::shared_ptr<void> device; //배송받은 실제 device이며 Runtime/Session/가짜 token이 아니다.
};
class WeakNativeBattleSource final { //next7 예약은 실제 Source를 대기 기간에 소유하지 않는다.
public:
    explicit WeakNativeBattleSource(const NativeBattleSource& source); //실제 배송값의 동일 control block만 weak로 기억한다.
    NativeBattleSource Lock() const; //살아 있는 실제 장치만 복구하며 없으면 원래 State8의 새 생성을 허용한다.
private:
    std::weak_ptr<LocalTcpTransport> tcp_;std::weak_ptr<LocalUdpTransport> udp_; //원래 실제 두 socket 장치다.
    std::weak_ptr<void> lifetime_,device_; //Source 수명과 배송 device를 World 소유권과 구별한다.
    std::weak_ptr<CompositeMissionTransport> composite_; //있는 경우 같은 coordinator/내부 끝까지 인계한다.
    std::weak_ptr<EditorLocalTransport> editor_; //실제기존Editor9를weak로기억하며새Source로가장하지않는다.
    std::weak_ptr<MetaConnectionRuntime> meta_; //선택적 실제 Meta owner의 정체성이다.
};
class ClientPacketSources final { //ClientConnectionGlobals가 소유하며 실제 Source를 강하게 되참조하지 않는다.
public:
    void Publish(const std::shared_ptr<MetaConnectionRuntime>& source); //make_shared 완료 뒤 실제 Meta와 같은 TCP/UDP의 경로만 공개한다.
    void Publish(const std::shared_ptr<CompositeMissionTransport>& source); //현재 실제 Composite/내부 device 경로를 공개한다.
    void Publish(const std::shared_ptr<EditorLocalTransport>& source); //기존 Editor device는 일반 MissionNetwork를 갖지 않아도 배송 수명은 보유한다.
    std::optional<NativeBattleSource> Resolve(const o::PacketConnection& connection) const; //sender/phase 대신 실제 동일 node 주소가 속한 device만 선택한다.
private:
    struct Route {
        std::weak_ptr<void> device; //오래된 경로를 정리할 수 있는 실제 device의 weak 정체성이다.
        std::function<std::optional<NativeBattleSource>(const o::PacketConnection&)> select; //동일 node가 맞을 때만 실제 장기 전송값을 배송 scope에 반환한다.
    };
    void Add(Route route); //이미 종료된 weak 경로만 버리고 새 실제 경로를 마지막에 공개한다.
    std::vector<Route> routes_; //Composite가 같은 Meta node를 인계한 경우 최신 실제 coordinator 경로가 먼저 선택된다.
};
}
