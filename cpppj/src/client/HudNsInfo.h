// 원본 NSInfo의 일반 미션 부착을 실제 배경 자식과 같은 Gump 트리에 연결한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// 원본0xd8BYTE NSInfo의 전체 자식 포인터다. 생성/메타맵 본문의 완료를 뜻하지 않는다.
struct HudNsInfoNode : GumpCreateNode {
    std::uint32_t background{}; // +a0: 모든 하위 패널의 실제 배경 부모다.
    std::array<std::uint32_t,8> players{}; // +a4~c0의 현재 PlayerInfo8개다.
    std::uint32_t roster{},title{},upperDivider{},lowerDivider{},battle{}; // +c4/+c8/+cc/+d0/+d4의 현재 실제 자식이다.
};
// 이름이 다른 호스트 모드에 복사하지 않고 실제 두 원본 전역을 빌린다.
struct HudNsInfoGlobals {
    const std::uint32_t& metamap; //00594fc8의 현재 모드이며 Session의challenge에 연결된다.
    const std::uint32_t& playerKnown; //00595344의 현재 로그인/플레이어 인식 상태다.
};
// 실제 공통 트리와 현재 파생 노드의 수명을 공급한다.
struct HudNsInfoHooks {
    std::function<HudNsInfoNode*(std::uint32_t)> node; // 고정this의 실제 파생 저장소다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 현재 배경의 같은 기본 노드다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach; //00466750의 실제 공통부착이다.
    std::function<void(std::uint32_t)> push; // 현재 배경의 가상+18이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 현재 배경의 가상+20이다.
    std::function<void(std::uint32_t)> metamapLayout; //0047ea71 이후의 별도 전체 메타맵 배치다. 미연결 진입은 거부한다.
};
// 전체 자식 생성은 별도 부모가 맡으며 여기서는0047e9d0의 실제 일반부착 경계를 복원한다.
class HudNsInfo {
public:
    // 참조 전역/노드/훅의 수명은 이 처리기 및 재진입보다 길어야 한다.
    HudNsInfo(HudNsInfoGlobals globals,HudNsInfoHooks hooks);
    // 기본Attach→현재배경Push→너비/높이·배율쓰기→배경Attach→현재mode관문 순서다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
private:
    // 원본의 고정this는 호출 중 같은 주소를 유지해야 한다.
    HudNsInfoNode& Node(std::uint32_t token) const;
    // 각 현재배경 조회에서 유효한 같은 기본 노드를 요구한다.
    GumpCreateNode& Base(std::uint32_t token) const;
    HudNsInfoGlobals globals_; // 복제하지 않은 두 현재전역 참조다.
    HudNsInfoHooks hooks_; // 같은 실제 트리와 미복원메타맵을 구별하는 경계다.
};
}
