// 공통 생성 후처리의 진단 복귀/부분 쓰기만 검사하며 게임·의식·공중3종·통신을 실행하지 않는다.
#include "TestSupport.h"
#include "o/SquidFactory.h"
#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>

using namespace netstorm::o;
namespace { // 같은 실제 공장과 물리 슬롯에 명시 진단 입력만 공급한다.
struct PostCreateScene { // TYPE 사본과 실제 현재 표의 차이를 관측하는 최소 소유자다.
    bool server{true}; // 진단 정상 복귀 뒤 읽는 같은 현재 서버 플래그다.
    SidPool pool{OriginalEdition::Patch1078,32768,true,&server}; // 원본 할당 목록/물리 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 진단 복귀 뒤 다시 선택할 현재 표다.
    SquidFactory factory{pool,types}; // 처음의 빈 표 사본과 이후 현재 표를 구별한다.
    Sid sid=pool.Allocate(); // 생성자/Pop 없이 공통 후처리 한 함수만 호출한다.
    std::vector<std::uint32_t> lines; // 실제 원문 줄의 실행 순서다.
    std::function<void(std::uint32_t)> resume; // 진단 정상 복귀 중의 명시 외부 변경이다.
    // 첫 생성 이전부터 같은 현재 표/진단을 연결하고 이웃 payload를 서로 다르게 채운다.
    PostCreateScene() {
        types[74].zOrder=0x123;types[74].flags1=TypeFlag1::kHasHitPoints;types[74].maxHitPoints=0x12345678;
        types[75].zOrder=0x234;types[75].flags1=TypeFlag1::kHasHitPoints;types[75].maxHitPoints=0x23456789;
        auto raw=Raw();raw[10]=74;raw[34]=9;raw[35]=77;raw[26]=1;raw[27]=2;raw[28]=3;raw[29]=4;
        factory.BindDiagnostics({types,[this](auto expression,auto file,auto line) {
            CHECK(file=="\\Ns\\O\\Squid.cpp");CHECK(expression==(line==0x7d4 ? "!isForm()" : "!isContained()"));
            lines.push_back(line);if (resume) resume(line);
        },{}});
    }
    // 원문 인자로 받은 동일 슬롯의 실제 바이트만 수정하며 자유 목록을 재작성하지 않는다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
};
// 선택된 진단 장치가 없을 때의 실패와 이미 기록된 바이트를 구별한다.
template<class F> bool PostCreateFails(F&& operation) { try { operation(); }catch (const std::exception&) { return true; }return false; }
}

TEST_CASE(FactoryPostCreate_EntryDiagnosticsResumeCurrentTypeAndKeepTailOrder) { // 세 진단을 일괄 사전 검사로 바꾸지 않는다.
    PostCreateScene scene;auto raw=scene.Raw();raw[10]=3;raw[11]=13;
    const auto free=scene.pool.FreeCount();
    scene.resume=[&](auto line) {
        if (line==0x7d4) { CHECK(raw[35]==77 && raw[34]==9);raw[10]=75; }
        if (line==0x7d5) { CHECK(raw[35]==77 && raw[26]==1);raw[11]|=2; }
        if (line==0x7df) { CHECK(raw[35]==0x34 && raw[34]==0 && raw[26]==0x89 && raw[29]==0x23);raw[11]&=static_cast<std::uint8_t>(~8U); }
    };
    scene.factory.PostCreate(scene.sid);
    CHECK(scene.lines==(std::vector<std::uint32_t>{0x7d4,0x7d5,0x7df}));
    CHECK(raw[10]==75 && raw[11]==7 && raw[35]==0x34 && scene.pool.FreeCount()==free);
}

TEST_CASE(FactoryPostCreate_NonServerStillChecksTailAndPreservesOwnerHp) { // 비서버의 자연 분기는 owner/HP만 건너뛴다.
    PostCreateScene scene;scene.server=false;auto raw=scene.Raw();raw[11]=12;
    scene.factory.PostCreate(scene.sid);
    CHECK(scene.lines==(std::vector<std::uint32_t>{0x7d5,0x7df}));
    CHECK(raw[35]==0x23 && raw[34]==9 && raw[26]==1 && raw[27]==2 && raw[28]==3 && raw[29]==4 && raw[11]==12);
}

TEST_CASE(FactoryPostCreate_CurrentTypeAndServerAreReadAfterDiagnostics) { // 진단이 TYPE 표/슬롯/서버를 바꾸면 초기 사본으로 덮지 않는다.
    PostCreateScene scene;auto raw=scene.Raw();raw[10]=0;
    scene.resume=[&](auto line) { CHECK(line==0x7d4);raw[10]=75;scene.types[75].zOrder=0x345;scene.server=false; };
    scene.factory.PostCreate(scene.sid);
    CHECK(scene.lines==(std::vector<std::uint32_t>{0x7d4}) && raw[35]==0x45 && raw[34]==9 && raw[26]==1);
}

TEST_CASE(FactoryPostCreate_MissingSelectedDiagnosticPreservesPartialWrites) { // 입구 실패와 후단 실패의 원문 쓰기 시점을 구별한다.
    PostCreateScene scene;auto raw=scene.Raw();raw[10]=3;scene.factory.BindDiagnostics({scene.types,{},{}});
    const auto before=std::vector(raw.begin(),raw.end());
    CHECK(PostCreateFails([&] { scene.factory.PostCreate(scene.sid); }) && std::equal(before.begin(),before.end(),raw.begin()));
    raw[10]=74;raw[11]=12;
    scene.factory.BindDiagnostics({scene.types,[&](auto,auto,auto line) { CHECK(line==0x7d5);scene.factory.BindDiagnostics({scene.types,{},{}}); },{}});
    CHECK(PostCreateFails([&] { scene.factory.PostCreate(scene.sid); }));
    CHECK(raw[35]==0x23 && raw[34]==0 && raw[26]==0x78 && raw[27]==0x56 && raw[28]==0x34 && raw[29]==0x12 && raw[11]==12);
}

TEST_CASE(FactoryPostCreate_AbstractHpBranchPreservesUnusedPayload) { // abstract 비트와 HAS_HIT_POINTS의 우선순위를 순수 숫자로만 대조한다.
    PostCreateScene scene;auto raw=scene.Raw();scene.types[74].flags2=0x400000;raw[40]=0;
    scene.factory.PostCreate(scene.sid);CHECK(raw[26]==1 && raw[27]==2 && raw[28]==3 && raw[29]==4 && raw[34]==0);
    raw[40]=1;scene.types[74].flags1=0;scene.factory.PostCreate(scene.sid);
    CHECK(raw[26]==0x78 && raw[27]==0x56 && raw[28]==0x34 && raw[29]==0x12 && scene.lines.empty());
}

TEST_CASE(FactoryPostCreate_PhysicalFreeDeadSlotDoesNotAlterPoolLists) { // 원문 함수가 검사하지 않는 free/dead를 새 진입 조건으로 만들지 않는다.
    PostCreateScene scene;auto raw=scene.Raw();raw[11]=7;const auto free=scene.pool.FreeCount();const auto next=scene.pool.FirstFree(false);
    scene.factory.PostCreate(scene.sid);
    CHECK(raw[11]==7 && raw[35]==0x23 && raw[26]==0x78 && scene.pool.FreeCount()==free && scene.pool.FirstFree(false)==next && scene.lines.empty());
}
