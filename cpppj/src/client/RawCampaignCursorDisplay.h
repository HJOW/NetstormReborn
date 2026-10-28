// 실제 커서의 현재 목록/판정/격자를 원본 수집/Draw와 현재 Renderer 표면에 연결한다.
#pragma once
#include "client/PreviewDraw.h"
#include "client/Sound.h"
#include "client/RawSurfaceWorld.h"
#include "o/SpStore.h"
#include <memory>
namespace netstorm::client {
class OwnerColorTableState; // 현재557940은 커서 표시기보다 오래 살아야 한다.
// 장기 게임 저장소는 참조만 빌리고 표시 옵션/피드백만 내부에서 소유한다.
class RawCampaignCursorDisplay {
public:
    // source/자산/SP/현재 시각·PlayerKnown은 표시기보다 오래 살아야 한다.
    RawCampaignCursorDisplay(RawWorldCore core,CursorPreviewSource source,const GameAssets& assets,o::ScrambledSpStore& sp,const std::uint32_t& playerKnown);
    ~RawCampaignCursorDisplay(); // 현재 callback을 보관하는 고정 주소를 해제한다.
    RawCampaignCursorDisplay(const RawCampaignCursorDisplay&)=delete; // 참조 주소를 보존한다.
    RawCampaignCursorDisplay& operator=(const RawCampaignCursorDisplay&)=delete; // 다른 세션과 복사하지 않는다.
    // 파일 팔레트를 교체할 때만 세 색표/기본 색을 다시 만든다. 일시 번개 팔레트와 구별한다.
    void SetPalette(std::span<const ScreenColor> palette,std::span<const std::uint32_t> colorSortLogical={}); //실제Client는같은고정초기RGB를별도전달하며기존독립검사는명시fallback이다.
    void SetView(SoundView view); // 현재 월드 viewport를 마지막 dirty와 별도로 보존한다.
    void BindOwnerColors(std::shared_ptr<OwnerColorTableState> owner); // 배너 옵션의 현재 표를 빌리며 테마 설정과 구별한다.
    // 원본 문자열 번역/실제 FontStore를 장기 출력기에 연결한다. 문자열은 원본 좁은 바이트다.
    void SetText(FontStore& fonts,std::function<std::string(std::string_view)> translate);
    // 부모 Gump 깊이와 기본17종 외 확장 소유자 색표의 실제 공급자를 연결한다.
    void SetExternal(std::function<std::uint32_t(std::uint32_t)> depth,
        std::function<ColorMap(std::uint32_t,std::uint32_t)> specializedOwner);
    // Native 수집은 dirty마다 한 번 호출하며 반환 항목을 다른 월드 그림과 함께 정렬한다.
    std::vector<RenderSprite> Sprites(ScreenRect clip,std::uint32_t dirtyFlags);
    std::array<std::uint32_t,2> TextColors() const; // 현재 파일 팔레트의 경고 흰색/비용 노랑이다.
    PreviewDrawState& Settings(); // 실제 격자/글자 표시 옵션과 타입별 색표 존재 상태다.
    PreviewCollectionState& Feedback(); // 현재 강조 시각/남은 횟수의 원본 저장소다.
private:
    struct Impl; // 현재 원본 상태/수집기/픽셀 장치의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // 커서/게임의 장기 저장소보다 먼저 해제한다.
};
}
