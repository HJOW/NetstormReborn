// 섬 추가 창의 실제 장식 부모·지도·후보와 현재 Session 장치의 수명을 묶는다.
#pragma once
#include "client/GumpDecoration.h"
#include "client/EditorIslandLayout.h"
#include "o/ChunkMap.h"
#include <optional>

namespace netstorm::client { // 단일 Client UI가 같은 실제 Session/난수/지도 변경 전역을 빌린다.
class RawCampaignSession; // 전체 편집 월드는 창이 복제하지 않는다.
// 원문0xd98 창의 의미 있는 필드와 호스트의 생성 완료 추적이다.
struct EditorIslandWindowNode:GumpDecorationNode {
    RawImage currentImage; // 원본+c4이며 이 부모만 소유하는 crop0 지도 Image다.
    EditorIslandLayoutFields layout; // 원본+d8..f0의 실제 일곱 자식/이미지/입력 별칭이다.
    std::optional<o::ChunkMap> preview; // 원본+f4의 독립14×14 지도다. 실제 생성 순서에 emplace한다.
    EditorIslandDragState interaction; // 원본+d00 후보21행/+d80 dirty/+d84 drag/+d88 record/+d90 ID다.
    std::uint32_t ready{}; // 원본+d94의 현재 후보 갱신 완료 값이다.
    bool counted{}; // C++ 부분 생성 반납에서 아직 증가하지 않은 원본 참조 수를 감소시키지 않는 추적이다.
};
// 모든 콜백은 창/루트 드래그가 반납될 때까지 같은 실제 Client/Session을 가리켜야 한다.
struct EditorIslandWindowContext {
    RawCampaignSession* session{}; // 같은 실제 기존/신규 Editor 설치를 완료한 Session이다.
    std::function<std::string(std::string_view)> translate; // 원본 좁은 Done/제목 번역이다.
    std::function<std::uint32_t(std::string_view)> tell; // 실제004c8f60 HtmlBody serial을 반환한다.
    std::function<std::uint32_t(std::uint32_t)> random; // 같은 Client의 GameRandom.Next다.
    std::function<void(float,float)> camera; // 실제00433aa0 중심 이동이다.
    std::function<void()> revision; // 같은005949d4 DWORD를 원문 성공 위치에서 증가시킨다.
    std::function<void(std::uint32_t)> pulseRegistered; //00419d50의 현재 등록 ID/버튼 점멸 관문이다.
};
}
