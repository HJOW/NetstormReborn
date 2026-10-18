// 실제 ShapeToBuffer의 메모리 캡처를 기존 VFX RLE 몸체와 보호 자산에 연결한다.
#pragma once
#include "client/VFXDraw.h"
#include "o/RawConstructionVisual.h"
#include "o/SquidDisplay.h"
#include <functional>

namespace netstorm::client {
// 실제 캡처가 읽는 현재 fixed16 배율과 TYPE 추가 기준점 옵션이다.
struct ShapeToBufferState {
    std::int32_t zoom{65536}; // 패치0059a918/CD00565c30이며 캡처 메타데이터에만 곱한다.
    bool addTypeHotspot{}; // 패치00542414/CD005207b0이 마지막 기준점 보정을 켠다.
    std::span<const std::uint32_t> ownerTypeMaps; // 현재 타입별 색표 토큰이다. 색표가 있으면 실제 mapped 경로가 필요하다.
    std::function<std::span<const std::uint8_t>(std::uint32_t,std::uint32_t)> ownerColors; // 현재 타입/owner로 실제 색 번호를 선택한256바이트 표다.
    std::function<std::int32_t(o::Sid)> mainFrame; // extra&2일 때 현재 파생의 실제 가상+78이다. -1이면 기본 그림을 생략한다.
    std::function<void(o::Sid,o::ConstructionVisualImage&,std::int32_t,std::int32_t)> additional; // 같은 캡처 버퍼에 그리는 가상+a4다. TYPE 기준점 보정 전에 호출한다.
};
// 004a7db0/CD0045b100의 현재 plain/mapped 및 파생 가상 그림 캡처다. 파생 분배는 현재 소비자가 공급한다.
class RawShapeToBuffer {
public:
    // 같은 판본/로딩 순서의 TYPE와 SHP를 읽기 전용으로 참조하며 창이나 장치를 만들지 않는다.
    RawShapeToBuffer(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
        std::span<const o::SquidDisplayShape> shapes,const ShapeDatabase& database,const ShapeToBufferState& state);
    // 현재 frame 헤더→투명 버퍼 확보→실제 plain RLE→선택 TYPE 기준점의 원본 순서다.
    // CD의 invalid 현재 객체/frame은8×8/4×4 투명 그림으로 반환한다.
    o::ConstructionVisualImage Capture(o::Sid sid) const;
    // 같은 피해/건설 캡처 어댑터가 다른 월드의 풀을 혼용하지 않도록 확인한다.
    const o::SidPool& Pool() const;
private:
    // 원본 imul lowDWORD 뒤 sar16의 signed 감김을 명시적으로 보존한다.
    std::int32_t Scale(std::int32_t value) const;
    const o::SidPool& pool_; // 현재 raw 타입/frame/owner와 free/extra를 읽는 실제 SID 저장소다.
    std::span<const o::RiftTypeRecord> types_; // 현재 maxHP 표식/추가 기준점과 타입 번호 계약이다.
    std::span<const o::SquidDisplayShape> shapes_; // 원본 loaded/frameCount 및 Squid 추가 헤더의 현재 자료다.
    const ShapeDatabase& database_; // 보호 SHP의 실제 RLE 스트림이며 기존 DrawShape를 재사용한다.
    const ShapeToBufferState& state_; // 매 캡처의 현재 배율/기준점 옵션/색표 토큰이다.
};
}
