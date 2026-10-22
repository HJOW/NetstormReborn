// 실제 보호 Fort와 별도 생성 파일을 사용해 목록의 파일/State/Subscriber 연결을 확인한다.
#include "client/SoloMapCatalog.h"
#include "client/CampaignTemplateStorage.h"
#include "o/ConfigInterface.h"
#include "o/OriginalText.h"
#include "DudeSpawnSupport.h"
#include <algorithm>
#include <fstream>
using namespace netstorm::client;

namespace {
// 전체 지형 파서/제품 목록을 사용하지 않고 파일의 원시 길이 레코드만 분리한다.
std::vector<std::vector<std::uint8_t>> FileSections(std::span<const std::uint8_t> bytes) {
    if (bytes.size()<2 || bytes[0]!='F') throw std::invalid_argument("목록 검사 원문의 Fort 서명 누락");
    std::vector<std::vector<std::uint8_t>> sections;std::size_t position=2;
    for (std::size_t index=0;index<35;++index) { // 원본 고정 섹션 수만 읽고 월드 내용은 해석하지 않는다.
        if (bytes.size()-position<2) throw std::out_of_range("목록 검사 원문 길이 누락");const auto length=std::size_t(bytes[position])+(std::size_t(bytes[position+1])<<8);
        if (length<2 || length>bytes.size()-position) throw std::out_of_range("목록 검사 원문 섹션 범위");
        sections.emplace_back(bytes.begin()+position+2,bytes.begin()+position+length);position+=length;
    }return sections;
}
// 같은 파일에서 독립적으로 읽은 종류/첫 State/작성자/전체 원문이다.
struct Metadata {
    std::string name,subscriber; // 파일 줄기 이름과 원시 Subscriber 이름이다.
    std::uint8_t kind{},flags{}; // State+17과 State 첫 BYTE의 명시 기대 입력이다.
    std::string path; // 같은 원문 보존 확인에 사용할 실제 경로다.
    std::vector<std::uint8_t> bytes; // 테스트 중 보호 원문을 바꾸지 않았는지 다시 대조한다.
};
// 미정 바이트는 검사에서 명시 입력하고 게임 규칙은 실제 getter/목록 몸체에 맡긴다.
SoloMapCatalogEffects Effects(std::string& user,std::size_t& allocations) {
    SoloMapCatalogEffects effects;effects.typeCount=[] { return 188; };
    effects.allocationBytes=[&allocations](auto bytes) { ++allocations;std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    effects.userName=[&user] { return user; };effects.translate=[](auto text) { return std::string(text); };return effects;
}
// 실제 예외 경계를 성공한 빈 목록으로 해석하지 않는다.
template<class Function> bool Rejected(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
// 생성 검사 전용 Fort이며 보호 파일에는 쓰지 않는다.
std::vector<std::uint8_t> FileImage(std::span<const std::uint8_t> state) {
    std::vector<std::uint8_t> bytes{'F',0};
    for (std::size_t index=0;index<35;++index) { // Subscriber/State 이외의 불완전한 게임 자료는 목록에서 해석하지 않아야 한다.
        std::vector<std::uint8_t> section;
        if (!index) section={1,0,0,0,2,0,'M','e'};else if (index==1) section.assign(state.begin(),state.end());else if (index==4 || index==7) section={0xff};
        const auto length=section.size()+2;bytes.push_back(static_cast<std::uint8_t>(length));bytes.push_back(static_cast<std::uint8_t>(length>>8));bytes.insert(bytes.end(),section.begin(),section.end());
    }return bytes;
}
// 빌드 산출물 아래의 이 검사 전용 파일만 생성/덮어쓴다.
void Write(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!file) throw std::runtime_error("목록 검사 전용 Fort 쓰기 실패");
}
}

// 디스크 TEST01과 내장 맵 사본을 원본 OS 순서로 분류하고 모든 표시 행을 원시 파일과 대조한다.
TEST_CASE(SoloMapCatalog_ProtectedFilesAndCache) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::vector<Metadata> source;bool custom{};
    for (const auto& path:assets.files.MatchDisk("d/*.fort")) { // 실제 원본 Win32 디스크 열거 순서가 기대 목록 순서다.
        auto bytes=assets.files.Read(path);const auto sections=FileSections(bytes);const auto& subscriber=sections[0];const auto& state=sections[1];
        CHECK(subscriber.size()>=6);if (subscriber.size()<6) return;const auto length=std::size_t(subscriber[4])+(std::size_t(subscriber[5])<<8);CHECK(length<=subscriber.size()-6);if (length>subscriber.size()-6) return;
        auto author=std::string(reinterpret_cast<const char*>(subscriber.data()+6),length);author=author.substr(0,author.find('\0'));auto name=std::filesystem::path(path).stem().string();custom|=AsciiLower(name)=="test01";
        source.push_back({std::move(name),std::move(author),state.size()>17 ? state[17] : std::uint8_t{},state.empty() ? std::uint8_t{0xa1} : state[0],path,std::move(bytes)});
    }
    CHECK(custom && source.size()>15);std::string user="test author";std::size_t allocations{};ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");SoloMapCatalog catalog(assets.files,config,Effects(user,allocations));
    for (const auto mode:{0,1,2}) for (const auto option:{0,1}) { // 같은 파일을 실제 세 모드/편집 허용 조합으로 재분류한다.
        config.SetInt("mayEditMissions",option);catalog.Invalidate();std::vector<const Metadata*> expected;
        for (const auto& item:source) if ((item.kind==mode || (mode==2 && option && item.kind==1)) && expected.size()<50) expected.push_back(&item); // 독립 원시 BYTE로 필터/상한 기대만 만든다.
        const auto count=catalog.Refresh(mode);CHECK(count==static_cast<std::int32_t>(expected.size()) && !catalog.Cached().dirty);
        for (std::size_t index=0;index<expected.size();++index) { // 이름/손상/작성자/선택/활성 전체 행을 비교한다.
            const auto& item=*expected[index];const auto flags=((item.flags&1U) ? 2U : 0U)|(AsciiLower(item.subscriber)!=AsciiLower(user) ? 4U : 0U);
            CHECK(catalog.Cached().names[index]==item.name && catalog.Cached().flags[index]==flags);
            const auto row=catalog.Row(static_cast<std::int32_t>(index),item.name,"unused");CHECK(row && row->name==item.name && row->checked && row->enabled && row->label==item.name+((flags&2U) ? " (~rCORRUPT~.)" : ""));
        }
        CHECK(!catalog.Row(count,"",""));if (count) { const auto saved=catalog.Cached().names;config.Set("fortSpec","missing/{local.1}.fort");CHECK(catalog.Refresh(99)==count && catalog.Cached().names==saved);config.Set("fortSpec","d/{local.1}.fort"); }
        std::printf("catalog mode=%d edit=%d files=%zu accepted=%d\n",mode,option,source.size(),count);
    }
    for (const auto& item:source) { CHECK(catalog.Kind(item.name)==item.kind && assets.files.Read(item.path)==item.bytes); } // 분류용 읽기와 목록용 초기화가 보호 원문을 바꾸지 않는다.
    const auto customMap=std::find_if(source.begin(),source.end(),[](const auto& item) { return AsciiLower(item.name)=="test01"; });CHECK(customMap!=source.end());if (customMap==source.end()) return;
    user=customMap->subscriber;config.Set("fortSpec","d/TEST01.fort");catalog.Invalidate();CHECK(catalog.Refresh(2)==1 && catalog.Cached().names[0]=="TEST01" && !(catalog.Cached().flags[0]&4U));
    const auto customRow=catalog.Row(0,"","test01");CHECK(customRow && customRow->checked && customRow->enabled && customRow->name=="TEST01");
    std::array<std::size_t,256> kinds{};for (const auto& item:source) ++kinds[item.kind]; // 전체 파일의 독립 원시 종류 분포를 실행 근거에 남긴다.
    std::printf("catalog protected kinds0=%zu kinds1=%zu kinds2=%zu custom=%s kind=%u flags=%u\n",kinds[0],kinds[1],kinds[2],customMap->name.c_str(),customMap->kind,catalog.Cached().flags[0]);
}

// TAFF 파일은 실제 내용 읽기로 접근하되 디스크의 편집 목록에는 섞이지 않아야 한다.
TEST_CASE(SoloMapCatalog_DiskEnumerationAndArchiveRead) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data/solo-map-archive";std::filesystem::create_directories(root);
    BaseFileSystem files(root,assets.root);files.RegisterArchive(assets.root/"netstorm.tarc");ConfigInterface config({});config.Set("fortSpec","d/{local.1}.fort");std::string user;std::size_t allocations{};SoloMapCatalog catalog(files,config,Effects(user,allocations));
    CHECK(files.MatchDisk("d/*.fort").empty() && !files.Match("d/*.fort").empty());CHECK(catalog.Refresh(2)==0); // 기본 디스크가 비었으면 아카이브/보조 디렉터리의 Fort를 나열하지 않는다.
    std::size_t read{};
    for (const auto& path:files.Match("d/*.fort")) { // 열거 밖의 내용 읽기도 같은 실제 파일/State 부모로 검사한다.
        const auto sections=FileSections(files.Read(path));const auto& state=sections[1];const auto kind=state.size()>17 ? state[17] : std::uint8_t{};CHECK(catalog.Kind(std::filesystem::path(path).stem().string())==kind);++read;
    }
    const auto explicitName=files.MatchDisk("d/absent.fort");CHECK((explicitName==std::vector<std::string>{"d/absent.fort"}));std::printf("catalog disk-only menu empty, archive/secondary content reads=%zu\n",read);
}

// 빈/짧은 State의 실제 생성/확장과 메타 조회 두 수명을 별도 생성 파일로 확인한다.
TEST_CASE(SoloMapCatalog_EmptyShortAndMetadataOnly) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data/solo-map-catalog";std::filesystem::create_directories(root);
    const std::array<std::uint8_t,1> shortState{0};std::array<std::uint8_t,28> editState{};editState[17]=2;
    auto missionState=editState;missionState[17]=1;Write(root/"case-mission.fort",FileImage(missionState));
    Write(root/"case-empty.fort",FileImage({}));Write(root/"case-short.fort",FileImage(shortState));Write(root/"case-edit.fort",FileImage(editState));
    BaseFileSystem files(root);ConfigInterface config({});config.Set("fortSpec","{local.1}.fort");std::string user="mE";std::size_t allocations{};SoloMapCatalog catalog(files,config,Effects(user,allocations));
    CHECK(!catalog.Kind("case-empty") && allocations==1);CHECK(!catalog.Kind("case-short") && allocations==1);CHECK(catalog.Kind("case-edit")==2 && allocations==1);
    CHECK(catalog.Refresh(0)==2 && allocations==3); // 빈 State는 분류용/목록용 실제 Template에서 각각 생성한다.
    CHECK(catalog.Cached().names[0]=="case-empty" && catalog.Cached().flags[0]==2 && catalog.Cached().names[1]=="case-short" && !catalog.Cached().flags[1]);
    const auto row=catalog.Row(0,"","CASE-EMPTY");CHECK(row && row->checked && row->label=="case-empty (~rCORRUPT~.)");
    config.Set("fortSpec","case-edit.fort");CHECK(catalog.Refresh(2)==2);catalog.Invalidate();CHECK(catalog.Refresh(2)==1 && catalog.Cached().names[0]=="case-edit");
    config.Set("fortSpec","{local.1}.fort");config.SetInt("mayEditMissions",1);catalog.Invalidate();CHECK(catalog.Refresh(2)==2 && catalog.Cached().names[1]=="case-mission");
    config.SetInt("mayEditMissions",0);CHECK(catalog.Refresh(2)==2);catalog.Invalidate();CHECK(catalog.Refresh(2)==1);CHECK(Rejected([&] { catalog.Kind("missing"); }));
}

// 파싱 때의 작성자 사본이 아닌 현재 원시 Subscriber와 원본 signed 문자열 경계를 읽어야 한다.
TEST_CASE(SoloMapCatalog_CurrentSubscriberSection) {
    FortTemplate fort;fort.sections.resize(35);fort.sections[0]={7,0,0,0,5,0,'A',0,'B','C','D'};fort.subscriber=FortSubscriber{7,"stale"};CampaignTemplateStorage storage;storage.Install(0,std::move(fort),"sample");storage.Borrow(8,0);
    CHECK(storage.ReadSubscriberName(8)=="A" && storage.CurrentSection(0)=="Subscriber");auto& bytes=storage.Template(0).sections[0];bytes={1,0,0,0,2,0,'N','e'};CHECK(storage.ReadSubscriberName(0)=="Ne");
    bytes[4]=0xff;bytes[5]=0xff;CHECK(Rejected([&] { storage.ReadSubscriberName(8); }));bytes={1,0,0,0,0,0};CHECK(storage.ReadSubscriberName(0).empty());
    bytes={1,0,0,0,0xff,0};bytes.resize(261,'x');CHECK(storage.ReadSubscriberName(8)==std::string(255,'x'));bytes[4]=0;bytes[5]=1;bytes.push_back('x');CHECK(Rejected([&] { storage.ReadSubscriberName(0); }));
    bytes.clear();CHECK(Rejected([&] { storage.ReadSubscriberName(8); }));
}

// 원본은 분류 뒤 파일을 다시 읽으므로 그 사이 사라진 파일은 실패 flags1을 가진 행으로 남는다.
TEST_CASE(SoloMapCatalog_FileDisappearsBetweenReads) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data/solo-map-catalog";std::filesystem::create_directories(root);const auto path=root/"vanishing.fort";
    std::array<std::uint8_t,28> state{};state[17]=2;Write(path,FileImage(state));BaseFileSystem files(root);ConfigInterface config({});config.Set("fortSpec","vanishing.fort");std::string user="Me";std::size_t allocations{},reads{};auto effects=Effects(user,allocations);
    effects.typeCount=[&] { ++reads;CHECK(std::filesystem::remove(path));return 188; }; // 이미 읽은 분류 원문은 유지하고 이 검사 전용 파일 한 개만 제거한다.
    effects.userName=[]()->std::string { throw std::logic_error("읽기 실패한 맵의 Subscriber/사용자 비교를 실행하면 안 됩니다"); };
    SoloMapCatalog catalog(files,config,std::move(effects));CHECK(catalog.Refresh(2)==1 && reads==1 && catalog.Cached().flags[0]==1);
    const auto row=catalog.Row(0,"vanishing","");CHECK(row && row->label=="~rvanishing" && row->checked && row->enabled);CHECK(!std::filesystem::exists(path));
}

// 원본004595a0은 마지막 확장자뿐 아니라 파일 이름의 앞선 점도 제거한다.
TEST_CASE(SoloMapCatalog_FileSpecStripsAllExtensions) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"build/test-data/solo-map-dots";std::filesystem::create_directories(root);std::array<std::uint8_t,28> state{};state[17]=2;Write(root/"map.extra.fort",FileImage(state));
    BaseFileSystem files(root);ConfigInterface config({});config.Set("fortSpec","map.extra.fort");std::string user="Me";std::size_t allocations{};SoloMapCatalog catalog(files,config,Effects(user,allocations));
    CHECK(catalog.Refresh(2)==1 && catalog.Cached().names[0]=="map");const auto row=catalog.Row(0,"map","");CHECK(row && row->label=="map" && row->checked);
}
