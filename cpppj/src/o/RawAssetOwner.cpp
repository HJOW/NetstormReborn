// PATCH00454cb0/CD0047b580과 PATCH00462580/CD00416f20의 쓰기 순서와 폭을 보존한다.
#include "o/RawAssetOwner.h"
#include <utility>

namespace netstorm::o { // 호스트 주소 대신 raw 슬롯의 원본 VT 기록값만 비교한다.
namespace { // 현재 가상 표를 판독하는 작은 보조 함수다.
// 정렬되지 않은 슬롯의 첫 DWORD를 little endian으로 읽는다.
std::uint32_t OwnerTable(std::span<const std::uint8_t> raw) {
    return std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
}
}
// 생산 카운터를 먼저 올리므로 공통 소유자가 진단/목록 처리에서 중단돼도 그 변경은 남는다.
std::function<void(Sid,std::uint32_t)> MakeAssetOwnerDispatch(SidPool& pool,const BridgeConnectState& state,
    std::uint32_t& productionDirty,std::function<void(Sid,std::uint32_t)> base) {
    auto surfaces=MakeOwnerDispatch(pool,state,std::move(base)); // 기존 섬 받침/종유석과 공통 부모의 동일 순서를 재사용한다.
    return [&pool,&productionDirty,surfaces=std::move(surfaces)](Sid sid,std::uint32_t owner) {
        const auto raw=pool.Slot(sid); // 호출할 때의 VT와 extra를 읽고 생성 당시 타입으로 추정하지 않는다.
        const bool patch=pool.Edition()==OriginalEdition::Patch1078; // 실제 슬롯 필드와 VT는 판본별이다.
        const auto table=OwnerTable(raw); // 파생 몸체는 이 가상 호출의 진입 표로 고정된다.
        if (table==(patch ? 0x50a8e8U : 0x500b10U)) {
            const_cast<std::uint8_t*>(raw.data())[patch ? 34U : 32U]=0;
            return;
        }
        if (table==(patch ? 0x509148U : 0x503298U) || table==(patch ? 0x509210U : 0x503358U)) {
            if (!(raw[patch ? 40U : 35U]&9U)) ++productionDirty;
        }
        surfaces(sid,owner);
    };
}
}
