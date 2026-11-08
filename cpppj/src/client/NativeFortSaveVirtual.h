//10.78 Fort저장의현재Form가상조회만같은장기Pool/TYPE/Hash/frames로제공한다.
#pragma once
#include "o/SidPool.h"
#include <cstdint>
#include <unordered_set>

namespace netstorm::o { class RawGameAuxState; } //현재DaisTYPE와같은Client장기데이터를빌린다.
namespace netstorm::client { //새World/Meta/Host객체나가상효과를생성하지않는조회adapter다.
class RawSurfaceStorage; //같은현재Pool/TYPE/Hash/진단의실제장기소유자다.
//저장·보상이 요청한 현재 Robust/HasLevel/Count를 같은 장기 물리에서 조회한다.
class NativeFortSaveVirtual {
public:
    NativeFortSaveVirtual(RawSurfaceStorage& storage,const o::RawGameAuxState& globals); //두소유자는이동기조회adapter보다오래살아야하며ctor는전역을쓰지않는다.
    std::uint32_t ObjectRobust(o::Sid sid) const; //현재가상60의9거부표/벌룬2표/그외현재TYPEflags를같은원문몸체로선택한다.
    std::uint32_t ContentRobust(o::Sid sid) const; //ContentForm의현재listflags하위BYTE진단정상복귀뒤현재같은BYTE를재조회한다.
    std::uint32_t HasLevel(o::Sid sid) const; //기존RawSquidLevel::Has의OR조건을바꾸지않는다.
    std::uint32_t Level(o::Sid sid); //제단/다이스의실제Count외에는같은base가상80을선택한다.
private:
    RawSurfaceStorage& storage_; //현재장기물리를빌리며대여/Reset/반납을수행하지않는다.
    const o::RawGameAuxState& globals_; //현재DaisType은실제선택위치에서읽는다.
    std::unordered_set<std::uint16_t> activeCounts_; //한조회사슬의손상순환만호스트스택소진과구별하며게임전역은아니다.
};
}
