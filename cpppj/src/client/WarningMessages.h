// 실제 StyleText/TextGump/Gump 수명으로 표시하는 원본 Warning 안내다.
#pragma once
#include "client/MessageGump.h"
#include <functional>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace netstorm::client {
class FontStore;
struct PaletteColorTable;
struct RenderText;
// 게임 정지/입력 모달을 만들지 않고 루트 직계 자식의 글자 안내를 관리한다.
class WarningMessages {
public:
    // 글꼴은 이 관리자보다 오래 살아야 한다. now는 원본처럼 호출할 때마다 벽 시각을 조회한다.
    WarningMessages(FontStore& fonts,const PaletteColorTable& palette,std::function<double()> now,std::function<void()> sound,MessageGumpHooks pauseContext={});
    // 파생 텍스트를 실제 Kill/Cleanup한 뒤 처리기와 저장소를 해제한다.
    ~WarningMessages();
    // 현재 화면/파일 팔레트를 갱신한다. 기존 메시지의 저장 문맥은 원본처럼 유지한다.
    void SetView(int width,int height,const PaletteColorTable& palette);
    // 원본 Warning 명령의 종류17/플레이어0/표시1/매개변수0으로 생성한다.
    void Add(std::string_view styledText);
    // 같은20칸 표에서 원본 Pause(kind11)의 영구 안내를 생성하거나 지연 삭제한다.
    void PauseStatus(bool enabled);
    // 원본 Show의 enabled=false로 같은 종류/플레이어/매개변수 메시지를 지연 삭제한다.
    void Remove(std::int32_t kind,std::int32_t player,std::int32_t parameter);
    // 원본 idle 시간 판정과 실제 지연 삭제를 처리하고 표시 변화 여부를 반환한다.
    bool Idle();
    // TextGump의 실제 기준선/클립/StyleText 출력 순서를 Renderer에 제출한다.
    std::vector<RenderText> Text() const;
    // 현재 serial·위치·크기·기한을 실제 창 검사에서 읽는다.
    std::string Report() const;
private:
    struct Impl; // 같은 Gump/StyleText 저장소와 처리기의 고정 수명 소유자다.
    std::unique_ptr<Impl> impl_; // 내부 callback은 이 구현의 안정된 주소를 사용한다.
};
}
