// 원본 004d3090의20칸 serial 표와 경고 시간 판정을 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace netstorm::client {
// 현재 직계 자식 Gump에서 읽은 메시지의 식별 필드다.
struct MessageBoardRecord {
    std::uint32_t token{},serial{}; // 같은 실제 노드의 호스트 토큰과 원본 serial이다.
    std::int32_t kind{},player{},parameter{}; // 원본 +a4/+a8/+b8의 중복/삭제 검색 키다.
};
// 실제 전역20 DWORD와 출력 억제/현재 화면 폭이다.
struct MessageBoardState {
    std::array<std::uint32_t,20> serials{0xffffffffU}; // 첫 -1은 원본의 첫 호출 초기화 표식이다.
    bool suppressed{}; // 원본00594fa4의 현재 비영 상태다.
    std::int32_t screenWidth{}; // 원본005c78f0의 현재 화면 폭이다.
};
// 실제 Gump 조회/생성/삭제/부착을 공급한다. 생성된 노드의 주소는 호출 동안 안정되어야 한다.
struct MessageBoardHooks {
    std::function<std::optional<MessageBoardRecord>(std::uint32_t)> lookup; // 직계 자식 serial 조회다.
    std::function<void(std::uint32_t)> kill; // 실제 지연 삭제 요청이다.
    std::function<MessageBoardRecord(std::int32_t,std::int32_t,std::int32_t,std::string_view)> create; // 실제 파생 생성이다.
    std::function<std::int32_t(std::uint32_t)> width; // 생성/측정 후 원본 ftol의 현재 폭이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> attach; // 실제 루트에 현재 좌표로 부착한다.
};
// Warning은 종류17이며 중복 일치로 검색을 멈추지 않는다. 빈 칸이 없으면 생성하지 않는다.
class MessageBoard {
public:
    // 전역과 실제 Gump 처리기의 수명은 이 분배기보다 길어야 한다.
    MessageBoard(MessageBoardState& state,MessageBoardHooks hooks);
    // enabled=false이면 일치 메시지를 삭제한다. true이면 최초 빈 칸에 원본 순서대로 생성한다.
    void Show(std::int32_t kind,std::int32_t player,bool enabled,std::int32_t parameter,std::string_view text) const;
private:
    MessageBoardState& state_; // 현재20칸 표와 억제/해상도를 공유한다.
    MessageBoardHooks hooks_; // 실제 조회/생성/수명/부착 소비자다.
};
// 00418070은 float 지연10초를 double 벽 시각에 더해 double 기한으로 저장한다.
double WarningDeadline(double wallSeconds);
// 004d3030: 기한0은 영구이며 벽 시각이 기한에 도달하면 삭제를 요청한다.
bool WarningExpired(double deadline,double wallSeconds);
}
