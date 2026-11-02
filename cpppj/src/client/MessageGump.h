// 원본10.78의004d0600 중 Pause(kind11)/게임시간(kind16)/Warning(kind17)을 기존 TextGump에 연결한다.
#pragma once
#include "client/TextGump.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 원본+a0 StyleText와+a4/+a8/+b0/+b8 메시지 검색/기한 필드를 같은 노드에 둔다.
struct MessageGumpNode:TextGumpNode {
    std::int32_t kind{},player{}; // 원본+a4/+a8의 종류와 소유자 검색 키다.
    std::uint32_t reserved{}; // 원본+ac는 이 생성자가 쓰지 않는 필드다.
    double deadline{}; // 원본+b0의 벽 시각 기한이며 Pause는0으로 시작한다.
    std::int32_t parameter{}; // 원본+b8의 메시지 검색 매개변수다.
    std::uint32_t trailing{}; // 원본+bc는 이 생성자가 쓰지 않는 필드다.
};
// 실제 TextGump/이름 조회/번역/StyleText/측정을 공급하며 별도 메시지 표는 만들지 않는다.
struct MessageGumpHooks {
    std::function<MessageGumpNode*(std::uint32_t)> node; // 기본/파생이 같은 저장소를 조회한다.
    std::function<void(std::uint32_t)> constructText; // 실제 TextGump::Construct(token,0,0,0,0)이다.
    std::function<std::uint32_t()> challenge; // 매 생성 시 현재00594fc8을 읽는다.
    std::function<void(std::int32_t)> playerName,challengePlayerName; // 48f480/42bbb0의 실제 조회다. Pause는 반환 이름을 쓰지 않는다.
    std::function<std::string(std::string_view)> translate; // 실제 원문 Pause 문자열을 현재 언어로 번역한다.
    std::function<void(std::uint32_t,std::string_view)> style; // 같은 기본 문맥으로4bc800의 StyleText를 작성한다.
    std::function<std::int32_t()> gameTimeSeconds; //004a68b0→ftol의현재CRT정수초이며게임FrameTime을대신쓰지않는다.
    std::function<double()> wallSeconds; //00424930은현재벽시각을읽은뒤0.1초를더한다.
    std::function<void()> warningSound; //kind17이공통이름조회뒤재생하는newWarnMessage.wav다.
    std::function<void(std::uint32_t,bool)> idle; //같은노드를기존00465330표에등록한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> fit; // 기존 TextGump::Fit(0,0)의 측정/단정도 저장이다.
};
std::string FormatWarningMessage(std::string_view pattern,std::optional<std::string_view> source); //한문자열의실제%s/폭/정밀도/%%서식을처리하며원문NULL은(null),입력첫NUL은종료로구별한다.
// 등록/루트20칸/Draw/삭제는 기존 MessageBoard·TextGump·GumpLifetime이 담당한다.
class MessageGump {
public:
    MessageGump(const GumpCreateState& create,MessageGumpHooks hooks); // 실제10.78 공통 생성 상태와 경계를 확인한다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t kind,std::int32_t player,std::int32_t parameter,std::optional<std::string_view> source=std::nullopt) const; //kind11/16/17을생성한다.kind17의nullopt는원문NULL이며빈문자열과구별한다.
private:
    MessageGumpNode& Node(std::uint32_t token) const; // 경계 호출 뒤 같은 현재 노드를 다시 조회한다.
    MessageGumpHooks hooks_; // 현재 실제 저장소와 공통 처리기를 빌리는 안정된 경계다.
};
}
