// NewFort의 세 실제 x86 부모와 같은 외부 입력을 재생한다. 제품 등록·빌드는 다음 통합 단위가 맡는다.
#include "RawSceneSupport.h"
#include "o/RawNewFort.h"
#include "o/RawTemplateLoad.h"
#include <array>
#include <bit>
#include <map>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // Native에서 명시한 파일/할당/등록/난수/저장 장치만 재생하는 검사 장면이다.
constexpr std::array<std::uint32_t,2> kNewFortAllocations{0x10006000,0x10006200}; // Native의 불투명 allocation 입력이며 호스트 포인터가 아니다.
constexpr std::string_view kNewFortPath="d/NewFort.fort"; // 모든 manager/missing 관측에 사용한 명시 파일 이름이다.

// 장치에 전달한 NUL 이전 문자열을 관측 fixture의 BYTE 표기로 바꾼다.
std::string NewFortTextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }

// 실제 입력 버퍼의 첫 NUL까지 읽으며 임의 공백 정리나 문자 변환을 하지 않는다.
std::string_view NewFortText(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("NewFort 검사 입력 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}

// 입력 주소/전역을 같은 C++ 저장소로 대응시킨다. 이 장면은 실제 게임 owner를 대체하는 제품 장치가 아니다.
struct NewFortReplay {
    std::uint32_t initialEditor{1},editor{1},known{1},owner{1},localOwner{1}; // 각 호출이 다시 읽는 Native 전역과 난수 변이의 초기 Editor 입력이다.
    std::uint32_t subscriber{123},coreActive{0x12345678},seed{0xa55a6789},alternate{0xabcdef01}; // 같은 DWORD를 참조하여 부분효과를 남긴다.
    std::uint32_t mutation{},spBits{},random254{},randomIndex{}; // 외부 장치가 제공하는 명시 입력이며 부모 결과로 계산하지 않는다.
    bool otherSelected{}; // PTR00540ca8의 두 입력 버퍼 중 현재 포인터를 나타낸다.
    std::array<std::uint8_t,256> handle{},other{}; // 원문이 실제로 복사하는 두 NUL 버퍼와 A5 주변 영역이다.
    std::array<std::uint8_t,16> core{}; // 10BYTE clear 뒤 나머지6BYTE가 보존되는지 함께 읽는다.
    std::array<std::uint8_t,288> states{}; // 9개 owner의 State32BYTE를 모두 읽어 잘못된 owner 쓰기를 검출한다.
    std::array<std::uint8_t,120> territory{}; // writer로 실제 전달한20개6BYTE 레코드의 전체 복사다.
    std::vector<std::uint32_t> randoms; // zero 재시도와 최종 비영값을 포함한 Native 난수 입력 순서다.
    std::map<std::string,std::uint32_t> counts; // 변이 장치가 지정한 호출 횟수이며 제품 부모가 사용하지 않는다.
    std::string trace; // 실제 하위 호출 순서와 캡처한 인자를 한 줄로 기록한다.

    // Native reset과 같은 전체 A5 입력과 두 문자열을 준비한다.
    NewFortReplay() {
        handle.fill(0xa5);other.fill(0xa5);core.fill(0xa5);states.fill(0xa5);
        const std::string before="Before",second="Other";std::copy(before.begin(),before.end(),handle.begin());handle[before.size()]=0;std::copy(second.begin(),second.end(),other.begin());other[second.size()]=0;
    }

    // 현재 handle 포인터의 같은 전체 저장소를 빌린다.
    std::span<std::uint8_t> Handle() { return otherSelected ? std::span(other) : std::span(handle); }

    // 호출 직전 인자를 기록한 다음 Native에서 명시한 외부 재진입 효과만 적용한다.
    void Event(std::string name) {
        if (!trace.empty()) trace+=';';trace+=name;const auto key=name.substr(0,name.find(':'));const auto count=++counts[key];
        if (mutation==1 && key=="translate") otherSelected=true;
        if (mutation==2 && key=="title" && count==1) editor=1-editor;
        if (mutation==3 && key=="sp") owner=2;
        if (mutation==4 && key=="core") owner=3;
        if (mutation==5 && key=="prepare" && count==1) seed=0xbbbb0000U|(seed&0xffffU);
        if (mutation==6 && key=="next" && name.starts_with("next:65536") && count==1U+(initialEditor==0)) seed=0x77770000U|(seed&0xffffU);
        if (mutation==7 && key=="subscriber") owner=4;
        if (mutation==8 && key=="money") editor=1;
        if (mutation==9 && key=="state") { owner=5;editor=1-editor; }
        if (mutation==10 && key=="config" && count==2) otherSelected=true;
        if (mutation==11 && key=="register") localOwner=3;
        if (mutation==12 && key=="coreWrite") seed=0xdeadffff;
    }

    // 제품 부모가 각 위치에서 다시 읽을 전역을 같은 장면에 연결한다.
    NewFortDataGlobals Globals() {
        return {subscriber,[this] { return Handle(); },[this] { return editor; },[this] { return known; },[this] { return localOwner; },[this] { return owner; },coreActive,seed};
    }

    // Native와 같은 큰 외부 하위 경계를 재생한다. Core10BYTE clear는 제품 부모의 실제 본문을 실행한다.
    NewFortDataHooks DataHooks() {
        NewFortDataHooks hooks;
        hooks.translate=[this](auto name)->std::string_view { CHECK(name=="Unnamed" && subscriber==0xffffffffU);Event("translate");return "Unnamed"; };
        hooks.configure=[this](auto key,auto value) { Event("config:"+std::string(key)+':'+NewFortTextHex(value)); };
        hooks.title=[this] { Event("title"); };hooks.registerSubscriber=[this] { Event("register");subscriber=0xfedcba98; };
        hooks.sp=[this](auto selected) { Event("sp:"+std::to_string(selected));return std::bit_cast<float>(spBits); };
        hooks.addSp=[this](auto amount) { Event("addSp:"+std::to_string(std::bit_cast<std::uint32_t>(amount))+':'+std::to_string(localOwner)); };
        hooks.coreData=[this] { Event("core");return std::span(core); };hooks.coreWrite=[this](auto selected) { Event("coreWrite:"+std::to_string(selected)); };
        hooks.prepareRandom=[this] { Event("prepare"); };
        hooks.nextRandom=[this](auto bound) { Event("next:"+std::to_string(bound));return bound==254 ? random254 : randoms.at(randomIndex++); };
        hooks.subscriber=[this](auto selected,auto id,auto value) { Event("subscriber:"+std::to_string(selected)+':'+std::to_string(id)+':'+NewFortTextHex(NewFortText(value))); };
        hooks.territory=[this](auto selected,auto records) { std::copy(records.begin(),records.end(),territory.begin());Event("territory:"+std::to_string(selected)+':'+Hex(territory)); };
        hooks.money=[this](auto selected) { Event("money:"+std::to_string(selected)); };
        hooks.state=[this](auto selected) { const auto result=std::span(states).subspan(selected*32,32);Event("state:"+std::to_string(selected));return result; };
        hooks.notify=[this](auto selected) { Event("notify:"+std::to_string(selected)); };return hooks;
    }

    // missing 부모에서는 두 큰 부모를 별도 fixture가 보장하는 명시 하위로 분리한다.
    NewFortMissingFileHooks MissingHooks() {
        NewFortMissingFileHooks hooks;hooks.path=[this] { Event("path");return kNewFortPath; };hooks.exists=[this](auto name) { Event("exists:"+NewFortTextHex(name));return 0; };hooks.owner=[this] { return owner; };
        hooks.manager=[this](auto selected,auto name,auto argument) { Event("manager:"+std::to_string(selected)+':'+NewFortTextHex(name)+':'+std::to_string(argument)); };
        hooks.state=[this](auto selected) { const auto result=std::span(states).subspan(selected*32,32);Event("state:"+std::to_string(selected));return result; };
        hooks.notify=[this](auto selected) { Event("notify:"+std::to_string(selected)); };
        hooks.data=[this] { CHECK(alternate==0xabcdef01);Event("data"); };
        hooks.subscriber=[this](auto selected) { CHECK(alternate==1);Event("subscriberLoad:"+std::to_string(selected)+":5508212:"+NewFortTextHex(NewFortText(Handle()))); };
        hooks.handle=[this] { return NewFortText(Handle()); };hooks.configure=[this](auto key,auto value) { Event("config:"+std::string(key)+':'+NewFortTextHex(value)); };hooks.title=[this] { Event("title"); };return hooks;
    }
};
}

// manager部分쓰기·존재삭제·두할당·NULL의 실제중단위치를48개 독립 관측에 대조한다.
TEST_CASE(NewFort_ManagerNative48) {
    const auto rows=LoadFixture(NETSTORM_NEW_FORT_MANAGER_FIXTURE).rows;CHECK(rows.size()==48);
    // 각 행은 기존ptr/exists/argument와 두 할당의 성공비트를 별도로 제공한다.
    for (const auto& row:rows) {
        CHECK(row.size()==10);if (row.size()!=10) return;std::array<std::uint8_t,20> header;header.fill(0xa5);Put(header,0,Number(row[0]));std::array<std::uint8_t,4> version;Put(version,0,0xdeadbeef);std::array<std::array<std::uint8_t,276>,2> managers;
        // 두 manager의 미사용 영역도 변경 없이 전부 대조한다.
        for (auto& manager:managers) manager.fill(0xa5);
        std::size_t allocationIndex{};std::string trace;
        const auto record=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 경계에 도달한 순서만 기록한다.
        const auto manager=[&](std::uint32_t token)->std::span<std::uint8_t> { const auto found=std::find(kNewFortAllocations.begin(),kNewFortAllocations.end(),token);if (found==kNewFortAllocations.end()) throw std::out_of_range("NewFort 검사 manager token");return managers[static_cast<std::size_t>(found-kNewFortAllocations.begin())]; }; // 불투명 입력을 실제 검사 배열에 연결한다.
        NewFortManagerHooks hooks;hooks.assertion=[&](auto line) { record("assert:"+std::to_string(line)); };hooks.exists=[&](auto name) { record("exists:"+NewFortTextHex(name));return static_cast<std::int32_t>(Number(row[1])); };
        hooks.allocate=[&](auto size) { record("allocate:"+std::to_string(size));const auto selected=allocationIndex++;return row[3].at(selected)=='1' ? kNewFortAllocations.at(selected) : 0U; };
        hooks.construct=[&](auto token,auto name) { record("construct:"+std::to_string(token)+':'+NewFortTextHex(name));ConstructTemplateDataManager(manager(token),name,version);return token; };
        hooks.fileName=[&](auto token) { record("fileName:"+std::to_string(token));return TemplateDataManagerFileName(manager(token)); };hooks.erase=[&](auto name) { record("erase:"+NewFortTextHex(name)); };hooks.destroy=[&](auto token) { record("destroy:"+std::to_string(token)); };hooks.release=[&](auto token) { record("release:"+std::to_string(token)); };
        hooks.initialize=[&](auto token) { CHECK(Get(header,0)==token);record("initialize:"+std::to_string(token));if (!token) throw std::out_of_range("원문0044d875 NULL read"); };
        bool completed=true;try { CreateNewFortManager(header,kNewFortPath,Number(row[2]),hooks); } catch (const std::out_of_range&) { completed=false; }
        CHECK(completed==(Number(row[4])!=0));CHECK(row[5]==(completed ? "0x0" : trace.ends_with("initialize:0") ? "0x44d875" : "0x4bfbff"));CHECK(Hex(header)==row[6]);CHECK(Get(version,0)==Number(row[7]));CHECK(Hex(managers[0])+','+Hex(managers[1])==row[8]);CHECK(trace==row[9]);
    }
}

// Editor/known/SP비트/seed·zero재시도/외부재진입13종의1248행 전체상태와 호출순서를 대조한다.
TEST_CASE(NewFort_DataNative1248) {
    const auto rows=LoadFixture(NETSTORM_NEW_FORT_DATA_FIXTURE).rows;CHECK(rows.size()==1248);
    // 기대값은 fixture만 사용하며 제품 부모의 판단을 검사에서 다시 계산하지 않는다.
    for (const auto& row:rows) {
        CHECK(row.size()==16);if (row.size()!=16) return;NewFortReplay scene;scene.initialEditor=scene.editor=Number(row[0]);scene.known=Number(row[1]);scene.spBits=Number(row[2]);scene.seed=Number(row[3]);scene.random254=Number(row[4]);scene.mutation=Number(row[6]);
        // Native의 명시 난수 시퀀스를 소모하고 남은 개수도 확인한다.
        for (const auto& random:Split(row[5],',')) scene.randoms.push_back(Number(random));
        CreateNewFortData(scene.Globals(),scene.DataHooks());CHECK(scene.randomIndex==scene.randoms.size());CHECK(scene.subscriber==Number(row[7]));CHECK(Hex(std::span(scene.handle).first(16))==row[8]);CHECK(Hex(std::span(scene.other).first(16))==row[9]);CHECK(scene.coreActive==Number(row[10]));CHECK(Hex(scene.core)==row[11]);CHECK(scene.seed==Number(row[12]));CHECK(Hex(scene.states)==row[13]);CHECK(Hex(scene.territory)==row[14]);CHECK(scene.trace==row[15]);
    }
}

// 파일부재 부모의 두번path조회·State캡처·Notify재조회·alternate확정순서를26행에 대조한다.
TEST_CASE(NewFort_MissingNative26) {
    const auto rows=LoadFixture(NETSTORM_NEW_FORT_MISSING_FIXTURE).rows;CHECK(rows.size()==26);
    // 두 owner와 동일 외부변이표를 사용하되 실제 선택된 경계만 효과를 받는다.
    for (const auto& row:rows) {
        CHECK(row.size()==5);if (row.size()!=5) return;NewFortReplay scene;scene.owner=Number(row[0]);scene.mutation=Number(row[1]);CHECK(InstallMissingNewFort(scene.alternate,scene.MissingHooks()));CHECK(scene.alternate==Number(row[2]));CHECK(Hex(scene.states)==row[3]);CHECK(scene.trace==row[4]);
    }
}

// A5의bit0은이미1이므로 별도0/A4/FE입력78행에서 실제 선택State만bit0이켜지는지 보강한다.
TEST_CASE(NewFort_MissingStateBitNative78) {
    const auto rows=LoadFixture(NETSTORM_NEW_FORT_MISSING_BITS_FIXTURE).rows;CHECK(rows.size()==78);
    // bit0이0인 세 전체패턴과 owner재조회효과를 같은 부모에 전달한다.
    for (const auto& row:rows) {
        CHECK(row.size()==6);if (row.size()!=6) return;NewFortReplay scene;scene.owner=Number(row[0]);scene.mutation=Number(row[1]);scene.states.fill(static_cast<std::uint8_t>(Number(row[2])));CHECK(InstallMissingNewFort(scene.alternate,scene.MissingHooks()));CHECK(scene.alternate==Number(row[3]));CHECK(Hex(scene.states)==row[4]);CHECK(scene.trace==row[5]);
    }
}
