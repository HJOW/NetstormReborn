// 단일 객체와 청크 저장을 원본 현재 조회/고정 count 위치에 연결한다.
#pragma once
#include "o/RawMemStream.h"
#include <array>

namespace netstorm::o {
// 객체 writer가 각 분기에서 다시 읽는 실제 전역 DWORD다.
enum class FortObjectSaveGlobal {
    Battle,Mission,Editor, //00594fbc/00594fa0/005c85a4: 전투·미션 및 편집 전역이다.
    Scope,Version,LocalOwner, //005c89b0/005c89c0/00540c70: Terr1·Chaff2/서식 버전/현재 플레이어다.
    BridgeType,LocalOnlyType, //005411a0/005412ec: 다리 약식 및 로컬 다이스 저장 판정 타입이다.
    BattleMask,OwnerMask //0054263c/00542644: 현재 flags2와 비교하는 두 마스크다.
};
// 원본은 보고 뒤 위치 BYTE 또는 owner 쓰기를 계속할 수 있다.
enum class FortObjectSaveAssertion { XOffset,YOffset,Owner }; // Template.cpp667/668/707행 진단이다.
// 실제 TYPE/가상/stream 경계를 공급하며 저장 순서와 필터는 본체가 수행한다.
struct FortObjectSaveHooks {
    std::function<std::uint32_t(FortObjectSaveGlobal)> global; // 매 호출 현재 DWORD를 읽으며 최초 값으로 캐시하지 않는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> form; // 고정 SID의 raw50BYTE를 빌린다. 빌림 자체는 효과가 없다.
    std::function<std::uint32_t(std::uint32_t)> flags1,flags2; //004ac1e0/004ac200: 현재 raw TYPE를 매번 다시 조회한다.
    std::function<std::uint32_t(std::uint32_t)> robust,hasLevel,level; // 현재 가상60/004ad0a0/가상80이다. ContentForm 판정으로 대체하지 않는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> island; //0046f6e0: 현재 x/y float 비트를 인자로 캡처한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> write; //0044cce0: 캡처한 BYTE/WORD만 같은 stream에 쓴다.
    std::function<void(std::uint32_t,std::uint32_t)> contents; //004bcea0: 고정 SID와 동일 stream의 내용물을 저장한다.
    std::function<void(FortObjectSaveAssertion)> assertion; // 실제 선택한 진단만 요구한다.
};
//004bd940: 초기 좌표 절삭/필터→위치·타입→현재 폭별 필드→레벨/owner→내용물 순서다.
std::uint32_t SaveFortObject(std::uint32_t stream,std::uint32_t sid,std::int32_t originX,std::int32_t originY,const FortObjectSaveHooks& hooks);
// 실제 __ftol의 signed64 불확정값과 하위 DWORD 감김을 float32 좌표에 적용한다.
std::int32_t FortSaveCoordinate(float value);
// 청크 Begin이 받는 네 단정도 좌표와 고정 flags0이다.
struct FortChunkSaveArea { float left{},top{},right{},bottom{}; }; // 원본 int<<4→float 저장 후 양끝 값이다.
// 원본 Finder의 각 청크별 독립 현재/다음 수명이다.
struct FortChunkSaveCursor {
    std::function<std::uint32_t()> current; // 현재 DWORD 후보이며 다른 효과 뒤 다시 조회한다.
    std::function<void()> next; // 저장 후 원본 Next를 실행한다.
};
// 청크 검색/단일 객체와 실제 stream만 연결하며 표지/count는 본체가 쓴다.
struct FortChunkSaveHooks {
    std::function<FortChunkSaveCursor(const FortChunkSaveArea&)> begin; //0041def0: 사각형/flags0으로 첫 후보까지 진행한다.
    std::function<std::uint32_t()> poolLimit; // 각 후보 경계에서 현재 물리 SID 수를 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)> object; //004bd940의 고정 stream/후보/최초 origin이다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; // 실제 stream 필드와 같은 물리 버퍼의 동기적 빌림이다.
};
//004be020: Begin→marker99→countWORD 위치 예약→후보 저장→탈출 시 고정 위치에 count 쓰기다.
void SaveFortChunk(std::uint32_t stream,std::int32_t chunkX,std::int32_t chunkY,const FortChunkSaveHooks& hooks);
}
