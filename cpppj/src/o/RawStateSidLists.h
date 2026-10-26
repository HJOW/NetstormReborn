// PATCH의 정적 SID 목록과 20포인터 등록 표를 Session보다 오래 유지한다.
#pragma once
#include "o/SquidPostPop.h"
#include <array>
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o { // Native 헤더 주소는 식별 토큰이며 호스트 포인터로 역참조하지 않는다.
inline constexpr std::size_t kStateSidRegistryCapacity=20; //00592938의 실제 포인터20칸이다.
//00472880은 첫 빈 칸에만 기록하고 가득 찬 표에는 쓰지 않는다. header0도 원문대로 허용한다.
void RegisterStateSidList(std::span<std::uint32_t> headers,std::uint32_t header);
//004728a0은 중복을 포함한 모든 일치 슬롯만0으로 만든다. 목록 개수/데이터는 건드리지 않는다.
void UnregisterStateSidList(std::span<std::uint32_t> headers,std::uint32_t header);
//00472830은 매 슬롯의 현재 포인터를 읽고 비영 헤더의+0c 개수만0으로 쓴다.
void ResetRegisteredStateSidLists(std::span<const std::uint32_t> headers,const std::function<void(std::uint32_t)>& resetCount);
// 원본 CRT의 정적 목록8개와 다른 Client 소유의 다중선택 목록 등록을 보관한다.
struct RawStateSidLists {
    SquidPostPopList factories; //0055a4d0 헤더의500DWORD 목록이다.
    SquidPostPopList dudes; //0055a4ec 헤더의30DWORD population이다.
    SquidPostPopList outposts; //0055a708 헤더의100DWORD이며 신전 목록과 다르다.
    SquidPostPopList providers; //00592f80 헤더의256DWORD 공급원이다.
    SquidPostPopList geysers; //005950d8 헤더의100DWORD 간헐천이다.
    SquidPostPopList priests; //005954c0 헤더의16DWORD 사제다.
    SquidPostPopList residences; //0059a9d4 헤더의50DWORD 거주 구역이다.
    SquidPostPopList temples; //005e1898 헤더의20DWORD 신전이다.
    std::array<std::uint32_t,kStateSidRegistryCapacity> registeredHeaders{}; //실제 정적 생성 순서9개와 빈11칸이며 개수 정리에서 보존한다.
    RawStateSidLists(); // 최초 한 번만 원본 용량을 확보하고9개 헤더를 순서대로 등록한다.
    void ResetCount(std::uint32_t header,std::int32_t& multipleCount); // 그때 선택된 실제 count만0으로 쓰며 다중선택도 같은 Client 필드를 빌린다.
    void PrepareGeneralWorld(); // 기존 일반 설치의 fresh 의미만 호환한다. Native 개수 정리·prepared 대여에는 호출하지 않는다.
};
}
