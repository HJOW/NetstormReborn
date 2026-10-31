// CD 루트/캐시의 독립 PE 전체 BYTE와 같은 실제 Client/Windows 파일 장치를 확인한다.
#include "RawSceneSupport.h"
#include "DudeSpawnSupport.h"
#include "client/CdPresence.h"
#include "client/CdDirectory.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "o/RawBaseFileReadPolicy.h"
#include <windows.h>

namespace { // 장치 입력만 구성하며 기대 분기는 독립 Native 결과에서 읽는다.
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
std::vector<char> CdBytes(std::string_view text) { // 입력의 NUL/첫 NUL 뒤 물리도 그대로 복원한다.
    std::vector<char> result;if (text=="-") return result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return result; // 두16진수마다 한BYTE다.
}
std::string CdHex(std::span<const char> bytes) { return Hex({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); } // BYTE의 signed 여부와 무관하게 전체 물리를 기록한다.
std::string CdTextHex(std::string_view text) { return CdHex({text.data(),text.size()}); } // 실제 경계에 전달한 C문자열만 기록한다.
// Native와 같은 세128BYTE 물리 및 OS 입력/명시 콜백 변이를 보관한다.
struct PresenceScene {
    std::array<char,128> cache{},output{},logical{}; // 현재 전역 캐시/호출자 출력/OS 목록의 실제 별개 물리다.
    std::vector<char> drives;std::vector<std::uint32_t> types; // 명시 MULTI_SZ와 같은 순서의 종류 결과다.
    std::uint32_t logicalResult{},capacity{},mask{},mutation{}; // 원문에 제공한 OS 반환/요청 용량/존재/변이 입력이다.
    std::size_t typeIndex{},probes{};bool outputUsed{},logicalUsed{}; // 실제 호출/물리 사용만 관측하며 결과를 대신하지 않는다.
    char* pathRoot{};std::string events; // 경로가 읽은 현재 source와 실제 호출 순서다.
    explicit PresenceScene(const std::vector<std::string>& row) { // 최종 BYTE는 읽지 않고 최초8입력만 설치한다.
        cache.fill(static_cast<char>(205));output.fill(static_cast<char>(205));logical.fill(static_cast<char>(205));const auto cached=CdBytes(row[1]);std::copy(cached.begin(),cached.end(),cache.begin());cache[cached.size()]=0;drives=CdBytes(row[2]);
        if (row[3]!="-") for (const auto& type:Split(row[3],',')) types.push_back(Number(type)); // 실제 열거가 선택한 순서로 결과를 제공한다.
        logicalResult=Number(row[4]);capacity=Number(row[5]);mask=Number(row[6]);mutation=Number(row[7]);outputUsed=row[0]=="roots";
    }
    void Event(std::string text) { if (!events.empty()) events+=';';events+=text; } // 현재 호출의 인자를 순서대로 보관한다.
    CdDriveRootsHooks RootHooks() { // OS 목록은 앞부분만 작성하며 뒤 물리는205를 보존한다.
        CdDriveRootsHooks hooks;hooks.logicalDrives=[this](auto buffer) { logicalUsed=true;Event("L:128");std::copy(drives.begin(),drives.end(),buffer.begin());return logicalResult; };
        hooks.driveType=[this](auto root) { Event("T:"+CdTextHex(root));const auto type=types.at(typeIndex++);if (mutation && typeIndex==1) const_cast<char*>(root.data())[0]='Z';return type; };return hooks;
    }
    CdPresenceHooks CheckHooks() { // 경로/존재의 source 변경은 같은 실제 캐시/출력 BYTE에 적용한다.
        CdPresenceHooks hooks;hooks.path=[this](auto root,auto marker) { CHECK(marker==kCdPresenceMarker);Event("P:"+CdTextHex(root));pathRoot=const_cast<char*>(root.data());return netstorm::o::JoinBaseFileRoot(root,marker); };
        hooks.exists=[this](auto path) { Event("E:"+CdTextHex(path));const auto result=(mask>>probes++)&1U;if (mutation && probes==1) pathRoot[0]='Z';return result!=0; };
        hooks.roots=[this](auto buffer) { outputUsed=true;const auto result=CollectCdDriveRoots(128,buffer,logical,RootHooks());return result; };return hooks;
    }
};
}

// 용량의 엄격 비교·초과·OS 실패·캐시 우회·콜백 뒤 source 재조회와 실제 stack 불변을 원문과 대조한다.
TEST_CASE(CdPresence_IndependentNative492FullBuffersAndCalls) {
    const auto rows=LoadFixture(NETSTORM_CD_PRESENCE_FIXTURE).rows;CHECK(rows.size()==492);std::size_t index{};
    for (const auto& row:rows) { // 기대 세 버퍼와 반환값은 제품 실행이 끝난 뒤에만 비교한다.
        PresenceScene scene(row);const auto result=row[0]=="roots" ? CollectCdDriveRoots(scene.capacity,scene.output,scene.logical,scene.RootHooks()) : CheckCdPresence(scene.cache,scene.output,scene.CheckHooks());const auto before=netstorm::test::FailureCount();
        CHECK(result==Number(row[8]));CHECK(CdHex(scene.cache)==row[9]);CHECK((scene.outputUsed?CdHex(scene.output):"-")==row[10]);CHECK((scene.logicalUsed?CdHex(scene.logical):"-")==row[11]);CHECK((scene.events.empty()?"-":scene.events)==row[12]);
        if (netstorm::test::FailureCount()!=before) std::fprintf(stderr,"CD presence Native row=%zu op=%s\n",index,row[0].c_str());++index;
    }
}

// 비영 캐시 실패의 첫BYTE 정리 뒤 장치 누락을 숨기지 않으며 미선택 OS 장치는 요구하지 않는다.
TEST_CASE(CdPresence_MissingBoundariesKeepEarlierEffects) {
    std::array<char,128> cache{},scratch{};cache.fill('x');cache[0]='Q';cache[1]=':';cache[2]='\\';cache[3]=0;CdPresenceHooks hooks;hooks.path=[](auto,auto) { return "marker"; };hooks.exists=[](auto) { return false; };
    bool threw{};try { static_cast<void>(CheckCdPresence(cache,scratch,hooks)); }catch (const std::logic_error&) { threw=true; }CHECK(threw && cache[0]==0 && cache[1]==':' && cache[4]=='x');
    std::size_t asserts{};CdDriveRootsHooks roots;roots.assertion=[&](auto line) { CHECK(line==0x529);++asserts; };roots.logicalDrives=[](auto) { return 0U; };CHECK(CollectCdDriveRoots(0,{},scratch,roots)==0 && asserts==1);
    hooks.roots=[](auto) { return 0U; };hooks.path={};hooks.exists={};CHECK(CheckCdPresence(cache,scratch,hooks)==0);
}

// 같은 Client 캐시와 별도 CD DWORD를 사용하고 실제 Unicode W 속성/스레드 오류 모드 복구도 확인한다.
TEST_CASE(CdPresence_ActualClientOwnerAndUnicodeAttributeDevice) {
    const auto& assets=netstorm::test::dudespawn::Assets(netstorm::o::OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;Client client(std::move(options));const auto owner=client.CdGlobals();CHECK(std::all_of(owner->CachedRoot().begin(),owner->CachedRoot().end(),[](auto byte) { return byte==0; }));
    client.RandomGlobals()->CrtState()=0xfedcba98U;client.LocalStateGlobals()->CdPresent()=7;CHECK(&client.LocalStateGlobals()->CdPresent()!=&client.RandomGlobals()->CrtState());
    const auto directory=assets.root.parent_path()/"extracted/cpp-cd-presence-654-disk"/std::filesystem::path(u8"한글-😀");std::filesystem::create_directories(directory/std::string(kCdPresenceMarker));const auto bytes=directory.u8string();CHECK(bytes.size()+1<=owner->CachedRoot().size());if (bytes.size()+1>owner->CachedRoot().size()) return;
    auto cached=owner->CachedRoot();std::fill(cached.begin(),cached.end(),char{});std::copy(bytes.begin(),bytes.end(),cached.begin());auto hooks=MakeWindowsCdPresenceHooks();hooks.roots=[](auto)->std::uint32_t { throw std::logic_error("캐시 성공에서 실제 CD 드라이브를 열거하면 안 됩니다"); };const auto mode=GetThreadErrorMode();
    client.LocalStateGlobals()->CdPresent()=owner->Check(hooks);CHECK(client.LocalStateGlobals()->CdPresent()==1 && GetThreadErrorMode()==mode && client.RandomGlobals()->CrtState()==0xfedcba98U);CHECK(client.CdGlobals().get()==owner.get() && owner->Check(hooks)==1); // 표시 파일이 디렉터리여도 원문의 속성 판단은 true다.
}
