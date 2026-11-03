// 원문 타입 변경128행/가상 입력36행의 전체 슬롯과 효과 순서를 허용오차 없이 재생한다.
#include "RawSceneSupport.h"
#include "o/SquidFactory.h"
#include "o/RawObjectConversion.h"
#include <bit>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 입력 준비와 trace는 검사 장치이며 기대값은 독립 원문 TSV에서만 읽는다.
constexpr Sid kConversionSid{50}; // Native의 실제 this는 POOL+50*50이다.
// 큰 게임 장치 없이 같은 원본 슬롯/타입/생성자 몸체를 조립한다.
struct ConversionScene {
    SidPool pool{OriginalEdition::Patch1078,24000}; // 원문의50BYTE 물리 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 번호순의 명시 TYPE 입력이다.
    std::unique_ptr<SquidFactory> factory; // 원본 생성자 recipe와 같은 풀을 사용한다.
    std::uint32_t buried{112},mutation{}; // 실제 비석 전역과 Unpop 경계의 명시 효과 입력이다.
    std::string trace; // 원문과 같은 진단/공간/부모 순서다.
    // 검사 입력만 물리 슬롯에 직접 설치한다. 제품 API의 상태 관문을 완화하지 않는다.
    std::span<std::uint8_t> Raw() { return {const_cast<std::uint8_t*>(pool.Bytes().data())+kConversionSid.value*50U,50}; }
    // 현재 trace에 하나의 실제 외부 효과를 추가한다.
    void Emit(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 원문 부모의 진단 복귀를 기록하며 기대 출력이나 상태를 고치지 않는다.
    void Diagnose(std::uint32_t line) { Emit("A,"+std::to_string(line)); }
    // Native와 같은 sentinel/생성자/foot 값을 새 입력으로 공급한다.
    void Prepare(std::uint32_t old,std::uint32_t next,std::uint32_t state,std::uint32_t mismatch) {
        trace.clear();mutation=0;auto raw=Raw();
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*7+9); // 모든 미사용 BYTE도 독립 입력과 일치시킨다.
        Put(raw,0,old==112 ? 0x50a748U : 0x502370U);raw[10]=static_cast<std::uint8_t>(old);raw[11]=static_cast<std::uint8_t>(state);Put(raw,12,next,2);Put(raw,14,std::bit_cast<std::uint32_t>(100.5F));Put(raw,18,std::bit_cast<std::uint32_t>(122.25F));
        types.assign(188,RiftTypeRecord{});
        for (const auto kind:{112U,114U,116U,80U}) { types[kind].footX=1;types[kind].footY=1; } // 원문의 현재 네 TYPE 입력이다.
        types[112].constructorAddress=0x4625f0;types[114].constructorAddress=0x420e40;types[116].constructorAddress=0x420e60;
        if (old!=next) { types[next].footX=(mismatch&1) ? 2 : 1;types[next].footY=(mismatch&2) ? 3 : 1; }
        factory=std::make_unique<SquidFactory>(pool,types);
    }
    // 원문에서 분리한 실제 장치 효과만 제공하며 객체 전환은 제품 본체가 수행한다.
    ObjectConversionHooks Hooks(bool bindWorld=false) {
        ObjectConversionHooks hooks;
        hooks.unpop=[this](Sid sid,auto flags) { CHECK(sid==kConversionSid);Emit("U,"+std::to_string(flags));auto raw=Raw();raw[11]|=4;if (mutation==1) Put(raw,12,116,2);else if (mutation==2) raw[10]=116; };
        hooks.repop=[this](Sid sid,auto flags) { CHECK(sid==kConversionSid);auto raw=Raw();Emit("P,"+std::to_string(Get(raw,14))+','+std::to_string(Get(raw,18))+','+std::to_string(flags));raw[11]=static_cast<std::uint8_t>(raw[11]&~4U); };
        hooks.retype=[this](Sid sid,auto type) { factory->Retype(sid,type,[this](auto line) { Diagnose(line); }); };
        hooks.defaultFrame=[](auto type)->std::uint32_t { switch (type) { case 112:return 17;case 114:return 21;case 116:return 33;case 80:return 5;default:throw std::logic_error("명시 원문 기본프레임 TYPE 범위"); } };
        hooks.frame=[this](Sid sid,auto value) { CHECK(sid==kConversionSid);Put(Raw(),36,value); };hooks.qa=[this](Sid sid,auto value) { CHECK(sid==kConversionSid);Put(Raw(),12,value,2); };
        hooks.base=[this](Sid sid,auto code) { CHECK(sid==kConversionSid);Emit("B,"+std::to_string(code));return 77U; };
        if (bindWorld) return MakeObjectConversionHooks(pool,*factory,std::move(hooks),[this](std::string_view expression,std::string_view file,std::uint32_t line) { CHECK(!expression.empty() && file=="\\Ns\\O\\Squid.cpp");Diagnose(line); }); // 실제 Session과 같은 Factory/필드 쓰기 바인더를 원문 기대값에 재생한다.
        return hooks;
    }
    // fixture50BYTE만 읽고 TYPE/VT/프레임/QA 밖 바이트까지 그대로 대조한다.
    bool Same(std::string_view hex) { if (hex.size()!=100) return false;const auto raw=Raw();for (std::size_t index=0;index<raw.size();++index) if (raw[index]!=std::stoul(std::string(hex.substr(index*2,2)),nullptr,16)) return false;return true; } // 각 두 hex 문자로 한 BYTE를 비교한다.
};
}
// 반환하지 않는 원문 void 함수는 전체 바이트와 진단 순서/풀 카운터로 비교한다.
TEST_CASE(SquidRetype_ReplaysNative128AndPreservesOtherSlots) {
    const auto fixture=LoadFixture(NETSTORM_SQUID_RETYPE_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { // Editor 바깥 호출도 원문의 진단 복귀 의미를 유지한다.
        if (row[0]=="old") continue;CHECK(row.size()==6);ConversionScene scene;scene.Prepare(Number(row[0]),Number(row[1]),Number(row[2]),Number(row[3]));
        auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());const auto count=scene.pool.FreeCount();scene.factory->Retype(kConversionSid,Number(row[1]),[&](auto line) { scene.Diagnose(line); });
        CHECK(scene.Same(row[4]));CHECK(scene.trace==row[5]);CHECK(scene.pool.FreeCount()==count);
        std::copy(scene.Raw().begin(),scene.Raw().end(),before.begin()+kConversionSid.value*50U);CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));++checked;
    }
    CHECK(checked==128);
}
// 상위 플래그/부모 반환과 Unpop 뒤 TYPE/QA 재조회를 실제 두 몸체의 독립 출력에 맞춘다.
TEST_CASE(ObjectConversion_ReplaysNative36CommandsAndLateReads) {
    const auto fixture=LoadFixture(NETSTORM_OBJECT_CONVERSION_FIXTURE);std::size_t checked=0;
    for (const auto& row:fixture.rows) { // 원문이 처리하는 release 플래그도 따로 제거하지 않는다.
        if (row[0]=="body") continue;CHECK(row.size()==7);const bool buried=row[0]=="Buried";ConversionScene scene;scene.Prepare(buried ? 112 : 114,114,Number(row[1]),0);scene.mutation=Number(row[3]);
        const auto body=ObjectConversionBody(Get(scene.Raw(),0));CHECK(body==(buried ? 0x462290U : 0x41e400U));RawObjectConversion conversion(scene.pool,scene.buried,scene.Hooks());
        CHECK(conversion.Command(kConversionSid,Number(row[2]),body)==Number(row[4]));CHECK(scene.Same(row[5]));CHECK(scene.trace==row[6]);++checked;
    }
    CHECK(checked==36);CHECK(ObjectConversionBody(0)==0);CHECK(ObjectConversionBody(0x509930)==0);
}
// 선택되지 않은 경계는 요구하지 않고 실제 실패 위치까지의 타입/공간 쓰기를 유지한다.
TEST_CASE(ObjectConversion_RejectsSelectedMissingEffectsAndKeepsWrittenPrefix) {
    ConversionScene scene;scene.Prepare(112,114,4,0);ObjectConversionHooks hooks;hooks.base=[](Sid,auto) { return 99U; };RawObjectConversion parent(scene.pool,scene.buried,hooks);
    CHECK(parent.Command(kConversionSid,0x03000015,0x462290)==99);CHECK(Throws([&] { parent.Command(kConversionSid,0x030000ce,0); }));CHECK(Throws([&] { parent.Command(kConversionSid,0x030000ce,0x462290); }));
    scene.types[116].constructorAddress=0xffffffffU;scene.factory=std::make_unique<SquidFactory>(scene.pool,scene.types);
    CHECK(Throws([&] { scene.factory->Retype(kConversionSid,116); }));CHECK(scene.Raw()[10]==116);CHECK(Get(scene.Raw(),0)==0x50a748U);
    scene.Prepare(112,114,0,0);CHECK(Throws([&] { scene.factory->Retype(kConversionSid,114); }));CHECK(scene.Raw()[10]==112);
}

// 실제 Session용 바인더로 두 가상 전환의 전체 슬롯/공간 순서를 기존 독립 원문36행에 맞춘다.
TEST_CASE(ObjectConversion_WorldBindingReplaysNative36Commands) {
    const auto fixture=LoadFixture(NETSTORM_OBJECT_CONVERSION_FIXTURE);std::size_t checked{};
    for (const auto& row:fixture.rows) { // 상위 비트/부모 위임과 Unpop 뒤 QA/TYPE 변화도 같은 실제 경계를 지난다.
        if (row[0]=="body") continue;CHECK(row.size()==7);const bool buried=row[0]=="Buried";ConversionScene scene;scene.Prepare(buried ? 112 : 114,114,Number(row[1]),0);scene.mutation=Number(row[3]);
        RawObjectConversion conversion(scene.pool,scene.buried,scene.Hooks(true));CHECK(conversion.Command(kConversionSid,Number(row[2]),ObjectConversionBody(Get(scene.Raw(),0)))==Number(row[4]));CHECK(scene.Same(row[5]) && scene.trace==row[6]);++checked;
    }
    CHECK(checked==36);
}

// 진단에서 복귀하는 실제 Factory 바인딩도 원문128행의 전체 타입/생성자 결과를 보존한다.
TEST_CASE(ObjectConversion_WorldBindingRetypeDiagnostics128) {
    const auto fixture=LoadFixture(NETSTORM_SQUID_RETYPE_FIXTURE);std::size_t checked{};
    for (const auto& row:fixture.rows) { // void/발자국 진단 조합과 원문 생성자 없는 fallback까지 비교한다.
        if (row[0]=="old") continue;CHECK(row.size()==6);ConversionScene scene;scene.Prepare(Number(row[0]),Number(row[1]),Number(row[2]),Number(row[3]));const auto free=scene.pool.FreeCount();auto hooks=scene.Hooks(true);hooks.retype(kConversionSid,Number(row[1]));
        CHECK(scene.Same(row[4]) && scene.trace==row[5]);CHECK(scene.pool.FreeCount()==free);++checked;
    }
    CHECK(checked==128);
}

// 원문 진단 장치는 선택된 위치에서만 요구하고 같은 풀·실제 DWORD/WORD 쓰기 폭을 보존한다.
TEST_CASE(ObjectConversion_WorldBindingDiagnosticReturnAndPhysicalFields) {
    ConversionScene scene;scene.Prepare(112,114,4,0);SidPool foreign(OriginalEdition::Patch1078,24000);SquidFactory other(foreign,scene.types);
    CHECK(Throws([&] { MakeObjectConversionHooks(scene.pool,other,{},{}); }));auto healthy=MakeObjectConversionHooks(scene.pool,*scene.factory,{},{});healthy.retype(kConversionSid,114);CHECK(scene.Raw()[10]==114);
    scene.Prepare(112,114,0,3);const auto before=std::vector<std::uint8_t>(scene.Raw().begin(),scene.Raw().end());auto missing=MakeObjectConversionHooks(scene.pool,*scene.factory,{},{});CHECK(Throws([&] { missing.retype(kConversionSid,114); }));CHECK(std::equal(before.begin(),before.end(),scene.Raw().begin()));
    std::vector<std::uint32_t> lines;auto hooks=MakeObjectConversionHooks(scene.pool,*scene.factory,{},[&](std::string_view expression,std::string_view file,std::uint32_t line) {
        CHECK(file=="\\Ns\\O\\Squid.cpp");
        if (line==0x22d) CHECK(expression=="isVoid()");
        else if (line==0x22e) CHECK(expression=="::getType(typeNum)->footX == ::getType(newType)->footX");
        else if (line==0x22f) CHECK(expression=="::getType(typeNum)->footY == ::getType(newType)->footY");
        else CHECK(false);
        lines.push_back(line); // 같은 실제 Client처럼 보고 후 반환하여 뒤 진단/생성자를 계속한다.
    });
    hooks.retype(kConversionSid,114);CHECK((lines==std::vector<std::uint32_t>{0x22d,0x22e,0x22f}));CHECK(scene.Raw()[10]==114);
    auto expected=std::vector<std::uint8_t>(scene.Raw().begin(),scene.Raw().end());Put(expected,36,0x89abcdefU);Put(expected,12,0x12345678U,2);hooks.frame(kConversionSid,0x89abcdefU);hooks.qa(kConversionSid,0x12345678U);CHECK(std::equal(expected.begin(),expected.end(),scene.Raw().begin()));
}
