// CreateAt의 원문 일반 입력408·이름15관측을 같은 실제 분배/스크립트 큐와 대조한다.
#include "GumpRawSupport.h"
#include "client/CreateAtInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <stdexcept>

using namespace netstorm::client; //제품의 실제 명령 분배/문자열 고리/FIFO를 사용한다.
using namespace netstorm::test::gumpraw; //원문 TSV/16진수 해독만 공유하며 정답 분기는 계산하지 않는다.
namespace { //원문 BYTE를 제품에 공급하기 위한 독립 검사 도구다.
std::string CreateAtSource(std::string_view text) { const auto raw=Bytes(std::string(text));return {raw.begin(),raw.end()}; } //첫NUL 뒤 바이트까지 원래 입력을 보존한다.
}

TEST_CASE(CreateAt_Native408GeneralInputKeepsGuideGumpReleaseAndModes) { //서로 다른두 인자·권한·정지/Console·상위플래그와 실제 생성 전달을 대조한다.
    const auto rows=LoadFixture(NETSTORM_CREATE_AT_INPUT_FIXTURE).rows;CHECK(rows.size()==408);
    for (const auto& row:rows) { //Guide/Gump 소비는 원문처럼 일반 명령 분배보다 먼저 적용한다.
        CHECK(row.size()==13);if (row.size()!=13) continue;std::string trace;
        if (!Number(row[5]) && !Number(row[6])) CHECK(DispatchCreateAtInput({Number(row[0]),std::bit_cast<std::int32_t>(Number(row[1])),std::bit_cast<std::int32_t>(Number(row[2]))},Number(row[7])!=0,Number(row[8])!=0,[&](auto token) { trace="createAt:"+std::to_string(token); }));
        CHECK((trace.empty() ? "-":trace)==row[9] && row[11]=="1" && row[12]=="0"); //정지/Console은 공통 입력의 독립 관측이며 이 일반 명령에 별도 관문을 넣지 않는다.
    }
}

TEST_CASE(CreateAt_Native15NamesFirstNulAndCurrentScriptStringQueue) { //실제00542a74 이름과 전체 숫자/문자열/NUL·미등록거부를 기존 FIFO까지 대조한다.
    const auto rows=LoadFixture(NETSTORM_CREATE_AT_NAMES_FIXTURE).rows;CHECK(rows.size()==15);
    for (const auto& row:rows) { //원문 결과 열만을 기대값으로 쓰며 실제 parser/lookup/고리/큐를 호출한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};QueueScriptCommand(CreateAtSource(row[1]),queue,strings,{GameMenuScriptCode,[&](auto) { ++unknown; }});CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") { CHECK(queue.Empty() && unknown==1);continue; }
        CHECK(!unknown);const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;
        const auto event=queue.Pop(false);CHECK(queue.Empty() && event.code==Number(expected[0]) && event.x==event.y && IsGameMenuCommand(event.code));const auto parameter=std::bit_cast<std::uint32_t>(event.y);
        if (expected[1].starts_with('n')) CHECK(parameter==Number(expected[1].substr(1)));else { const auto text=strings.Resolve(parameter);CHECK(Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()})==expected[1].substr(1)); }
    }
    CHECK(GameMenuCommandValue(0x542a74U)==kCreateAtCommand && CreateAtScriptCode(std::string_view("cReAtEaT\0ignored",16))==kCreateAtCommand);CHECK(CreateAtScriptCode("CreateAts")==0xffffffffU && GameMenuScriptCode("CreateAts")==0xffffffffU);
}

TEST_CASE(CreateAt_OnlyReachedParentAndThrowingActualCallback) { //미도달 부모는 요구하지 않고 선택된 실제 생성 실패를 정상 성공/다른 부모로 바꾸지 않는다.
    CHECK(!DispatchCreateAtInput({'C',0,0},true,true,{}));CHECK(DispatchCreateAtInput({kCreateAtCommand|InputCode::kRelease,0,0},true,true,{}));CHECK(DispatchCreateAtInput({kCreateAtCommand,0,0},false,false,{}));
    bool missing{};try { static_cast<void>(DispatchCreateAtInput({kCreateAtCommand,0,0},true,false,{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    std::uint32_t calls{},token{};bool propagated{};try { static_cast<void>(DispatchCreateAtInput({kCreateAtCommand,17,std::bit_cast<std::int32_t>(0xffffffffU)},false,true,[&](auto current) { ++calls;token=current;throw std::runtime_error("명시 실제 생성 부모 실패"); })); }catch (const std::runtime_error&) { propagated=true; }CHECK(propagated && calls==1 && token==0xffffffffU);
}
