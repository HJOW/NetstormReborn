// 원본cf570의 LF/Cancel 본문 조립과 현재 transient HTML serial 재사용을 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 실제 현재 두 전역을 소유하며 새 부모 생성 콜백이 createdSerial을 변경할 수 있다.
struct MissionLoadingMessageState {
    std::uint32_t transientSerial{}; //005cab34의 진행 안내 부모 serial이다. 등록 id/객체 토큰이 아니다.
    std::uint32_t createdSerial{}; //005cab38의 가장 최근 cf270 부모 serial이며 create 직후 다시 읽는다.
};
// 외부 HTML 본체는 현재 노드/필드를 제공한다. 포맷된 원문은 이미 Windows-1252 등 실제 바이트여야 한다.
struct MissionLoadingMessageHooks {
    std::function<std::string()> cancelText; // 실제 Cancel 번역은 기본 버튼을 추가하는 분기에서만 조회한다.
    std::function<std::uint32_t(std::uint32_t)> findSerial; //00464020의 현재 serial 검색이며 실패는0이다.
    std::function<std::uint32_t&(std::uint32_t)> updateDepth; // 부모+21bc의 현재 DWORD 참조다. 콜백 동안 같은 노드가 살아 있어야 한다.
    std::function<std::uint32_t(std::uint32_t)> serial; // 기존 부모 갱신이 끝난 뒤 현재+2c를 읽는다.
    std::function<void(std::uint32_t,std::string&)> metadata; //004c83b0은 메타를 현재 버퍼에서 지운다. 다음 body는 같은 변경 결과를 받는다.
    std::function<void(std::uint32_t,std::string_view)> body; //004c7aa0의 현재 부모 본문/재배치 갱신이다.
    std::function<std::uint32_t(std::string_view,std::uint32_t,std::int32_t,std::int32_t)> create; //004cf270(body,0,400,30)이며 반환은 새 serial이다.
};
// 이미 확장printf를 거친 C문자열에 LF와 원본 기본 Cancel 메타만 붙인다. 임의 스타일/따옴표 escape를 만들지 않는다.
// 임시 버퍼/확장printf4de490은 외부 경계다. 1024BYTE 정상 버퍼 밖 NUL 쓰기는 호스트 예외로 거부한다.
std::string BuildMissionLoadingMessageBody(std::string_view formatted,const std::function<std::string()>& cancelText);
// 본문 조립→현재 transient 검색→기존 갱신 또는 새 생성/현재created 검색→현재 serial 저장 순서다.
std::uint32_t ShowMissionLoadingMessage(MissionLoadingMessageState& state,std::string_view formatted,
    const MissionLoadingMessageHooks& hooks);
}
