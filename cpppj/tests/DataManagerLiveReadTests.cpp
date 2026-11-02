// 명시 DataManager 읽기의 원문 부모와 실제 Client/Storage 부분 효과를 검사한다.
#include "QuickHelpFileReadTestSupport.h"
#include "client/DataManagerFileRead.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawColorMapCache.h"
#include "o/RawDataManagerImage.h"
#include "o/RawDataManagerSelection.h"

namespace { // Native/OS 입력은 검사 소유자에만 두고 게임/MP를 시작하지 않는다.
using namespace netstorm;using namespace netstorm::client;using namespace netstorm::o;using namespace netstorm::test::rawscene;
constexpr std::uintptr_t kLiveFile=0x12200000,kLiveKey=0x12300100,kLiveAllocation=0x12302000; // 763 및769 원문 장치의 같은 격리 토큰이다.
constexpr std::uint32_t kLivePoison=0x65656565; // 원문 미작성 File 필드의 명시 seed다.
struct LiveBoundary { std::string kind; }; // 명시 Open/할당/read/close/서명/본문/반납 예외다.
std::vector<std::uint8_t> LiveBytes(const std::string& hex) { std::vector<std::uint8_t> bytes;for (std::size_t at=0;at<hex.size();at+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));return bytes; } // Native 입력의 원시 저장 바이트만 복원한다.
std::string LiveWords(std::span<const std::uint8_t> bytes,std::size_t count) { std::string result;for (std::size_t index=0;index<count;++index) { if (!result.empty()) result+=',';result+=std::to_string(Get(bytes,index*4)); }return result; } // 같은 MarkStream20의 다섯 DWORD다.
std::string LiveFields(const RawBaseFileState& file) { const std::array<std::uintptr_t,6> values{{file.file,file.flags,static_cast<std::uint32_t>(file.offset),static_cast<std::uint32_t>(file.length),file.key,file.keyLength}};std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; } // File 반환 직후의 실제 필드를 보존한다.
std::string LiveEvents(const std::vector<std::string>& events) { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result; } // 원문 장치/서명/본문/현재할당 반납 순서다.
std::string LiveText(std::string_view name) { return Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()}); } // 파일 이름은 인코딩을 바꾸지 않고 비교한다.
std::vector<std::uint8_t> LiveImage(std::uint8_t value) { std::vector<std::uint8_t> bytes{'F',0xfe};for (std::size_t section=0;section<FortTemplate::kSectionCount;++section) { bytes.push_back(static_cast<std::uint8_t>(section==1?3:2));bytes.push_back(0);if (section==1) bytes.push_back(value); }return bytes; } // 실제35개 섹션 중 State1BYTE만 독립 값으로 작성한다.
TemplateFileLoadHooks LiveEffects() { TemplateFileLoadHooks hooks;hooks.read=[](auto)->std::vector<std::uint8_t> { throw std::logic_error("Native reader의 값형 fallback 호출"); };hooks.sameName=[](auto a,auto b) { return !CompareDataManagerSectionNames(a,b); };hooks.wrongName=[] { throw std::logic_error("예상밖이름진단"); };hooks.readOnly=[](auto) { return false; };hooks.typeCount=[] { return 188; };hooks.tooManyTypes=[] { throw std::logic_error("예상밖TYPE진단"); };hooks.rejectFile=[](auto) { throw std::logic_error("예상밖서명삭제"); };hooks.currentSlot=[] { return 1U; };hooks.refreshTitle=[] {};return hooks; } // 명시 Native read 이외의 실제 Template 효과를 공급한다.
}

TEST_CASE(DataManagerLiveRead_Native64ParentMinimumAllocationSignatureAndCleanup) { // 실제44d380의 빈 이름표 경로까지 부모/서명/전체read/반납을 비교한다.
    std::size_t rows{};
    auto fixture=LoadFixture(NETSTORM_DATAMANAGER_LIVE_READ_FIXTURE);const auto body=LoadFixture(NETSTORM_DATAMANAGER_LIVE_BODY_FIXTURE);fixture.rows.insert(fixture.rows.end(),body.rows.begin(),body.rows.end()); // 기존60행과독립본문4행을모두실행한다.
    for (const auto& row:fixture.rows) { // 앞6열만 입력이며 나머지는 실제 PE 기대값이다.
        CHECK(row.size()==13);if (row.size()!=13) return;const auto payload=LiveBytes(row[0]);const auto key=Number(row[1]),subfile=Number(row[2]),probe=Number(row[3]),scenario=Number(row[5]);auto available=Number(row[4]);std::uint32_t used{},position{};std::vector<std::string> events;std::vector<std::uint8_t> allocation;std::array<std::uint8_t,4096> scratch;scratch.fill(101);std::array<std::uint8_t,276> manager{};Put(manager,0x110,0xffffffffU);std::array<std::uint8_t,3> names{'/','/',0};std::span<std::uint8_t> current;
        std::array<std::uint8_t,4> nullPage{}; // 시나리오7 Native의명시mapped0페이지다. 실제할당블록과분리하여NULLtoken읽기의물리효과만공급한다.
        RawBaseFileState file{kLivePoison,kLivePoison,static_cast<std::int32_t>(kLivePoison),static_cast<std::int32_t>(kLivePoison),kLivePoison,kLivePoison};RawBaseFileIOHooks io;BaseFileWholeReadHooks whole;DataManagerFileHooks parent;
        whole.probe=[&](auto physical) { const auto end=std::find(physical.begin(),physical.end(),std::uint8_t{});events.push_back("P:"+Hex(physical.first(static_cast<std::size_t>(end-physical.begin()))));return probe; };
        whole.open=[&](auto& state,auto physical,auto mode,auto create) { const auto end=std::find(physical.begin(),physical.end(),std::uint8_t{});events.push_back("O:"+Hex(physical.first(static_cast<std::size_t>(end-physical.begin())))+":"+std::to_string(mode)+":"+std::to_string(create)+":"+LiveFields(state));state={scenario==2?0:kLiveFile,0x80U|subfile,subfile?37:0,subfile?static_cast<std::int32_t>(payload.size()):0,key?kLiveKey:0,key?3U:0U};position=subfile?37U:0U;if (scenario==1) throw LiveBoundary{"open"}; };
        whole.allocate=[&](auto size) { events.push_back("N:"+std::to_string(size));if (scenario==3) throw LiveBoundary{"allocation"};allocation.assign(size,205);if (scenario==7) return BaseFileWholeAllocation{0,nullPage};return BaseFileWholeAllocation{kLiveAllocation,allocation}; };
        io.tell=[&](auto token) { events.push_back("T:"+std::to_string(token));return static_cast<std::int32_t>(position); };io.seek=[&](auto token,auto offset,auto origin) { events.push_back("S:"+std::to_string(token)+":"+std::to_string(static_cast<std::uint32_t>(offset))+":"+std::to_string(origin));position=origin==2?static_cast<std::uint32_t>(payload.size()):static_cast<std::uint32_t>(offset); };
        io.read=[&](auto token,auto destination,auto size,auto count) { const auto actual=std::min(size*count,available);events.push_back("R:"+std::to_string(token)+":"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==4) throw LiveBoundary{"read"};if (actual) std::copy_n(payload.begin()+used,actual,destination.begin());used+=actual;available-=actual;position+=actual; };
        const std::array<std::uint8_t,3> keyBytes{{'a','b','c'}};io.key=[&](auto token,auto size) { CHECK(token==kLiveKey && size==3);return std::span<const std::uint8_t>(keyBytes); };io.close=[&](auto token) { events.push_back("C:"+std::to_string(token));if (scenario==5) throw LiveBoundary{"close"}; };io.freeKey=[&](auto token) { events.push_back("F:"+std::to_string(token)); };
        parent.fileName=[&] { events.push_back("N:"+LiveText("input.fort"));return std::string_view("input.fort"); };
        parent.readFile=[&](auto name,auto check,auto output,auto extra) { CHECK(check==1 && extra==0);events.push_back("W:1:0");std::string input(name);input+='\0';std::int32_t length{};const auto token=ReadDataManagerFileBytes(file,{reinterpret_cast<const std::uint8_t*>(input.data()),input.size()},length,io,whole,scratch);Put(output,0,static_cast<std::uint32_t>(length));return static_cast<std::uint32_t>(token); };
        parent.signature=[&](auto source) { current=source;events.push_back("G:"+LiveWords(source,5));if (scenario==6) throw LiveBoundary{"signature"};TemplateFileSignatureHooks signature;signature.buffer=[&](auto token) { CHECK(token==kLiveAllocation);return std::span(allocation); };signature.typeCount=[] { return 188; };signature.fileName=parent.fileName;signature.rejectFile=[&](auto name) { events.push_back("X:"+LiveText(name)); };return ReadTemplateFileSignature(manager,source,signature); };
        parent.image.names=[&] { events.push_back("B:"+LiveWords(current,5));if (scenario==7 || scenario==9) throw LiveBoundary{"body"};return std::span(names); };parent.image.assertion=[&](auto kind) { if (kind!=DataManagerImageAssertion::MarkPosition) throw std::logic_error("Native 관측 밖 DataManager 진단");events.push_back("D:42");throw LiveBoundary{"assertion"}; }; // 길이1 F의 실제pos>end 진단도 실패로 전파하며 파서 오류를 무시하지 않는다.
        parent.releaseFile=[&](auto token) { events.push_back("A:"+std::to_string(token));if (scenario==8) throw LiveBoundary{"release"}; };std::string outcome="ok";bool result{};
        try { result=ReadDataManagerFile(manager,parent); }catch (const LiveBoundary& failure) { outcome=failure.kind; }
        // 원문 기대값을 바꾸지 않고 첫 차이의 입력/필드/효과를 원시 로그에 남긴다.
        const std::array<std::string,7> observed{{outcome,result?"1":"0",std::to_string(Get(manager,0x10c)),std::to_string(Get(manager,0x110)),LiveFields(file),LiveEvents(events),allocation.empty()?"-":Hex(allocation)}};
        for (std::size_t field=0;field<observed.size();++field) { // 각 원문 결과 열을 독립적으로 식별한다.
            if (observed[field]!=row[field+6]) std::printf("Native row %zu payload=%s key=%u subfile=%u probe=%u available=%s scenario=%u field=%zu\n  actual=%s\n  expected=%s\n",rows+1,row[0].c_str(),key,subfile,probe,row[4].c_str(),scenario,field,observed[field].c_str(),row[field+6].c_str());
        }
        CHECK(outcome==row[6] && result==static_cast<bool>(Number(row[7])) && Get(manager,0x10c)==Number(row[8]) && Get(manager,0x110)==Number(row[9]) && LiveFields(file)==row[10] && LiveEvents(events)==row[11] && (allocation.empty()?"-":Hex(allocation))==row[12]);++rows;
    }
    CHECK(rows==64);
}

TEST_CASE(DataManagerLiveRead_ActualClientMissingAndEmptyAreSeparateAllocationsWithSameFalseResult) { // 빈 파일을 새 파싱 오류/삭제로 주장하지 않고 존재/최소4와 정상false 정리를 구분한다.
    using namespace netstorm::test::helpread;const auto root=Work();std::filesystem::create_directories(root);Client instance(Options(root));CHECK(!ReadDataManagerFileImage(instance.Files(),"d/missing.fort"));Text(root/"d/empty.fort","");const auto empty=ReadDataManagerFileImage(instance.Files(),"d/empty.fort");CHECK(empty && empty->length==0 && empty->storage.size()==4);
    CampaignTemplateStorage storage;auto effects=BindDataManagerFileRead(LiveEffects(),instance.Files());std::size_t signatures{},rejects{};effects.typeCount=[&] { ++signatures;return 188; };effects.rejectFile=[&](auto) { ++rejects; };storage.LoadFile(1,{},"d/missing.fort",effects);CHECK(!Get(storage.Headers(),20) && !storage.OwnedImages());storage.LoadFile(1,{},"d/empty.fort",effects);CHECK(!Get(storage.Headers(),20) && !storage.OwnedImages() && signatures==0 && rejects==0);
}

TEST_CASE(DataManagerLiveRead_ActualClientCurrentArchivePayloadKeepsLegacySnapshot) { // 실제35개 섹션 parser는 현재 FILE을 쓰고 기존 Files.Read는 이전 snapshot을 유지한다.
    using namespace netstorm::test::helpread;const auto root=Work(),path=root/"one.tarc";const auto first=LiveImage(11),second=LiveImage(29);Write(path,Archive("d/map.fort",first,true));Client instance(Options(root));CHECK(instance.Files().Read("d/map.fort")==first);Write(path,Archive("d/map.fort",second,true));CampaignTemplateStorage storage;auto effects=BindDataManagerFileRead(LiveEffects(),instance.Files());storage.LoadFile(1,{},"d/map.fort",effects);
    CHECK(storage.OwnedImages()==1 && storage.State(1).bytes.size()==1 && storage.State(1).bytes[0]==29 && storage.FileHeader(1)==(std::array<std::uint32_t,2>{'F',1}) && instance.Files().Read("d/map.fort")==first);
}

TEST_CASE(DataManagerLiveRead_ActualSelectedArchiveOpenFailureKeepsPublishedManager) { // Probe metadata 성공 뒤 선택한 backing FILE 실패는 보조 파일/optional 없음으로 바꾸지 않는다.
    using namespace netstorm::test::helpread;const auto root=Work(),secondary=Work(),path=root/"one.tarc";const auto image=LiveImage(13);Write(path,Archive("d/map.fort",image));Write(secondary/"d/map.fort",image);Client instance(Options(root));CHECK(instance.Files().ContainsArchiveOnly("d/map.fort"));instance.InitializeNativeSecondaryFiles(platform::PathToUtf8(secondary));std::filesystem::rename(path,root/"moved.tarc");CampaignTemplateStorage storage;auto effects=BindDataManagerFileRead(LiveEffects(),instance.Files());std::size_t parses{};effects.typeCount=[&] { ++parses;return 188; };bool failed{};
    try { storage.LoadFile(1,{},"d/map.fort",effects); }catch (const ColorCacheUnableToOpen&) { failed=true; }
    CHECK(failed && Get(storage.Headers(),20) && storage.OwnedImages()==1 && parses==0); // Load가 먼저 공개한 manager/readonly 효과는 유지한다.
}

TEST_CASE(DataManagerLiveRead_ActualTruncatedEncryptedFileFailsBeforeParserAndClosesFile) { // 실제 짧은fread는반납뒤중단하고미정padding을성공섹션으로채우지않는다.
    using namespace netstorm::test::helpread;const auto root=Work(),path=root/"one.tarc";const auto image=LiveImage(17);auto archive=Archive("d/map.fort",image,true);Write(path,archive);Client instance(Options(root));CHECK(instance.Files().ContainsArchiveOnly("d/map.fort"));archive.resize(archive.size()-3);Write(path,archive);CampaignTemplateStorage storage;auto effects=BindDataManagerFileRead(LiveEffects(),instance.Files());std::size_t parses{};effects.typeCount=[&] { ++parses;return 188; };bool failed{};
    try { storage.LoadFile(1,{},"d/map.fort",effects); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="DataManager 전체읽기의 실제 부분 I/O"; }
    CHECK(failed && Get(storage.Headers(),20) && parses==0);std::filesystem::rename(path,root/"short.tarc");Write(path,Archive("d/map.fort",image,true));storage.Clear(1);storage.LoadFile(1,{},"d/map.fort",effects);CHECK(storage.State(1).bytes.size()==1 && storage.State(1).bytes[0]==17);
}

TEST_CASE(DataManagerLiveRead_ActualCompleteIoWithMalformedImageKeepsParsedPrefix) { // I/O 정상과 파서 실패를 구분하고 이미 붙인 원문 Subscriber 노드를 보존한다.
    using namespace netstorm::test::helpread;const auto root=Work();const std::array<std::uint8_t,9> malformed{{'F',0xfe,5,0,11,13,17,8,0}};Write(root/"d/map.fort",malformed);Client instance(Options(root));CampaignTemplateStorage storage;auto effects=BindDataManagerFileRead(LiveEffects(),instance.Files());bool failed{};
    try { storage.LoadFile(1,{},"d/map.fort",effects); }catch (const std::out_of_range&) { failed=true; }
    CHECK(failed && Get(storage.Headers(),20) && storage.OwnedImages()==1 && storage.SectionOrder(1)==std::vector<std::string>{"Subscriber"});const auto bytes=storage.Template(1).Section("Subscriber");CHECK(bytes.size()==3 && bytes[0]==11 && bytes[1]==13 && bytes[2]==17);
}
