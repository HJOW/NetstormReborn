// 독립 프레임 검사의 서버 Pop에 실제 writer와 명시적으로 빈 원문 연결 표를 연결한다.
#pragma once
#include "o/RawBattleBroadcast.h"
#include "o/RawSquidTransmission.h"
#include "o/SquidPop.h"
#include <memory>
#include <stdexcept>

namespace netstorm::test { // 픽셀/프레임 검사 입력을 바꾸지 않고 새 실제 종속 경계만 공급한다.
struct NoPeerSquidTransmission { // 네트워크 장치를 열지 않는 명시9개 Player/NULL 연결 입력이다.
    o::RawSquidTransmissionState state; // 실제35BYTE/압축writer가사용하는공통버퍼/기본전역이다.
    std::array<std::uint8_t,9*180> players{}; // PATCH Player9의현재접속플래그는모두0이다.
    o::BattlePacketPeers peers{}; // 모든원문연결포인터가NULL인명시입력이다.
    std::uint32_t localOwner{1}; // 기존독립프레임검사의정상로컬번호다.
    bool server{true}; // 같은검사풀의명시서버권한이며임시bool참조를보관하지않는다.
    o::RawBattleBroadcast broadcast{o::OriginalEdition::Patch1078,players,peers,localOwner,server}; // 원문현재조건으로NULL연결을자연생략한다.
};
inline void BindNoPeerSquidTransmission(o::SquidPop& pop,const o::SidPool& pool,std::span<const o::RiftTypeRecord> types) { // 일반프레임검사전용이며패킷바이트/MP검사를추가하지않는다.
    if (pool.Edition()!=o::OriginalEdition::Patch1078) return;
    auto context=std::make_shared<NoPeerSquidTransmission>();o::RawSquidTransmissionHooks hooks;
    hooks.type=[types](std::uint8_t number)->const o::RiftTypeRecord& { if (number>=types.size()) throw std::out_of_range("독립프레임검사의현재TYPE범위");return types[number]; };
    hooks.defaultType=[] { return 82U; }; // 명시Native기본bridge타입입력이다.
    hooks.maxHp=[types](std::uint8_t number) { if (number>=types.size()) throw std::out_of_range("독립프레임검사의HP타입범위");return types[number].maxHitPoints; }; // weakened-priest는0인명시기본입력이다.
    hooks.broadcast=[context](auto packet) { context->broadcast.Send(packet); };
    hooks.assertion=[](auto,auto,auto) { throw std::logic_error("독립프레임검사의실제writer원문진단"); };
    auto writer=std::make_shared<o::RawSquidTransmission>(pool,context->state,std::move(hooks));
    pop.BindTransmission([writer](o::Sid sid,std::uint32_t flags) { writer->Pop(sid,flags); }); // writer의방송장치가context를보유하므로참조와버퍼수명을함께보존한다.
}
}
