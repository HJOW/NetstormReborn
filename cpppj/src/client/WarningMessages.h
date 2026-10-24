// 실제 StyleText/TextGump/Gump 수명으로 표시하는 원본 Warning 안내다.
#pragma once
#include "client/MessageGump.h"
#include "client/MessageBoard.h"
#include <functional>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace netstorm::client {
struct RawMenuTree;
class RawMenuControls;
struct PaletteColorTable;
// 게임 정지/입력 모달을 만들지 않고 루트 직계 자식의 글자 안내를 관리한다.
class WarningMessages {
public:
    // Client의 단일 Gump/StyleText를 빌린다. 부모가 노드를 먼저 정리하며 now는 실제 벽 시각이다.
    WarningMessages(RawMenuTree tree,RawMenuControls& menus,std::function<double()> now,std::function<void()> sound,MessageGumpHooks pauseContext={});
    // 공통 Draw의 참조를 끊는다. 노드 소멸은 같은 Client의 가상 소멸 분배가 먼저 수행한다.
    ~WarningMessages();
    // 현재 화면/파일 팔레트를 갱신한다. 기존 메시지의 저장 문맥은 원본처럼 유지한다.
    void SetView(int width,int height,const PaletteColorTable& palette);
    // 원본 Warning 명령의 종류17/플레이어0/표시1/매개변수0으로 생성한다.
    void Add(std::string_view styledText);
    // 같은20칸 표에서 원본 Pause(kind11)의 영구 안내를 생성하거나 지연 삭제한다.
    void PauseStatus(bool enabled);
    // 원본 Show의 enabled=false로 같은 종류/플레이어/매개변수 메시지를 지연 삭제한다.
    void Remove(std::int32_t kind,std::int32_t player,std::int32_t parameter);
    MessageGumpNode* Node(std::uint32_t token) const; // 공통 기본/텍스트 분배에 같은 안정된 파생 주소를 빌린다.
    void Idle(std::uint32_t token); // 공통 idle 표의 가상 호출에서만 벽시계 만료를 확인하고 Kill한다.
    void DeleteScalar(std::uint32_t token,std::uint32_t flags); // 공통 TextGump가 같은 소유 조각과 기본 노드를 반납한다.
    void Release(std::uint32_t token); // 실제 공통 scalar 소멸자의 free 위치에서 파생 저장소만 반납한다.
    std::optional<MessageBoardRecord> Lookup(std::uint32_t serial) const; //00464060: 현재 공통 루트의 비dead 직계 자식만 검색한다.
    bool HasMessages() const; // 별도 표시 사본 대신 현재20칸/공통 트리에서 실제 생존 안내를 읽는다.
    // 현재 serial·위치·크기·기한을 실제 창 검사에서 읽는다.
    std::string Report() const;
private:
    struct Impl; // 같은 Gump/StyleText 저장소와 처리기의 고정 수명 소유자다.
    std::unique_ptr<Impl> impl_; // 내부 callback은 이 구현의 안정된 주소를 사용한다.
};
}
