// 현재 장치를 매호출 빌려 기존 Native 부모를 실행하며 미정 지역 버퍼를 임의0으로 채우지 않는다.
#include "client/ClientChatRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "o/OriginalText.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>

namespace netstorm::client { // 실제 Client/자산은 동기 호출이 복귀할 때까지 살아 있어야 한다.
namespace { // 원문 CRT/TYPE 조회 장치만 공유하며 별도 게임 상태를 만들지 않는다.
// 원문 진단 뒤 계속하는 장치를 기존 실제 로그 스트림에 연결한다.
void ChatAssertion(std::uint32_t line) { std::clog<<"Chat 원문 진단:"<<line<<'\n'; }
// 매호출 같은 icon TYPE의 +170 글자별 DWORD 개수를 읽는다. 최대랭크 getter는 B/C를 따로 조회한다.
std::int32_t IconCount(const GameAssets& assets,const RawSurfaceStorage& surface,std::uint8_t letter) {
    const auto kind=assets.Find("icon").block+o::kFirstAssetTypeNumber;if (kind>=surface.frames.size()) throw std::out_of_range("Chat icon 현재TYPE 물리 범위");return surface.frames[kind].Run(letter).count;
}
}
// 현재 Subscriber는 실제 부모의 각 Config 키 서식 위치에서 다시 읽는다.
void InitializeClientChatManager(Client& client) {
    auto hooks=MakeClientChatManagerHooks(client.ChatManagerGlobals());hooks.assertion=ChatAssertion;
    hooks.registerCommand=[&client](auto name,auto callback,auto flag) {
        const std::string source(name);client.DotCommandGlobals()->Register({reinterpret_cast<const std::uint8_t*>(source.c_str()),source.size()+1},callback,flag);
    }; // 같은 실제 인접 명령표에 원문 이름/함수 식별자/표시 포인터를 등록한다.
    hooks.readConfig=[&client](auto key)->std::optional<std::string> {
        std::string value;client.Configuration().ReadString(key,value);const auto bytes=o::EncodeOriginalText(value);return std::string(bytes.begin(),bytes.end());
    }; // 실제 Config의 UTF8 저장값을 원문 좁은 바이트로 돌려 CRT60/500에 전달한다. 누락/빈 값은 같은 빈 지역 문자열이다.
    o::InitializeChatManager({client.ChatManagerGlobals()->Current(),client.CurrentSubscriber(),client.CoreDataGlobals()->Active()},hooks);
}
// manager0/active0 자연 관문은 원문 wrapper가 판정하며 아이콘/Core를 선행 조회하지 않는다.
void RefreshClientChatRankPrefix(Client& client,const GameAssets& assets,const RawSurfaceStorage& surface) {
    auto hooks=MakeClientChatManagerHooks(client.ChatManagerGlobals());hooks.assertion=ChatAssertion;
    hooks.core=[&client,&surface] { const std::uint32_t metamap=surface.ownerMode.challenge;return client.CoreDataGlobals()->Read(client.GameplayGlobals()->player,metamap,{ChatAssertion}); };
    hooks.minorCount=[&assets,&surface] { return IconCount(assets,surface,'B'); };
    hooks.maximumRank=[&assets,&surface] { const auto minor=static_cast<std::uint32_t>(IconCount(assets,surface,'B'));const auto major=static_cast<std::uint32_t>(IconCount(assets,surface,'C'));return std::bit_cast<std::int32_t>(major*minor-1U); }; //004c6c20의 두 현재TYPE 조회와32비트 곱/감김을 보존한다.
    std::array<std::uint8_t,o::kChatRankOutputBytes> output,temporary; // 원문 지역 두256BYTE이며 formatter가 실제로 쓰는 부분만 먼저 정의한다.
    o::RefreshChatManagerRankPrefix({client.ChatManagerGlobals()->Current(),client.CurrentSubscriber(),client.CoreDataGlobals()->Active()},hooks,output,temporary);
}
}
