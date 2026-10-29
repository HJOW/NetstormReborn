// 현재 장치를 매호출 빌려 기존 Native 부모를 실행하며 미정 지역 버퍼를 임의0으로 채우지 않는다.
#include "client/ClientChatRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/ChatRecipients.h"
#include "client/RawChatSubmit.h"
#include "client/RawChatFormat.h"
#include "client/RawChatMessage.h"
#include "client/RawChatPacket.h"
#include "client/RawMenuControls.h"
#include "client/HudChatGeometry.h"
#include "client/RawChatWindowContext.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "o/OriginalText.h"
#include <algorithm>
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
// 실제 strncpy/현재 캡처·UTF8 Config 값을 같은 원문 순서에 연결한다. 디스크 저장 시점은 기존 Client 프레임을 따른다.
std::uint32_t ApplyClientChatFormat(Client& client,bool front,std::string_view input) {
    const auto owner=client.ChatManagerGlobals();const auto memory=MakeClientChatManagerHooks(owner);ChatFormatHooks hooks;
    hooks.memory=memory.memory;hooks.copyFormat=memory.copyFormat;hooks.assertion=ChatAssertion;
    hooks.writeConfig=[&client](auto key,auto value) { const std::span<const std::uint8_t> bytes{reinterpret_cast<const std::uint8_t*>(value.data()),value.size()};client.Configuration().Set(key,o::DecodeOriginalText(bytes)); }; // 같은 좁은 원문 바이트를 기존 UTF8 저장 경계에서만 변환한다.
    hooks.illegalFormat=[&client](auto value) { client.WriteDiagnosticLog(29,"Illegal format \""+std::string(value)+"\"\n"); };
    return ApplyChatFormat(front,{owner->Current(),client.CurrentSubscriber()},input,owner->FormatCapture(),hooks);
}
// 원문의 flags2 지역 Identity와 같은 subscriber 입력으로 현재 Player 소유자를 조회한다.
std::uint32_t FindClientChatSenderOwner(Client& client,const RawSurfaceStorage& surface,std::string_view currentName,std::uint32_t subscriber) {
    o::PlayerIdentityRecord record;o::RawPlayerIdentity::Initialize(record,2);
    for (std::size_t index=0;index<4;++index) record[4+index]=static_cast<std::uint8_t>(subscriber>>(8*index)); // 실제 subscriber DWORD만 입력한다.
    static_cast<void>(ResolveClientChatIdentity(client,surface,currentName,record));std::uint32_t owner{};
    for (std::size_t index=0;index<4;++index) owner|=std::uint32_t(record[index])<<(8*index); // 실패도 원문이 유지한 초기 owner0을 읽는다.
    return owner;
}
// 이름 getter의 반환은 조회 성공 여부가 아니라 비영 목적 포인터라는 원문 계약을 보존한다.
ChatMessageHooks MakeClientChatMessageHooks(Client& client,std::shared_ptr<RawSurfaceStorage> surface,std::function<std::string_view()> currentName) {
    if (!surface || !currentName) throw std::invalid_argument("Chat 일반 조합의 실제 표면/현재 이름 getter 필요");
    ChatMessageHooks hooks;hooks.memory=[owner=client.ChatManagerGlobals()](auto token) { return owner->Bytes(token); };hooks.currentName=currentName;
    hooks.nameForSubscriber=[&client,surface=std::move(surface),name=std::move(currentName)](auto output,auto subscriber) {
        o::PlayerIdentityRecord record;o::RawPlayerIdentity::Initialize(record,0);
        for (std::size_t index=0;index<4;++index) record[4+index]=static_cast<std::uint8_t>(subscriber>>(8*index)); // 실제 이름 getter의 subscriber DWORD만 입력한다.
        std::string_view result="SOMEONE"; // 원문1078의 현재 기본 실패 이름이며 조회 실패에도 목적 버퍼에 복사한다.
        if (ResolveClientChatIdentity(client,*surface,name(),record)) {
            const auto bytes=std::span<const std::uint8_t>(record).subspan(8);const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 대상 이름 getter의 실제 NUL 없음");
            result={reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
        }
        if (output.size()<=result.size()) throw std::out_of_range("Chat 대상 이름 getter의 실제 목적 물리 부족");std::copy(result.begin(),result.end(),output.begin());output[result.size()]=0;
        return 1U; // 실제 목적 포인터의 비영 별칭이며 실패 이름을 썼어도0을 반환하지 않는다.
    };
    return hooks;
}
// 같은 현재 manager/이름을 원문 say와 실제 패킷에 전달하며 지역 이름의 미작성 꼬리는 채우지 않는다.
void BindClientChatMessage(RawChatWindowContext& context,Client& client,std::shared_ptr<RawSurfaceStorage> surface,std::function<std::string_view()> currentName,ChatPacketHooks packet) {
    auto message=MakeClientChatMessageHooks(client,std::move(surface),std::move(currentName));message.deliver=[packet=std::move(packet)](auto manager,auto recipients,auto body) { return SendChatPacket(manager,recipients,body,packet); };
    context.message=[&client,message=std::move(message)](auto input) { std::array<std::uint8_t,kChatMessageNameBytes> name;return SayChatMessage(input,name,{client.ChatManagerGlobals()->Current(),client.CurrentSubscriber()},message); };
}
// 제출 지역의 미작성 꼬리를 보존하고 같은 장기 명령표·현재 UI에 효과를 분배한다.
void BindClientChatSubmit(RawChatWindowContext& context,Client& client,RawMenuControls& menus) {
    if (!context.resolve || !context.translate) throw std::invalid_argument("Chat 제출의 실제 Identity/번역 장치 필요");
    context.sendChat=[&client,&menus,resolve=context.resolve,translate=context.translate,message=context.message](auto manager,auto input) {
        auto hooks=MakeClientChatSubmitHooks(client.ChatManagerGlobals());hooks.assertion=ChatAssertion;hooks.resolve=resolve;hooks.translate=translate;hooks.records=[&client] { return client.ChatRecords(); };
        hooks.inputVisible=[&client,&menus](auto visible) {
            const auto current=client.ChatWindowGlobals()->window.current;if (!current) return;
            const auto* geometry=menus.ChatGeometryBackend();if (!geometry) throw std::logic_error("Chat 제출의 현재 입력 표시 장치 없음");geometry->SetInputVisible(current,visible);
        }; //0042f270은 현재 root0에서 하위 장치를 조회하지 않는다.
        hooks.dispatch=[&client,&menus,inputVisible=hooks.inputVisible,message](auto body) {
            DotCommandDispatchHooks commands;
            commands.invoke=[&client,&menus,inputVisible,message](auto callback,auto argument) -> std::uint32_t {
                if (callback==0x42f680U || callback==0x42e520U) return menus.InvokeChatCommand(callback);
                if (callback==0x42ddf0U || callback==0x42def0U) {
                    const auto end=std::find(argument.begin(),argument.end(),0);if (end==argument.end()) throw std::out_of_range("Chat 서식 인자의 실제 NUL 없음");
                    return ApplyClientChatFormat(client,callback==0x42def0U,{reinterpret_cast<const char*>(argument.data()),static_cast<std::size_t>(end-argument.begin())});
                }
                if (callback==0x42f1e0U) {
                    if (!client.ChatManagerGlobals()->Current()) return 0;
                    if (argument.empty()) throw std::out_of_range("Chat say의 실제 인자 물리 없음");
                    if (argument.front()==0) { inputVisible(0);return 0; }
                    if (argument.front()==' ') return 0;
                    if (!message) throw std::logic_error("Chat 일반 본문의 실제 서식·송신 미연결");return message(argument);
                } // 선행 관문은 장치를 요구하지 않으며 나머지는 같은 실제 say/조합/패킷에 넘긴다.
                throw std::logic_error("Chat 제출의 실제 명령 callback 미연결:"+std::to_string(callback));
            };
            const auto owner=client.DotCommandGlobals();static_cast<void>(DispatchDotCommand(owner->Bytes(),owner->DispatchState(),body,commands));
        };
        o::PlayerIdentityRecord identity;std::array<std::uint8_t,kChatSubmitWorkBytes> work; // 원문60BYTE와 인접4097BYTE이며 사용 전 본문이 작성하는 부분만 정의한다.
        SubmitChatInput(manager,input,identity,work,hooks);
    };
}
}
