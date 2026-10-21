// 원본 생성자/Load의 바이트 보존과 효과 순서를 검사하도록 작성했다. 사용자 중단에 따라 실행하지 않는다.
#include "o/RawTemplateLoad.h"
#include "RawSceneSupport.h"
#include <algorithm>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 생성자가 쓰지 않는 이름 뒤 공간과 양쪽 경계는 별도 입력 값 그대로 남아야 한다.
TEST_CASE(TemplateLoad_ConstructorAndBorrowedFileName) {
    std::array<std::uint8_t,284> bytes;bytes.fill(0xa7);auto manager=std::span(bytes).subspan(4,276);std::array<std::uint8_t,4> global{1,2,3,4}; // 미정 tail을0으로 가정하지 않는 명시 입력이다.
    ConstructTemplateDataManager(manager,"MiXeD.fort",global);CHECK(Get(manager,0)==0x514690 && !Get(manager,4) && !Get(manager,8) && !Get(manager,0x10c) && Get(manager,0x110)==0xffffffffU);
    CHECK(std::all_of(global.begin(),global.end(),[](auto value) { return value==0; }));CHECK(TemplateDataManagerFileName(manager)=="MiXeD.fort" && manager[22]==0 && manager[23]==0xa7 && manager[267]==0xa7);
    CHECK(Get(bytes,0)==0xa7a7a7a7U && Get(bytes,280)==0xa7a7a7a7U);const auto view=TemplateDataManagerFileName(manager);manager[12]='m';CHECK(view=="miXeD.fort");
    ConstructTemplateDataManager(manager,std::string_view("x\0ignored",9),global);CHECK(TemplateDataManagerFileName(manager)=="x" && manager[14]=='X'); // 원본 C 문자열처럼 첫 NUL까지만 쓴다.
    const std::string maximum(255,'q');ConstructTemplateDataManager(manager,maximum,global);CHECK(TemplateDataManagerFileName(manager)==maximum && !manager[267]);
    CHECK(Throws([&] { ConstructTemplateDataManager(manager,std::string(256,'q'),global); }));CHECK(Throws([&] { TemplateDataManagerFileName(manager.first(275)); }));
    manager[267]='q';CHECK(Throws([&] { TemplateDataManagerFileName(manager); }));
}

// 이미 파일을 가진 헤더는 같음/다름 어느 경우에도 할당/읽기/해제를 하지 않는다.
TEST_CASE(TemplateLoad_ExistingFileKeepsAllHeaderFields) {
    std::array<std::uint8_t,20> header;header.fill(0xa7);Put(header,0,0x1234);const auto before=header;std::size_t names{},compares{},diagnostics{}; // 기존 manager와 stale 부가 필드의 명시 입력이다.
    TemplateLoadHooks hooks;hooks.fileName=[&](auto manager) { CHECK(manager==0x1234);++names;return std::string_view("Keep.fort"); };
    hooks.sameName=[&](auto first,auto second) { CHECK(first=="Keep.fort" && second=="KEEP.FORT");return ++compares==1; }; // CRT 비교의0/비영 결과만 외부 효과 입력으로 제공한다.
    hooks.wrongName=[&] { ++diagnostics; };LoadTemplateFile(header,"KEEP.FORT",hooks);CHECK(header==before && names==1 && compares==1 && !diagnostics);
    LoadTemplateFile(header,"KEEP.FORT",hooks);CHECK(header==before && names==2 && compares==2 && diagnostics==1);
}

// readonly 사전 설정은 파일 판정을 건너뛰되1로 정규화하고 성공 시 현재 슬롯만 제목을 갱신한다.
TEST_CASE(TemplateLoad_NewReadOnlyAndCurrentTitleBranches) {
    // 네 입력은 preset 비영/영과 판정 true/false 및 현재 슬롯 true/false를 독립해서 고른다.
    for (std::uint32_t variant=0;variant<4;++variant) {
        std::array<std::uint8_t,20> header{};Put(header,4,0xaabbccdd);Put(header,8,variant==3 ? 0x87654321U : 0U);Put(header,12,0x7391);Put(header,16,0x12345678);std::string trace;
        TemplateLoadHooks hooks;hooks.allocate=[&](auto size) { CHECK(size==276 && !Get(header,4) && !Get(header,0));trace+='A';return 0x101U; };
        hooks.construct=[&](auto manager,auto name) { CHECK(manager==0x101 && name=="new.fort" && !Get(header,0));trace+='C';return 0x202U; }; // 생성자 EAX가 헤더에 설치되는 것을 구별한다.
        hooks.fileName=[&](auto manager) { CHECK(manager==0x202 && Get(header,0)==0x202);trace+='N';return std::string_view("new.fort"); };
        hooks.readOnly=[&](auto name) { CHECK(name=="new.fort" && Get(header,16)==0x12345678);trace+='R';return variant==1; };
        hooks.readFile=[&](auto manager) { CHECK(manager==0x202 && !Get(header,16) && Get(header,12)==0x7391 && Get(header,8)==(variant==1 || variant==3 ? 1U : 0U));trace+='F';return true; };
        hooks.isCurrent=[&] { trace+='I';return variant!=2; };hooks.refreshTitle=[&] { trace+='T'; };
        LoadTemplateFile(header,"new.fort",hooks);CHECK(trace==(variant==3 ? "ACFIT" : variant==2 ? "ACNRFI" : "ACNRFIT"));CHECK(Get(header,0)==0x202 && !Get(header,4) && Get(header,12)==0x7391);
    }
}

// 실제 read/소멸 효과가 ptr를 바꾸어도 실패의 free는 소멸 직전 저장 포인터를 쓴다.
TEST_CASE(TemplateLoad_FailureRereadsPointerAndPreservesOtherFields) {
    std::array<std::uint8_t,20> header{};Put(header,8,4);Put(header,12,0x7391);std::string trace;TemplateLoadHooks hooks; // readonly preset은 미선택 이름/판정 경계를 요구하지 않는다.
    hooks.allocate=[](auto) { return 11U; };hooks.construct=[](auto pointer,auto) { return pointer; };
    hooks.readFile=[&](auto pointer) { CHECK(pointer==11);trace+='F';Put(header,0,22);Put(header,4,1);Put(header,8,7);Put(header,16,9);return false; };
    hooks.lifetime.destroy=[&](auto pointer) { CHECK(pointer==22);trace+='D';Put(header,0,33);Put(header,12,0x8273); };
    hooks.lifetime.release=[&](auto pointer) { CHECK(pointer==22 && Get(header,0)==33);trace+='R';Put(header,16,10); };
    LoadTemplateFile(header,"failed.fort",hooks);CHECK(trace=="FDR" && !Get(header,0) && Get(header,4)==1 && Get(header,8)==7 && Get(header,12)==0x8273 && Get(header,16)==10);
    hooks.readFile=[&](auto) { Put(header,0,0);return false; };trace.clear();LoadTemplateFile(header,"failed.fort",hooks);CHECK(trace.empty() && !Get(header,0)); // 실패 뒤 이미NULL이면 소멸/free를 선택하지 않는다.
}

// read 성공 뒤 ptr가NULL이면 current/title을 조회하지 않고 NULL 할당도 임의 성공으로 바꾸지 않는다.
TEST_CASE(TemplateLoad_NullAndUnselectedEffectBoundaries) {
    std::array<std::uint8_t,20> header{};Put(header,8,1);TemplateLoadHooks hooks;hooks.allocate=[](auto) { return 1U; };hooks.construct=[](auto pointer,auto) { return pointer; };
    hooks.readFile=[&](auto pointer) { CHECK(pointer==1);Put(header,0,0);return true; };LoadTemplateFile(header,"input.fort",hooks);CHECK(!Get(header,0));
    header.fill(0);Put(header,4,7);hooks.allocate=[](auto) { return 0U; };hooks.construct={};hooks.readOnly=[](auto) { return false; };hooks.fileName=[](auto pointer)->std::string_view { CHECK(!pointer);throw std::runtime_error("명시 NULL manager 가상 호출 거부"); };
    CHECK(Throws([&] { LoadTemplateFile(header,"input.fort",hooks); }) && !Get(header,0) && !Get(header,4));CHECK(Throws([&] { LoadTemplateFile(std::span(header).first(19),"input.fort",hooks); }));
}
