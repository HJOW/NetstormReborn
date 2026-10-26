// 원본 일반 입력과 현재 저장 부모의166관측을 기대값 계산 없이 현재 바이트·효과·부분 중단으로 대조한다.
#include "RawSceneSupport.h"
#include "client/EditorSaveGoMain.h"
#include <map>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 새 검사 입력/관측 이름은 다른 기존 TU와 공유하지 않는다.
struct SaveGoMainStop {}; // 원문 하위의 명시 중단이며 정상 저장 복귀와 구별한다.
// 원문 순서로 발생한 조회/큰 하위/직접 next 변경을 구분자 하나로 기록한다.
void SaveGoMainTrace(std::string& trace,std::string entry) { if (!trace.empty()) trace+=';';trace+=entry; }
// 실제 원문 Template9 바이트를 저장 형식에서 되돌린다.
std::vector<std::uint8_t> SaveGoMainHex(const std::string& text) {
    if (text.size()%2) throw std::invalid_argument("SaveGoMain hex 폭 오류");std::vector<std::uint8_t> bytes;
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(index,2),nullptr,16))); // 두 글자가 메모리 한BYTE다.
    return bytes;
}
}
// 첫12열만 입력으로 설치하고 뒤9열은 실제 원문 관측과 비교할 때만 읽는다.
TEST_CASE(EditorSaveGoMain_ReplaysNative166CurrentReadsAndPartialStops) {
    const auto fixture=LoadFixture(NETSTORM_EDITOR_SAVE_GO_MAIN_FIXTURE);std::size_t checked=0,stops=0;
    for (const auto& row:fixture.rows) { // 두 x87 제어어의 독립 관측도 각각 같은 C++ 정수 부모를 재생한다.
        if (row[0]=="name") continue;CHECK(row.size()==21);if (row.size()!=21) continue;
        auto owner=Number(row[5]),battle=Number(row[6]),fort=Number(row[7]),next=Number(row[9]);auto headers=SaveGoMainHex(row[8]);CHECK(headers.size()==180);
        std::string trace;std::map<std::string,std::uint32_t> calls;bool stopped=false;
        const auto effect=[&](std::string kind,std::uint32_t slot) { // 명시 callback이 변경한 실제 현재 필드만 같은 저장소에 적용한다.
            SaveGoMainTrace(trace,kind+"Save:"+std::to_string(slot));const auto trigger=kind+std::to_string(++calls[kind]);
            if (row[10]!="-") for (const auto& entry:Split(row[10],';')) { // 입력 mutation만 해석하며 원문 기대 trace로 변경을 만들지 않는다.
                const auto fields=Split(entry,'/');if (fields.size()!=3) throw std::logic_error("SaveGoMain mutation 폭");if (fields[0]!=trigger) continue;
                const auto address=Number(fields[1]),value=Number(fields[2]);
                switch (address) { case 0x540cac:owner=value;break;case 0x594fbc:battle=value;break;case 0x594fb8:fort=value;break;case 0x5c8878:next=value;break;
                default:if (address<0x5c89f0 || address+4>0x5c89f0+headers.size()) throw std::out_of_range("SaveGoMain 명시 주소 범위");Put(headers,address-0x5c89f0,value);break; }
            }
            if (row[11]!="-") for (const auto& entry:Split(row[11],';')) if (entry==trigger) throw SaveGoMainStop{}; // 하위에 도달한 뒤 현재 변경은 보존한다.
        };
        CurrentMapSnapshotReads reads;
        reads.owner=[&] { SaveGoMainTrace(trace,"owner:"+std::to_string(owner));return owner; };
        reads.manager=[&](auto slot) { if (slot>=9) throw std::out_of_range("SaveGoMain 검사 Template 범위");const auto value=Get(headers,slot*20U);SaveGoMainTrace(trace,"manager:"+std::to_string(slot)+':'+std::to_string(value));return value; };
        reads.battle=[&] { SaveGoMainTrace(trace,"battle:"+std::to_string(battle));return battle; };reads.fort=[&] { SaveGoMainTrace(trace,"fort:"+std::to_string(fort));return fort; };
        CurrentMapSnapshotHooks hooks;hooks.battle=[&](auto slot) { effect("battle",slot); };hooks.fort=[&](auto slot) { effect("fort",slot); };
        const auto before=next;const InputEvent event{Number(row[2]),std::bit_cast<std::int32_t>(Number(row[3])),std::bit_cast<std::int32_t>(Number(row[4]))};
        try { CHECK(DispatchEditorSaveGoMain(event,next,[&] { SnapshotCurrentMap(reads,hooks); })); }catch (const SaveGoMainStop&) { stopped=true; }
        if (!stopped && next!=before) SaveGoMainTrace(trace,"next:"+std::to_string(next)); // 정상 복귀에서 관측한 실제 next 변경만 기록한다.
        CHECK(stopped==(Number(row[12])==0));CHECK(owner==Number(row[13]) && battle==Number(row[14]) && fort==Number(row[15]));
        CHECK(Hex(headers)==row[16] && next==Number(row[17]));CHECK((trace.empty() ? "-" : trace)==row[18]);CHECK(Number(row[20])>0);
        if (!stopped) CHECK(Number(row[19])==0);else ++stops;++checked;
    }
    CHECK(checked==166 && stops==4);
}
// No/뗌/다른 명령은 현재 저장 장치를 요구하지 않으며 선택된 저장 실패 뒤 next는 바뀌지 않는다.
TEST_CASE(EditorSaveGoMain_NoReleaseAndMissingSelectedEffects) {
    std::uint32_t next=91;CHECK(!DispatchEditorSaveGoMain({0x020000caU,0,1},next,{}));CHECK(next==91);
    CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand|InputCode::kRelease,0,1},next,{}));CHECK(next==91);
    CHECK(DispatchEditorSaveGoMain({kSaveGoMainCommand,1,0},next,{}));CHECK(next==3);next=92;
    bool missing=false;try { static_cast<void>(DispatchEditorSaveGoMain({kSaveGoMainCommand,0,1},next,{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing && next==92);
    CurrentMapSnapshotReads reads;reads.owner=[] { return 7U; };reads.manager=[](auto slot) { CHECK(slot==7);return 0U; };SnapshotCurrentMap(reads,{}); // manager0이면 이후 모드/하위를 요구하지 않는다.
}
