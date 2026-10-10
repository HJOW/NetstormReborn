// 통지 단위 검사와 실제 정리/배치 통합 검사가 함께 쓰는 효과 경계 기본값이다.
#pragma once
#include "ConstructionSupport.h"
#include "o/RawConstructionNotice.h"

namespace netstorm::test::construction {
// 관찰하지 않는 경계만 비워 둔다. 사제 이동 분기는 각 검사가 필요한 실제/관찰 효과로 바꾼다.
inline ConstructionNoticeHooks SilentNoticeHooks() {
    ConstructionNoticeHooks hooks;
    // 예측 정리와 조각 배치는 통합 검사에서 실제 모듈로 교체한다.
    hooks.clear=[](const ConstructionClearRequest&) {};hooks.place=[](const ConstructionPlaceRequest&) {};
    // 건설 사제 이동은 생략하고 소유 사제 조회는 예약 슬롯 0을 돌려준다.
    hooks.moveBuilder=[](std::uint32_t,Sid,std::uint32_t,std::uint32_t) {};hooks.findPriest=[](std::uint32_t) { return Sid{}; };
    // contained 타입 조회/삭제는 필요할 때 독립 관찰 경계로 교체한다.
    hooks.containedType=[](std::uint32_t) { return 0U; };hooks.removeContained=[](Sid,std::uint32_t) {};
    // 기본 사제는 이동 불가이며 기본 접근점은 원점이다.
    hooks.immobile=[](Sid) { return true; };hooks.approach=[](Sid,float,float,std::uint32_t) { return ConstructionNoticePoint{}; };
    // 기본 이동은 실패이며 실제 환불은 어댑터가 배치 모듈로 교체한다.
    hooks.tryMove=[](Sid,float,float,Sid) { return false; };hooks.refund=[](std::uint32_t,std::uint32_t) {};
    // 삭제·건설 시작·최종 갱신은 검사별 관찰 효과로 교체한다.
    hooks.destroy=[](Sid,std::uint32_t) {};hooks.start=[](Sid,std::uint32_t) {};hooks.refresh=[] {};
    return hooks;
}
// 이력의 좌표 비트를 little endian 40바이트로 직렬화한다. 기대값은 독립 원본 출력에서만 읽는다.
inline std::string NoticeHistory(const ConstructionNoticeState& state) {
    std::array<std::uint8_t,40> bytes{};
    // 가장 최근부터 다섯 좌표를 순서대로 쓴다.
    for (std::size_t i=0;i<state.history.size();++i) {
        Put(bytes,i*8,Bits(state.history[i].x));Put(bytes,i*8+4,Bits(state.history[i].y));
    }
    return Hex(bytes);
}
}
