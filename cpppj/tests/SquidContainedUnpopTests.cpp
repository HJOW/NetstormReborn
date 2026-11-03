// 공통 종속 해제의 링크·진단 복귀·UI 큐 계약을 명시 입력으로 검사한다. 새 CPU 관측이나 실제 플레이 검사가 아니다.
#include "TestSupport.h"
#include "o/RawSquidContainedUnpop.h"
#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace { // 원본 파일을 바꾸지 않는 검사 소유 SID 풀과 가상 경계 입력만 구성한다.
struct ContainedUnpopScene { // 반환 기대값은 검사 본문에 직접 쓰며 제품 실행으로 만들지 않는다.
    SidPool pool; // 검사마다 독립적으로 소유하는 실제 원시 풀이다.
    Sid source{50}; // 해제할 합성 객체이며 게임 내 유닛을 생성하지 않는다.
    Sid parent{70}; // 가상 부모 getter가 반환할 현재 입력이다.
    Sid previous{}; // 가상 이전 getter가 반환할 현재 입력이다.
    std::uint32_t contentFormType{6}; // 통지 조건이 조회하는 현재 전체 DWORD다.
    std::uint32_t gumpFormType{3}; // UI 검색이 조회하는 현재 전체 DWORD다.
    std::uint32_t uiEvent{0x02000039}; // 명령 경계에 그대로 전달할 입력 토큰이다.
    std::uint32_t checking{}; // UI finder 진단의 현재 비영 DWORD다.
    bool metamap{}; // UI finder의 현재 메타맵 관문이다.
    bool boss{true}; // UI finder의 현재 권한 관문이다.
    std::size_t parentCalls{},previousCalls{},setPreviousCalls{},noticeCalls{}; // 선택된 실제 가상 경계 호출 횟수다.
    std::vector<std::uint32_t> diagnostics; // 원문 진단의 발생 순서를 저장한다.
    std::vector<std::array<std::uint32_t,3>> commands; // 실제 요청한 세 DWORD만 보관하는 명령 큐다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> onAssertion; // 진단 정상 복귀 중 변경할 명시 입력이다.
    std::function<void(Sid,Sid,std::uint32_t)> onNotice; // 부모 통지 중 변경할 명시 입력이다.

    explicit ContainedUnpopScene(OriginalEdition edition):pool(edition,24000) { // 판본별 슬롯 폭을 실제 풀에서 가져온다.
        for (std::uint16_t id=0;id<100;++id) { auto raw=Raw(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{}); } // 관측할 합성 슬롯만 초기화한다.
        Raw(source)[10]=74;Raw(source)[11]=12;Raw(parent)[10]=74;Put(parent,6,source.value);
    }
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free 진단 복귀도 같은 물리 바이트로 구성한다.
    std::uint16_t Word(Sid sid,std::size_t offset) { const auto raw=Raw(sid);return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8)); } // 정확 WORD만 읽는다.
    void Put(Sid sid,std::size_t offset,std::uint16_t value) { auto raw=Raw(sid);raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 이웃 필드를 보존하는 입력 쓰기다.
    SquidContainedUnpopState State() { return {contentFormType,gumpFormType,uiEvent,RawCheckingFlag{checking},[this] { return metamap; },[this] { return boss; }}; } // 임시 state도 검사 수명 내 같은 owner를 빌린다.
    SquidContainedUnpopHooks Hooks() { // 가상 getter의 반환 입력과 setter의 쓰기 위치를 제품 몸체와 분리한다.
        SquidContainedUnpopHooks hooks;
        hooks.parent=[this](Sid sid) { CHECK(sid==source);++parentCalls;return parent; }; // 원본 VT+0 호출의 현재 반환값이다.
        hooks.previous=[this](Sid sid) { CHECK(sid==source);++previousCalls;return previous; }; // 원본 VT+4 호출의 현재 반환값이다.
        hooks.setPrevious=[this](Sid sid,Sid value) { ++setPreviousCalls;Put(sid,Raw(sid)[10]<70 ? 14U : 16U,value.value); }; // 혼합 이웃의 Form/자산 쓰기 폭을 구별한다.
        hooks.parentNotice=[this](Sid p,Sid sid,std::uint32_t reason) { CHECK(p==parent && sid==source && reason==2);++noticeCalls;if (onNotice) onNotice(p,sid,reason); }; // 링크 정리 뒤 부모의 실제 통지 요청을 관측한다.
        hooks.command=[this](std::uint32_t event,std::uint32_t object,std::uint32_t value) { commands.push_back({event,object,value}); }; // UI 실행을 대신하지 않고 요청 인자만 관측한다.
        hooks.assertion=[this](std::string_view expression,std::string_view file,std::uint32_t line) { diagnostics.push_back(line);if (onAssertion) onAssertion(expression,file,line); }; // 정상 복귀 뒤 변경 입력을 재조회하는지 검사한다.
        return hooks;
    }
};
template<class Function> bool ContainedFailed(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; } // 선택된 미연결 경계의 예외를 확인한다.
}

TEST_CASE(SquidContainedUnpop_MixedLinksAndParentNotice) { // 머리/중간 삭제와 두 이웃 배치를 두 판본에서 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 판본별 같은 연결 계약을 독립 입력으로 실행한다.
        for (const auto nextType:{3U,74U}) { // Form과 자산의 서로 다른 prev 필드가 선택되는지 검사한다.
            for (const bool hasPrevious:{false,true}) { // 부모 머리와 중간 항목의 변경 대상을 구분한다.
                ContainedUnpopScene scene(edition);const Sid next{61};scene.previous=hasPrevious ? Sid{60} : Sid{};
                scene.Put(scene.source,4,next.value);scene.Raw(next)[10]=static_cast<std::uint8_t>(nextType);scene.Put(next,14,0xaaaa);scene.Put(next,16,0xbbbb);
                if (hasPrevious) { scene.Raw(scene.previous)[10]=6;scene.Put(scene.previous,4,scene.source.value);scene.Put(scene.parent,6,scene.previous.value); }
                scene.onNotice=[&](Sid, Sid,std::uint32_t) { CHECK(scene.Raw(scene.source)[11]==4);CHECK(scene.Word(next,nextType<70 ? 14U : 16U)==scene.previous.value); }; // 부모 통지 전에 링크/상태 쓰기가 완료되어야 한다.
                RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Detach(scene.source);
                CHECK(scene.Word(scene.parent,6)==(hasPrevious ? scene.previous.value : next.value));
                if (hasPrevious) CHECK(scene.Word(scene.previous,4)==next.value);
                CHECK(scene.Word(next,nextType<70 ? 16U : 14U)==(nextType<70 ? 0xbbbbU : 0xaaaaU));
                CHECK(scene.Word(scene.source,4)==next.value && scene.Raw(scene.source)[11]==4);
                CHECK(scene.parentCalls==1 && scene.previousCalls==1 && scene.setPreviousCalls==1 && scene.noticeCalls==1 && scene.diagnostics.empty() && scene.commands.empty());
            }
        }
    }
}

TEST_CASE(SquidContainedUnpop_DeadAndFreeParentDiagnosticReturn) { // free는 진단 후 계속하고 dead만 링크 해제를 생략한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 원문 두 판본의 진단 행 차이도 보존한다.
        for (const auto parentState:{0U,1U,2U,3U}) { // 살아 있음/free/dead/둘 다를 별도 입력으로 확인한다.
            ContainedUnpopScene scene(edition);scene.Raw(scene.parent)[11]=static_cast<std::uint8_t>(parentState);
            scene.onAssertion=[&](auto expression,auto file,auto line) { CHECK(expression=="!p.isFree()" && file=="\\Ns\\O\\Squid.cpp" && line==(edition==OriginalEdition::Patch1078 ? 0x3a8U : 0x39fU)); }; // free 진단을 빈 생략으로 바꾸지 않는다.
            RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Detach(scene.source);
            CHECK(scene.parentCalls==1 && scene.diagnostics.size()==((parentState&1U) ? 1U : 0U));
            CHECK(scene.previousCalls==((parentState&2U) ? 0U : 1U) && scene.noticeCalls==((parentState&2U) ? 0U : 1U));
            CHECK(scene.Word(scene.parent,6)==((parentState&2U) ? scene.source.value : 0U) && scene.Raw(scene.source)[11]==((parentState&2U) ? 12U : 4U));
        }
        ContainedUnpopScene changed(edition);changed.Raw(changed.parent)[11]=1;
        changed.onAssertion=[&](auto,auto,auto) { changed.Raw(changed.parent)[11]=2; }; // 진단 뒤의 현재 dead가 몸체 실행을 결정한다.
        RawSquidContainedUnpop unpop(changed.pool,changed.State(),changed.Hooks());unpop.Detach(changed.source);
        CHECK(changed.previousCalls==0 && changed.noticeCalls==0 && changed.Raw(changed.source)[11]==12);
    }
}

TEST_CASE(SquidContainedUnpop_DiagnosticKeepsSavedNextAndCurrentType) { // 늦은 진단 전에 실행한 쓰기와 저장 next/현재 타입을 구별한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 부모 머리와 prev가 모순인 진단 정상 복귀를 두 판본에서 대조한다.
        ContainedUnpopScene scene(edition);scene.previous=Sid{60};scene.Put(scene.source,4,61);scene.Raw(Sid{61})[10]=3;
        scene.onAssertion=[&](auto expression,auto file,auto line) {
            CHECK(expression=="prev == 0" && file=="\\Ns\\O\\Squid.cpp" && line==(edition==OriginalEdition::Patch1078 ? 0x3c0U : 0x3b7U));
            CHECK(scene.Word(scene.previous,4)==61 && scene.Word(Sid{61},14)==60); // 앞선 두 링크 쓰기는 진단 이전에 관측되어야 한다.
            scene.Put(scene.source,4,62);scene.Put(scene.parent,6,88);scene.Raw(scene.source)[10]=5;scene.contentFormType=5;scene.Raw(scene.source)[11]=0x8c;
        }; // 후속 부모 head는 저장 next61, 통지는 현재 타입/전역5를 사용해야 한다.
        RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Detach(scene.source);
        CHECK(scene.diagnostics.size()==1 && scene.Word(scene.parent,6)==61 && scene.Word(scene.source,4)==62 && scene.Raw(scene.source)[11]==0x84 && scene.noticeCalls==1);
    }
}

TEST_CASE(SquidContainedUnpop_SelfNextDiagnosticPreservesWrite) { // prev가 자기 자신을 next로 받는 진단 후에도 원문 쓰기는 계속된다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 인라인화된 CD setter와 패치 helper의 행 번호를 구별한다.
        ContainedUnpopScene scene(edition);scene.previous=Sid{60};scene.Put(scene.source,4,60);scene.Put(scene.parent,6,60);scene.Raw(scene.previous)[10]=3;
        scene.onAssertion=[&](auto expression,auto,auto line) { CHECK(expression=="n != getSid()" && line==(edition==OriginalEdition::Patch1078 ? 0x123U : 0x10cU));scene.Put(scene.previous,4,99); }; // 진단 중의 값을 저장 next가 다시 덮어쓴다.
        RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Detach(scene.source);
        CHECK(scene.diagnostics.size()==1 && scene.Word(scene.previous,4)==60 && scene.Word(scene.previous,14)==60 && scene.Raw(scene.source)[11]==4 && scene.noticeCalls==1);
    }
}

TEST_CASE(SquidContainedUnpop_SelectedMissingHookKeepsPartialLinks) { // 선택하지 않은 장치는 요구하지 않고 선택한 누락은 이미 수행한 효과를 보존한 채 실패한다.
    ContainedUnpopScene scene(OriginalEdition::Patch1078);scene.previous=Sid{60};scene.Put(scene.parent,6,60);scene.Put(scene.source,4,61);auto hooks=scene.Hooks();hooks.setPrevious={};
    RawSquidContainedUnpop unpop(scene.pool,scene.State(),hooks);CHECK(ContainedFailed([&] { unpop.Detach(scene.source); }));
    CHECK(scene.Word(scene.previous,4)==61 && scene.Raw(scene.source)[11]==12 && scene.noticeCalls==0); // next setter 누락 전 prev.next 쓰기는 되돌리지 않는다.
    ContainedUnpopScene form(OriginalEdition::Patch1078);form.Raw(form.source)[10]=3;auto noNotice=form.Hooks();noNotice.parentNotice={};
    RawSquidContainedUnpop formUnpop(form.pool,form.State(),noNotice);formUnpop.Detach(form.source);CHECK(form.Raw(form.source)[11]==4); // 일반 Form3 제거는 부모 통지 자체를 선택하지 않는다.
    ContainedUnpopScene asset(OriginalEdition::Patch1078);auto missingNotice=asset.Hooks();missingNotice.parentNotice={};
    RawSquidContainedUnpop assetUnpop(asset.pool,asset.State(),missingNotice);CHECK(ContainedFailed([&] { assetUnpop.Detach(asset.source); }));CHECK(asset.Raw(asset.source)[11]==4 && asset.Word(asset.parent,6)==0); // 부모 통지 누락은 완료된 해제를 취소하지 않는다.
}

TEST_CASE(SquidContainedUnpop_UiFinderFiltersFormsAndQueuesOnce) { // 상태 플래그와 무관한 Form 검색/전체 DWORD 비교/최초 한회 큐를 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 같은 UI 알림 계약을 검사한다.
        ContainedUnpopScene scene(edition);scene.Put(scene.source,6,80);scene.Raw(Sid{80})[10]=74;scene.Put(Sid{80},4,81);scene.Raw(Sid{81})[10]=69;scene.Put(Sid{81},4,82);scene.Raw(Sid{82})[10]=3;scene.Raw(Sid{82})[11]=0xff;scene.Put(Sid{82},4,83);scene.Raw(Sid{83})[10]=3;
        RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.NotifyUi(scene.source);
        CHECK(scene.commands.size()==1 && scene.commands.front()==(std::array<std::uint32_t,3>{scene.uiEvent,scene.source.value,0}));
        scene.commands.clear();scene.gumpFormType=0x10003;unpop.NotifyUi(scene.source);CHECK(scene.commands.empty()); // 타입 DWORD를 BYTE로 줄이지 않는다.
        scene.gumpFormType=74;unpop.NotifyUi(scene.source);CHECK(scene.commands.empty()); // 자산은 타입 전역과 같아도 Form 검색 대상이 아니다.
        scene.gumpFormType=3;auto hooks=scene.Hooks();hooks.command={};RawSquidContainedUnpop missing(scene.pool,scene.State(),hooks);CHECK(ContainedFailed([&] { missing.NotifyUi(scene.source); }));
        scene.Put(scene.source,6,0);missing.NotifyUi(scene.source);CHECK(scene.commands.empty()); // 비어 있는 목록은 명령 장치 없이 정상 반환한다.
    }
}

TEST_CASE(SquidContainedUnpop_UiDiagnosticReadsCurrentHeadAndGlobals) { // 진단 복귀 뒤 current head/타입/이벤트를 사용하고 패치 전용 관문을 보존한다.
    ContainedUnpopScene scene(OriginalEdition::Patch1078);scene.Raw(scene.source)[11]=14;scene.checking=0x80000000;scene.boss=false;
    scene.onAssertion=[&](auto expression,auto file,auto line) {
        CHECK(expression=="inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy" && file=="\\Ns\\O\\Squidfinder.cpp" && line==0x158);
        scene.Put(scene.source,6,80);scene.Raw(Sid{80})[10]=5;scene.gumpFormType=5;scene.uiEvent=0xf1234567;
    }; // finder는 진단 이후 새 목록과 현재 DWORD를 읽는다.
    RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.NotifyUi(scene.source);
    CHECK(scene.diagnostics.size()==1 && scene.commands.size()==1 && scene.commands.front()==(std::array<std::uint32_t,3>{0xf1234567,50,0}));
    for (const auto gate:{0U,1U,2U}) { // checking0/메타맵/권한의 각 단락평가가 진단을 생략한다.
        ContainedUnpopScene quiet(OriginalEdition::Patch1078);quiet.Raw(quiet.source)[11]=14;quiet.checking=gate ? 1U : 0U;quiet.metamap=gate==1;quiet.boss=gate==2;
        auto hooks=quiet.Hooks();hooks.assertion={};RawSquidContainedUnpop allowed(quiet.pool,quiet.State(),hooks);allowed.NotifyUi(quiet.source);CHECK(quiet.diagnostics.empty());
    }
    ContainedUnpopScene cd(OriginalEdition::Cd1072);cd.Raw(cd.source)[11]=14;cd.checking=1;cd.boss=false;auto hooks=cd.Hooks();hooks.assertion={};RawSquidContainedUnpop cdUnpop(cd.pool,cd.State(),hooks);cdUnpop.NotifyUi(cd.source);CHECK(cd.diagnostics.empty()); // CD에 패치 전용 finder 진단을 추가하지 않는다.
}

TEST_CASE(SquidContainedUnpop_UnpopNotifiesUiAfterCurrentParentEffects) { // 부모 통지로 새로 연결된 GumpForm을 뒤에서 검색하고 부모 dead여도 UI 단계는 진행한다.
    for (const bool deadParent:{false,true}) { // 분리 조기 반환과 전체 Unpop 반환을 혼동하지 않는다.
        ContainedUnpopScene scene(OriginalEdition::Patch1078);scene.Raw(scene.parent)[11]=deadParent ? 2 : 0;
        if (deadParent) { scene.Put(scene.source,6,80);scene.Raw(Sid{80})[10]=3; }
        else scene.onNotice=[&](Sid,Sid,std::uint32_t) { scene.Put(scene.source,6,80);scene.Raw(Sid{80})[10]=3;scene.uiEvent=0x02000061; }; // 실제 통지 이후 UI 검색에 보일 입력이다.
        RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Unpop(scene.source);
        CHECK(scene.commands.size()==1 && scene.commands.front()==(std::array<std::uint32_t,3>{deadParent ? 0x02000039U : 0x02000061U,50,0}));
        CHECK(scene.noticeCalls==(deadParent ? 0U : 1U) && scene.Raw(scene.source)[11]==(deadParent ? 12U : 4U));
    }
}

TEST_CASE(SquidContainedUnpop_PrefixDiagnosticRechecksContainedWithoutSettingVoid) { // Form 전용 void 선행을 공통 분리에 잘못 추가하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 contained 진단 사이의 현재 BYTE 재조회를 검사한다.
        ContainedUnpopScene scene(edition);scene.Raw(scene.source)[11]=0;
        scene.onAssertion=[&](auto expression,auto file,auto line) {
            CHECK(expression=="containedFlag" && file=="\\Ns\\O\\Squid.cpp");
            CHECK(line==(scene.diagnostics.size()==1 ? (edition==OriginalEdition::Patch1078 ? 0x3a3U : 0x39aU) : (edition==OriginalEdition::Patch1078 ? 0x3b1U : 0x3a8U)));
            if (scene.diagnostics.size()==2) scene.Raw(scene.source)[11]=0x88; // 두 번째 진단 복귀에서만 contained를 채우고 나머지 상태 비트를 보존한다.
        };
        RawSquidContainedUnpop unpop(scene.pool,scene.State(),scene.Hooks());unpop.Detach(scene.source);
        CHECK(scene.diagnostics.size()==2 && scene.Raw(scene.source)[11]==0x80 && scene.noticeCalls==1 && scene.Word(scene.parent,6)==0);
    }
}

TEST_CASE(SquidContainedUnpop_CommonVirtualGettersKeepWordAndNeighborBytes) { // 실제 생성 VT가 링크 배치를 결정하며 타입/state의 잘못된 추정을 허용하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 내장 Form4종과 대표 자산 VT를 확인한다.
        ContainedUnpopScene scene(edition);const bool patch=edition==OriginalEdition::Patch1078;
        const std::array<std::uint32_t,5> tables=patch ? std::array<std::uint32_t,5>{0x512110U,0x512098U,0x512188U,0x512208U,0x501dd0U} : std::array<std::uint32_t,5>{0x505a08U,0x505990U,0x505a80U,0x505b00U,0x500030U}; // 원문 생성자 기록값이며 실행 포인터로 사용하지 않는다.
        for (std::size_t index=0;index<tables.size();++index) { // 서로 다른 VT에서도 읽기/쓰기 폭은 원문 WORD를 유지한다.
            const bool form=index<4;auto raw=scene.Raw(scene.source);scene.Put(scene.source,0,static_cast<std::uint16_t>(tables[index]));scene.Put(scene.source,2,static_cast<std::uint16_t>(tables[index]>>16));scene.Put(scene.source,14,0xabcd);scene.Put(scene.source,16,0xbcde);scene.Put(scene.source,18,0xcdef);raw[10]=form ? 74 : 3;raw[11]=0xff;
            CHECK(CommonContainedParent(scene.pool,scene.source)==Sid{static_cast<std::uint16_t>(form ? 0xbcde : 0xabcd)});
            CHECK(CommonContainedPrevious(scene.pool,scene.source)==Sid{static_cast<std::uint16_t>(form ? 0xabcd : 0xbcde)});
            const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());CommonContainedSetPrevious(scene.pool,scene.source,Sid{0xffff});
            CHECK(scene.Word(scene.source,form ? 14U : 16U)==0xffff && scene.Word(scene.source,form ? 16U : 14U)==(form ? 0xbcdeU : 0xabcdU) && scene.Word(scene.source,18)==0xcdef);
            for (std::size_t byte=0;byte<raw.size();++byte) if (byte!=(form ? 14U : 16U) && byte!=(form ? 15U : 17U)) CHECK(raw[byte]==before[byte]); // 선택한 WORD 밖 전체 슬롯 바이트가 그대로여야 한다.
        }
        scene.Put(scene.source,0,0);scene.Put(scene.source,2,0);
        CHECK(ContainedFailed([&] { CommonContainedParent(scene.pool,scene.source); }) && ContainedFailed([&] { CommonContainedPrevious(scene.pool,scene.source); }) && ContainedFailed([&] { CommonContainedSetPrevious(scene.pool,scene.source,Sid{1}); })); // 모르는 VT를 공통 필드 추정으로 대체하지 않는다.
    }
}
