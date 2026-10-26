// 원문0047fb90의 입력 부모를 전용 이미지/목록 수명 장치에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <array>
#include <functional>
#include <span>

namespace netstorm::client { //전용 창의 원문 필드와 입력만 소유한다.
// 원본+d00의21행과 drag6BYTE는 별도 저장소다.
struct EditorIslandDragState {
    std::uint32_t flags{},dirty{1},drag{}; //실제 parent+4/+d80/+d84의 현재값이다.
    std::array<std::uint8_t,126> choices{}; //원본21행이며 Territory120BYTE와 별개다.
    std::array<std::uint8_t,6> record{}; //드래그 시작 때 복사한 원문6BYTE다.
    std::int32_t sourceIndex{}; //목록의 현재 선택을 원문 ID로 바꾼+d90 값이다.
    std::uint32_t* sharedFlags{}; // 실제 창에서는 같은 기본 Gump+4를 직접 사용하고 독립 원문 재생에서는 기존 flags를 사용한다.
};
// 이미지와 목록의 현재 필드를 매 호출 재조회하며 빈 장치는 성공으로 간주하지 않는다.
struct EditorIslandDragHooks {
    std::function<std::uint32_t()> closeSerial; //실제 닫기 버튼의 현재+2c다.
    std::function<void()> killParent,pushParent; // 실제 Kill과00465250의 비어 있는 focus 등록이다. 기존 pushParent 이름은 관측 API 호환을 위해 유지한다.
    std::function<void(const char*)> assertion; //dragNULL/nonNULL 진단 후 원문대로 계속한다.
    std::function<std::uint32_t(std::int32_t)> image; //0047d1c0의 현재 목록 항목 image 포인터다.
    std::function<std::int32_t()> selection; //같은 목록+14c를 매번 재조회한다.
    std::function<std::int32_t(std::int32_t)> recordIndex; //0047d170의 현재 행ID다.
    std::function<std::uint32_t(std::uint32_t)> createDrag; //실제 MetaImageGump 생성자다.
    std::function<void(std::uint32_t)> adjustDrag,killDrag; //0047cbd0 및 공통 Kill이다.
    std::function<std::array<float,2>(std::uint32_t)> size; //실제 drag+3c/+40의 현재 float 크기다.
    std::function<void(std::uint32_t,float,float)> attachDrag; //실제 root/flags0 Attach 뒤 depth−100을 쓴다.
    std::function<std::array<std::int32_t,4>()> parentBounds,listBounds,mapBounds; //같은 실제 Collection의 현재 경계다.
    std::function<std::array<std::int32_t,2>()> mapSize; //preview의 현재 폭/높이다.
    std::function<bool(std::span<std::uint8_t,6>)> drop; //CanPlace→실제 World 배치/Graph/Territory이며 false는 충돌이다.
    std::function<void(float,float)> camera; //00433aa0의 실제 좌표 장치다.
    std::function<void(std::uint32_t)> releaseImage; //0046d8a0→free의 같은 image 반납이다.
    std::function<void(std::int32_t)> remove; //0047d1f0의 현재 행 제거다.
    std::function<void()> clearSelectionAndUpdate,incrementRevision; //selection−1→virtual8(0), 실제 지도DWORD++다.
};
// 주어진 원문 사건 하나만 소비하며 실패한 drop의 record 좌표/drag를 보존한다.
bool HandleEditorIslandDrag(EditorIslandDragState& state,const InputEvent& event,const EditorIslandDragHooks& hooks);
}
