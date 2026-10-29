// 현재 장치를 매호출 빌려 기존 Native 부모를 실행하며 미정 지역 버퍼를 임의0으로 채우지 않는다.
#include "client/ClientChatRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/ChatRecipients.h"
#include "client/RawChatSubmit.h"
#include "client/RawMenuControls.h"
#include "client/HudChatGeometry.h"
#include "client/RawChatWindowContext.h"
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
//매조회에서현재Playerbase를빌리며다른Session의고정뷰나이름사본을저장하지않는다.
bool ResolveClientChatIdentity(Client& client,const RawSurfaceStorage& surface,std::string_view currentName,o::PlayerIdentityRecord& record) {
    o::PlayerIdentityReads reads;reads.localOwner=[&client] { return client.GameplayGlobals()->player.battleOwner; };reads.subscriber=[&client] { return client.CurrentSubscriber(); };
    reads.address=[&client] { return client.GameplayGlobals()->LocalAddress(); };reads.name=[currentName] { return currentName; }; //원문이읽는현재로컬정보를같은동기호출동안빌린다.
    reads.battle=[&surface] { return surface.ownerMode.battle; };reads.editor=[&client] { return client.GameplayGlobals()->world.editor!=0; };reads.metamap=[&surface] { return surface.ownerMode.challenge; }; //모드에따른실제검색순서는기존원문본문에남긴다.
    o::RawPlayerIdentity identity(client.Edition(),client.CoreDataGlobals()->Players(),{},client.ChatRecords(),std::move(reads));return identity.Resolve(record); //메타맵기록은선택시기존명시경계로남으며일반Player/Chat행으로대체하지않는다.
}
//현재이름getter는매Identity진입에서다시읽고두UI소비자가같은검사부모를사용한다.
void BindClientChatRecipients(RawChatWindowContext& context,Client& client,std::shared_ptr<RawSurfaceStorage> surface,std::function<std::string_view()> currentName) {
    if (!currentName || !context.translate || !surface) throw std::invalid_argument("Chat 대상 검사의 현재 이름/번역/표면 장치 필요");
    context.resolve=[&client,surface=std::move(surface),name=std::move(currentName)](auto& record) { return ResolveClientChatIdentity(client,*surface,name(),record); };
    context.recipient=[&client,resolve=context.resolve,translate=context.translate](auto text) {
        o::PlayerIdentityRecord scratch; //원문지역의미작성꼬리를임의0으로만들지않으며본문이초기화하는필드만사용한다.
        return CheckChatRecipients(client.ChatManagerGlobals()->Current(),text,scratch,{resolve,translate});
    };
    context.editor.recipient=context.recipient;
    context.editor.complete=[&client,translate=context.translate](auto callback,auto word,auto length,bool backward) {
        if (callback!=0x42e540U) throw std::logic_error("실제 Console의 미지원 자동완성 callback");ChatNameCompletionHooks hooks;hooks.translate=translate;hooks.records=[&client] { return client.ChatRecords(); };hooks.assertion=ChatAssertion;return CompleteChatName(client.ChatWindowGlobals()->completion,word,length,backward,hooks);
    }; //같은Client의현재Chat100행을새단어때조회하고NULL순환은장기BSS만사용한다.
    context.diagnostic=[&client](auto level,auto format,auto parameter) {
        if (format!="Could not lookup a name for subId == %d\n") throw std::logic_error("Chat 이름 조회 진단의 미지원 원문 서식");
        client.WriteDiagnosticLog(std::bit_cast<std::int32_t>(level),"Could not lookup a name for subId == "+std::to_string(std::bit_cast<std::int32_t>(parameter))+"\n"); //원문의first인자와signed%d를보존해현재공유trace에전달한다.
    };
}
// 제출 지역의 미작성 꼬리를 보존하고 같은 장기 명령표·현재 UI에 로컬 효과를 분배한다.
void BindClientChatSubmit(RawChatWindowContext& context,Client& client,RawMenuControls& menus) {
    if (!context.resolve || !context.translate) throw std::invalid_argument("Chat 제출의 실제 Identity/번역 장치 필요");
    context.sendChat=[&client,&menus,resolve=context.resolve,translate=context.translate](auto manager,auto input) {
        auto hooks=MakeClientChatSubmitHooks(client.ChatManagerGlobals());hooks.assertion=ChatAssertion;hooks.resolve=resolve;hooks.translate=translate;hooks.records=[&client] { return client.ChatRecords(); };
        hooks.inputVisible=[&client,&menus](auto visible) {
            const auto current=client.ChatWindowGlobals()->window.current;if (!current) return;
            const auto* geometry=menus.ChatGeometryBackend();if (!geometry) throw std::logic_error("Chat 제출의 현재 입력 표시 장치 없음");geometry->SetInputVisible(current,visible);
        }; //0042f270은 현재 root0에서 하위 장치를 조회하지 않는다.
        hooks.dispatch=[&client,&menus,inputVisible=hooks.inputVisible](auto body) {
            DotCommandDispatchHooks commands;
            commands.invoke=[&client,&menus,inputVisible](auto callback,auto argument) -> std::uint32_t {
                if (callback==0x42f680U || callback==0x42e520U) return menus.InvokeChatCommand(callback);
                if (callback==0x42f1e0U) {
                    if (!client.ChatManagerGlobals()->Current()) return 0;
                    if (argument.empty()) throw std::out_of_range("Chat say의 실제 인자 물리 없음");
                    if (argument.front()==0) { inputVisible(0);return 0; }
                    if (argument.front()==' ') return 0;
                    throw std::logic_error("Chat 일반 본문의 실제 서식·송신 미연결");
                } //0042f1e0의 현재 manager/공백/빈 본문 관문만 수행하며 나머지를 빈 성공으로 바꾸지 않는다.
                throw std::logic_error("Chat 제출의 실제 명령 callback 미연결:"+std::to_string(callback));
            };
            const auto owner=client.DotCommandGlobals();static_cast<void>(DispatchDotCommand(owner->Bytes(),owner->DispatchState(),body,commands));
        };
        o::PlayerIdentityRecord identity;std::array<std::uint8_t,kChatSubmitWorkBytes> work; // 원문60BYTE와 인접4097BYTE이며 사용 전 본문이 작성하는 부분만 정의한다.
        SubmitChatInput(manager,input,identity,work,hooks);
    };
}
}
