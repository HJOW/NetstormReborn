// 정렬 부착의 늦은 Pop 실패에서도 원문 링크/부모/contained 부분 효과가 유지되는지 확인한다.
#include "TestSupport.h"
#include "FormTypeInputs.h"
#include "o/ContentFormHost.h"
#include "o/SquidFactory.h"
#include "RawSceneSupport.h"
#include "o/RawContentAttach.h"
#include <array>
#include <stdexcept>

namespace { // 실제 Pop 장치 진입의 중단과 앞선 호스트 선행 거부를 구별한다.
using namespace netstorm::o;
struct ContentPopStop {}; // 일반 생산 목록의 Pop 경계에서만 발생하는 관측 표식이다.
// 비정렬 WORD만 읽어 다른 payload와 풀 목록을 변경하지 않는다.
std::uint16_t CurrentContentWord(const SidPool& pool,Sid sid,std::size_t at) { const auto raw=pool.Slot(sid);return std::uint16_t(raw[at])|(std::uint16_t(raw[at+1])<<8); }
}

// 두 대여 모드에서 통지 뒤 실제 TYPE 변경의 소비 위치를 확인하며 사제/전투 꼬리는 실행하지 않는다.
TEST_CASE(ContentAttachCurrent_ActualHostReadsBorrowedTypesAfterNotice) {
    for (const bool current:{false,true}) { // 독립 사본과 실제 현재 표 대여의 차이다.
        SidPool pool(OriginalEdition::Patch1078,32768);auto types=netstorm::test::MakeFormTypeInputs(188);SquidFactory factory(pool,types);
        const auto parent=factory.Create(90,2),form=factory.CreateContained(2);auto raw=pool.AllocatedBytes(form);raw[18]=85;raw[19]=0;
        std::uint32_t notices{},captured{};ContentFormState state;ContentFormHost host(pool,types,[&](Sid,Sid,std::uint32_t) {
            ++notices;types[85].flags2=0x200000;pool.AllocatedBytes(parent)[10]=255; // 현재 TYPE를 다시 읽으면 부모 TYPE의 물리 오류가 battle0 판정보다 먼저 드러난다.
        },{},&state,[&](Sid,std::uint32_t) { ++captured; });
        if (current) host.BindCurrentTypes(types);
        host.BindPop([&](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]&=static_cast<std::uint8_t>(~4U); });
        bool stopped=false;try { host.Attach(form,parent,0x10); } catch (const std::out_of_range&) { stopped=true; }
        CHECK(stopped==current && notices==1 && captured==0 && !state.inBattle && CurrentContentWord(pool,parent,6)==form.value);
    }
}

namespace { // 독립 PE 입력과 결과 직렬화만 제공하며 기대 링크를 계산하지 않는다.
using namespace netstorm::test::rawscene;
constexpr std::uint32_t kContentTable=0x14000000; // Native 격리 가상 표의 명시 토큰이다.
std::span<std::uint8_t> CurrentContentRaw(SidPool& pool,std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // raw fixture는 free 슬롯도 같은 물리로 다룬다.
}

// 원문41행의 정렬/Pop·통지 실패/진단 정상복귀/저장 VT·현재 getter를 전체250BYTE와 대조한다.
TEST_CASE(ContentAttachCurrent_IndependentNative41FrontAndPartialEffects) {
    std::size_t cases{};
    for (const auto& row:LoadFixture(NETSTORM_CONTENT_ATTACH_CURRENT_FIXTURE).rows) { // 제품 구현 전에 고정한 PE 출력만 기대값으로 읽는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;const auto layout=Number(row[0]),state=Number(row[1]),profile=Number(row[2]);
        SidPool pool(OriginalEdition::Patch1078,32768);
        for (std::uint16_t sid=0;sid<16;++sid) std::fill(CurrentContentRaw(pool,sid).begin(),CurrentContentRaw(pool,sid).end(),std::uint8_t{}); // Native의 초기 입력 범위다.
        std::uint32_t index{};
        for (const auto sid:std::array<std::uint16_t,4>{5,7,9,11}) { // payload까지 서로 같은 입력을 두고 각 VT만 구분한다.
            auto raw=CurrentContentRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0x71});Put(raw,0,kContentTable+index++*0x100U);
            for (const auto at:{4U,6U,14U,16U,20U}) Put(raw,at,0,2); // 같은 WORD 입력이며 출력 예상값이 아니다.
            raw[10]=6;raw[11]=4;Put(raw,18,85,2);raw[23]=0;
        }
        auto own=CurrentContentRaw(pool,5);auto first=CurrentContentRaw(pool,7);auto parent=CurrentContentRaw(pool,9);auto last=CurrentContentRaw(pool,11);own[11]=static_cast<std::uint8_t>(state);
        if (layout) { Put(parent,6,7,2);Put(first,4,11,2);Put(last,14,7,2);first[23]=layout==1 ? 2:0;last[23]=layout==1 ? 3:1;if (layout==3) { first[10]=1;last[23]=2; } }
        if (profile>=3 && profile<=5) { parent[11]=6;own[11]=6; }
        std::uint32_t content=6;std::string trace,cause="returned";const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 선택된 경계 순서만 기록한다.
        ContentAttachHooks hooks;hooks.contentType=[&] { return content; };
        hooks.assertion=[&](auto,auto,auto line) { event("D"+std::to_string(line));if (profile==4 || profile==5) { CurrentContentRaw(pool,line==0xbe6 ? 9:5)[11]=4;if (profile==5) { content=1;first[10]=last[10]=1; } } };
        hooks.previous=[&](Sid sid,std::uint32_t table) { event("G"+std::to_string(sid.value)+','+std::to_string(table));if (profile==7) throw std::string("previous");if (profile==6) Put(own,0,kContentTable+0x400);return Sid{CurrentContentWord(pool,sid,14)}; };
        hooks.setPrevious=[&](Sid sid,std::uint32_t table,Sid value) { event("R"+std::to_string(sid.value)+','+std::to_string(table)+','+std::to_string(value.value));Put(CurrentContentRaw(pool,sid.value),14,value.value,2); };
        hooks.setParent=[&](Sid sid,std::uint32_t table,Sid value) { event("P"+std::to_string(sid.value)+','+std::to_string(table)+','+std::to_string(value.value));Put(CurrentContentRaw(pool,sid.value),16,value.value,2); };
        hooks.pop=[&](Sid,std::uint32_t flags) { event("O"+std::to_string(flags));if (profile==1) throw std::string("pop");own[11]&=static_cast<std::uint8_t>(~4U); };
        hooks.notice=[&](Sid sid,Sid child,std::uint32_t reason) { event("N"+std::to_string(sid.value)+','+std::to_string(child.value)+','+std::to_string(reason));if (profile==2) throw std::string("notice"); };
        try { if (AttachSortedContent(pool,Sid{5},Sid{9},0x10,hooks)) cause="attached"; } catch (const std::string& stopped) { cause=stopped; }
        std::string bytes;for (const auto sid:std::array<std::uint16_t,5>{0,5,7,9,11}) { if (!bytes.empty()) bytes+=';';bytes+=Hex(pool.Slot(Sid{sid})); } // 미수정 슬롯0/이웃 payload도 보존돼야 한다.
        CHECK(cause==row[3]);CHECK((trace.empty()?"-":trace)==row[4]);CHECK(bytes==row[5]);++cases;
    }
    CHECK(cases==41);
}

// void 진단과 생산 TYPE의 실제 소비 위치보다 정렬 링크가 먼저 완료돼야 한다.
TEST_CASE(ContentAttachCurrent_ActualHostReachesPopAfterLinksDespiteLateInvalidInput) {
    for (const bool missingVoid:{true,false}) { // 첫 입력은 void0, 둘째는 늦게 읽을 범위 밖 생산 TYPE다. 실제 포획/전투는 없다.
        SidPool pool(OriginalEdition::Patch1078,32768);auto types=netstorm::test::MakeFormTypeInputs(188);SquidFactory factory(pool,types);
        const auto parent=factory.Create(90,2),form=factory.CreateContained(2);auto raw=pool.AllocatedBytes(form);
        raw[18]=missingVoid ? 85:255;raw[19]=missingVoid ? 0:255;if (missingVoid) raw[11]&=static_cast<std::uint8_t>(~4U);
        std::uint32_t calls{};ContentFormHost host(pool,types,[](Sid,Sid,std::uint32_t) { throw std::logic_error("늦은 Pop 실패 뒤 통지 도달 오류"); });
        host.BindPop([&](Sid current,std::uint32_t flags) {
            ++calls;CHECK(current==form && flags==0x110 && CurrentContentWord(pool,parent,6)==form.value && CurrentContentWord(pool,form,16)==parent.value && (pool.Slot(form)[11]&8));throw ContentPopStop{};
        });
        bool stopped=false;try { host.Attach(form,parent,0x10); } catch (const ContentPopStop&) { stopped=true; }
        CHECK(stopped && calls==1 && CurrentContentWord(pool,parent,6)==form.value && CurrentContentWord(pool,form,4)==0 && CurrentContentWord(pool,form,14)==0 && CurrentContentWord(pool,form,16)==parent.value && (pool.Slot(form)[11]&8));
    }
}
