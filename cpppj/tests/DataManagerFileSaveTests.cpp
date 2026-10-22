// 실제 PE 저장 부모의144입력과 현재 포인터/이름/길이/관문 재조회 순서를 대조한다.
#include "o/RawDataManagerFileSave.h"
#include "RawSceneSupport.h"
#include <algorithm>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
constexpr std::uint32_t kSaveVtable=0x1100100,kImage=0x1101000,kAlternate=0x1102000; //독립 관측에만 사용한 불투명 주소 토큰이다.
// 실제 가상 getter가 반환하는 manager+12의 현재 원문이다.
std::string_view Name(std::span<const std::uint8_t> manager) {
    const auto bytes=manager.subspan(12);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("저장 검사 파일명의 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 원본 strcpy와 같이 새 이름/NUL만 쓰고 남은 manager 바이트는 보존한다.
void Name(std::span<std::uint8_t> manager,std::string_view text) {
    std::copy(text.begin(),text.end(),manager.begin()+12);manager[12+text.size()]=0;
}
// 호스트 예외 경계를 값0의 정상 반환으로 바꾸지 않는다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 입력만 재생하여 실제 전체 manager/MarkStream/전역/파일 장치 순서를 원본 결과와 비교한다.
TEST_CASE(DataManagerFileSave_Native144) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_FILE_SAVE_FIXTURE).rows;CHECK(rows.size()==144);
    for (const auto& row:rows) { //각 입력은 원본과 같은 초기 전역과 manager에서 독립 실행한다.
        CHECK(row.size()==7);if (row.size()!=7) return;const auto input=Split(row[0],',');CHECK(input.size()==4);if (input.size()!=4) return;
        std::uint32_t readOnly=Number(input[0]),owner=Number(input[1]);const auto hidden=Number(input[2]),variant=Number(input[3]);std::array<std::uint8_t,9> hiddenPlayers{};hiddenPlayers.at(owner)=static_cast<std::uint8_t>(hidden);
        std::array<std::uint8_t,64> manager;manager.fill(0xa7);Put(manager,0,kSaveVtable);Name(manager,"unit.fort");std::array<std::uint8_t,20> snapshot{};std::span<std::uint8_t> active;std::size_t names{};std::string trace;
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; //외부 경계 요청만 원본과 같은 순서로 기록한다.
        const auto capture=[&] { std::copy(active.begin(),active.end(),snapshot.begin()); }; //로컬 MarkStream이 살아 있는 각 경계에서 현재 원문을 보관한다.
        DataManagerFileSaveHooks hooks;
        hooks.image=[&](auto source,std::span<std::uint8_t,20> stream) {
            CHECK(source.data()==manager.data());active=stream;event("S:"+Hex(stream));Put(stream,0,variant==1 ? 0U : kImage);Put(stream,4,0);Put(stream,8,variant==7 ? 0xffffffffU : 17U);Put(stream,12,0x8000);Put(stream,16,0x12345678);
            if (variant==2) readOnly=!readOnly;
            if (variant==3) { owner=(owner+1)%9;hiddenPlayers[owner]=static_cast<std::uint8_t>(!hidden); }
            capture();
        }; //이미지 생성은 저장 억제 관문보다 먼저 실제 현재 전역을 바꿀 수 있다.
        hooks.fileName=[&](auto source) {
            ++names;event("N:"+std::string(Name(source)));if (variant==6 && names==2) { Put(active,0,kAlternate);Put(active,8,55); }capture();return Name(source);
        }; //둘째 이름 조회가 이미지 길이를 바꿔도 이미 보관한 쓰기 인자는 바뀌지 않는다.
        hooks.permissions=[&](auto name,auto mode) {
            event("C:"+std::string(name)+':'+std::to_string(mode));
            if (variant==4) { Name(manager,"changed.fort");readOnly=1;owner=4;hiddenPlayers.fill(0xff); }
            if (variant==5) { Put(active,0,kAlternate);Put(active,8,33); }capture();
        }; //권한 효과 후 이름/길이만 다시 읽고 저장/숨김 관문과 이미지 포인터는 재조회하지 않는다.
        hooks.write=[&](auto name,auto pointer,auto length) { event("W:"+std::string(name)+':'+std::to_string(pointer)+':'+std::to_string(length));capture(); };
        hooks.release=[&](auto pointer) { event("F:"+std::to_string(pointer));capture(); };
        SaveDataManagerFile(manager,{readOnly,owner,hiddenPlayers},hooks);
        CHECK(Hex(manager)==row[1] && Hex(snapshot)==row[2] && readOnly==Number(row[3]) && owner==Number(row[4]) && Hex(hiddenPlayers)==row[5] && trace==row[6]);
    }
}

// 억제 경로는 이미지 생성/반납을 생략하지 않으며 호스트 실패에서도 중복 반납하지 않는다.
TEST_CASE(DataManagerFileSave_SelectedBoundariesAndExceptions) {
    std::array<std::uint8_t,64> manager{};std::array<std::uint8_t,9> hidden{};std::uint32_t readOnly=1,owner=999,releases{};DataManagerFileSaveHooks hooks;
    CHECK(Rejects([&] { SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks); }));
    hooks.image=[](auto,std::span<std::uint8_t,20> stream) { Put(stream,0,kImage); };
    hooks.release=[&](auto pointer) { CHECK(pointer==kImage);++releases; };SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks);CHECK(releases==1);
    readOnly=0;CHECK(Rejects([&] { SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks); }) && releases==2);owner=1;
    hooks.fileName=[](auto) { return std::string_view("working.fort"); };hooks.permissions=[](auto,auto mode) { CHECK(mode==0x180); };
    hooks.write=[](auto,auto,auto) { throw std::runtime_error("호스트 파일 쓰기 실패 입력"); };CHECK(Rejects([&] { SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks); }) && releases==3);
    hooks.image=[](auto,std::span<std::uint8_t,20> stream) { Put(stream,0,kImage);throw std::runtime_error("호스트 이미지 생성 중 실패 입력"); };CHECK(Rejects([&] { SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks); }) && releases==4);
    hooks.release=[&](auto) { ++releases;throw std::runtime_error("호스트 반납 실패 입력"); };CHECK(Rejects([&] { SaveDataManagerFile(manager,{readOnly,owner,hidden},hooks); }) && releases==5);
}
