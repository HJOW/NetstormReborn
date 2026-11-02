// 실제TYPE 파일 접합과 별도로 관측한49c160의 고정 호출·formatter·실패 수명을 확인한다.
#include "QuickHelpFileReadTestSupport.h"
#include "client/TypeFileRead.h"
#include "o/RawColorMapCache.h"
#include <cstdio> // 최초 원문 대조 실패의 행·상태·호출·할당 바이트를 기록한다.

namespace { // 검사 소유 입력만 기록하며 원본/기존fixture를 수정하지 않는다.
using namespace netstorm;using namespace netstorm::test::rawscene; // 원문 BYTE fixture 해석기와 현재 파일 장치를 함께 쓴다.
constexpr std::uintptr_t kFile=0x12200000,kKey=0x12300100,kAllocation=0x12302000; // Native 격리FILE/키/할당과 같은 관측 토큰이다.
constexpr std::uint32_t kPoison=0x65656565,kSizeSeed=0x76543210; // 미작성 필드와 원문이 유지하는 입력 길이의 명시seed다.
struct BoundaryFailure { std::string kind; /* 원문 실제 Open/할당/읽기/닫기 실패를 구별한다. */ }; // C++ OS 예외의 ABI를 복제하지 않는다.
std::string Fields(const o::RawBaseFileState& state) { // 원문 여섯DWORD를 동일 순서로 직렬화한다.
    const std::array<std::uintptr_t,6> values{{state.file,state.flags,static_cast<std::uint32_t>(state.offset),static_cast<std::uint32_t>(state.length),state.key,state.keyLength}};std::string result;
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); } // Native와 같은 필드 순서다.
    return result;
}
std::string Events(const std::vector<std::string>& events) { // 실제 선택된 효과만 원래 순서로 연결한다.
    std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; } // 호출되지 않은 장치를 합성하지 않는다.
    return result;
}
std::string TextHex(std::span<const std::uint8_t> bytes) { return Hex(bytes.first(static_cast<std::size_t>(std::find(bytes.begin(),bytes.end(),std::uint8_t{})-bytes.begin()))); } // 실제 NUL까지의 이름만 기록한다.
std::vector<std::uint8_t> Unhex(std::string_view text) { // Native 입력 열만16진수에서 저장 바이트로 되돌린다.
    CHECK(text.size()%2==0);std::vector<std::uint8_t> result;result.reserve(text.size()/2);
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); // 빈 본문도 그대로 빈 벡터다.
    return result;
}
std::filesystem::path Work() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-type-live-read-777/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())/std::filesystem::path(u8"타입😀"); } // 창/게임을 열지 않는 검사 전용 새Unicode 루트다.
void Text(const std::filesystem::path& path,std::string_view text) { test::helpread::Text(path,text); } // 기존 독립 입력 작성기로 검사 파일만 쓴다.
std::span<const std::uint8_t> Bytes(std::string_view text) { return {reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}; } // 본문과 저장 길이를 분리하지 않는다.
}

TEST_CASE(TypeLiveFileRead_Native47FixedCallFormatterAndWholeReadLifetime) { // 부모 관측에서 call/이름·파일수명만 대조하며 scanner/파서를 성공으로 합성하지 않는다.
    std::size_t rows{};
    const auto nullRows=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/type-live-null-destination-x86.tsv").string().c_str()).rows;CHECK(nullRows.size()==4); // 반환0의 읽기 대상은 별도 원문 관측으로 검증한다.
    for (const auto& row:LoadFixture(NETSTORM_TYPE_LIVE_READ_FIXTURE).rows) { // 입력6열만 장치로 제공하고 원문 출력10열은 독립 기대값이다.
        CHECK(row.size()==16);const auto typeName=Unhex(row[0]),payload=Unhex(row[1]);const auto key=Number(row[2]),subfile=Number(row[3]),scenario=Number(row[5]);auto available=Number(row[4]);const auto length=static_cast<std::uint32_t>(payload.size());std::uint32_t used{},position{};std::vector<std::string> events;std::vector<std::uint8_t> allocation,nullDestination;std::array<std::uint8_t,4096> scratch;scratch.fill(101); // 예약한 할당 블록과 반환0에 대응하는 호스트 물리 span은 서로 다른 입력이다.
        o::RawFileName names;const std::array<o::FileNamePart,1> initial{{{-3,"seed"}}};for (int index=0;index<8;++index) static_cast<void>(names.Format(initial)); // Native의 최초 고리8을 별도 입력으로 만든다.
        static_cast<void>(client::FormatTypeFileName(names,{reinterpret_cast<const char*>(typeName.data()),typeName.size()}));CHECK(names.Index()==Number(row[7]));const auto physical=names.CurrentBytes();const auto sourceEvents=Split(row[14],';');CHECK(!sourceEvents.empty());CHECK(sourceEvents.front()=="W:"+TextHex(physical)+":0:0:"+std::to_string(kSizeSeed));
        o::RawBaseFileState state{kPoison,kPoison,static_cast<std::int32_t>(kPoison),static_cast<std::int32_t>(kPoison),kPoison,kPoison};o::RawBaseFileIOHooks io;o::BaseFileWholeReadHooks hooks;std::int32_t output=static_cast<std::int32_t>(kSizeSeed);
        hooks.probe=[](auto)->std::uint32_t { throw std::logic_error("TYPE check0에서 Probe 도달"); }; // 원문이 선택하지 않은 존재조회에 도달하면 실패한다.
        hooks.open=[&](auto& file,auto name,auto mode,auto create) { events.push_back("O:"+TextHex(name)+":"+std::to_string(mode)+":"+std::to_string(create)+":"+Fields(file));file={scenario==2?0:kFile,0x80U|subfile,subfile?37:0,subfile?static_cast<std::int32_t>(length):0,key?kKey:0,key?3U:0U};position=subfile?37U:0U;if (scenario==1) throw BoundaryFailure{"open"}; };
        hooks.allocate=[&](auto requested) { events.push_back("N:"+std::to_string(requested));if (scenario==3) throw BoundaryFailure{"allocation"};allocation.assign(requested,205);if (scenario==7) { nullDestination.assign(requested,0);return o::BaseFileWholeAllocation{0,nullDestination}; }return o::BaseFileWholeAllocation{kAllocation,allocation}; }; // NULL 반환에서는 예약 블록을 읽기 대상으로 잘못 빌리지 않으며 실제 호스트 NULL에 쓰지 않는다.
        io.tell=[&](auto token) { events.push_back("T:"+std::to_string(token));return static_cast<std::int32_t>(position); };io.seek=[&](auto token,auto offset,auto origin) { events.push_back("S:"+std::to_string(token)+":"+std::to_string(static_cast<std::uint32_t>(offset))+":"+std::to_string(origin));position=origin==2?length:static_cast<std::uint32_t>(offset); };
        io.read=[&](auto token,auto destination,auto size,auto count) { const auto actual=std::min(size*count,available);events.push_back("R:"+std::to_string(token)+":"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==4) throw BoundaryFailure{"read"};for (std::uint32_t index=0;index<actual;++index) destination[index]=payload[used+index];used+=actual;available-=actual;position+=actual; }; // 미작성 allocation/scratch seed를 유지한다.
        const std::array<std::uint8_t,3> keyBytes{{'a','b','c'}};io.key=[&](auto token,auto size) { CHECK(token==kKey && size==3);return std::span<const std::uint8_t>(keyBytes); };io.close=[&](auto token) { events.push_back("C:"+std::to_string(token));if (scenario==5) throw BoundaryFailure{"close"}; };io.freeKey=[&](auto token) { events.push_back("F:"+std::to_string(token)); };
        std::uintptr_t result{};std::string outcome="ok";try { result=client::ReadTypeFileBytes(state,physical,output,io,hooks,scratch); }catch (const BoundaryFailure& error) { outcome=error.kind; }
        std::vector<std::string> expectedEvents;for (std::size_t index=1;index<sourceEvents.size();++index) { if (!sourceEvents[index].starts_with("D:")) expectedEvents.push_back(sourceEvents[index]); } // 부모의 후단 진단은 call 경계 대조와 구별하고 실제 빈파일 검사에서 따로 확인한다.
        const auto actualFields=Fields(state),actualEvents=Events(events),expectedTrace=Events(expectedEvents),actualAllocation=allocation.empty()?"-":Hex(allocation); // 집계 단언의 차이를 실제 관측 항목별로 기록한다.
        if (actualFields!=row[13] || actualEvents!=expectedTrace || actualAllocation!=row[15]) std::fprintf(stderr,"TYPE row=%zu scenario=%u\nfields actual=%s expected=%s\nevents actual=%s expected=%s\nallocation actual=%s expected=%s\n",rows,scenario,actualFields.c_str(),row[13].c_str(),actualEvents.c_str(),expectedTrace.c_str(),actualAllocation.c_str(),row[15].c_str()); // 기대값을 바꾸지 않고 최초 실패 원인을 구별한다.
        CHECK(outcome==(row[6]=="assertion" ? "ok":row[6]) && result==Number(row[9]) && static_cast<std::uint32_t>(output)==Number(row[10]));CHECK(actualFields==row[13] && actualEvents==expectedTrace && actualAllocation==row[15]);++rows;
        if (scenario==7) { // 원문41bb53의 Read 반환 직후 물리와 비교하며 SEH의 후속 FS0 복구는 호스트에 합성하지 않는다.
            bool found=false;for (const auto& observation:nullRows) { // 네 key/subfile 입력을 원문에서 관측한 대상 바이트와 연결한다.
                CHECK(observation.size()==3);if (observation.size()==3 && Number(observation[0])==key && Number(observation[1])==subfile) { found=true;CHECK(Hex(nullDestination)==observation[2]); }
            }CHECK(found);
        }
    }
    CHECK(rows==47);
}

TEST_CASE(TypeLiveFileRead_CurrentEncryptedArchivePreservesSnapshotApi) { // metadata 등록 뒤 실제 backing FILE 변경을 TYPE만 다시 읽는다.
    const auto root=Work(),archive=root/"types.tarc";const std::string first="typename First {}",second="typename Fresh {}";test::helpread::Write(archive,test::helpread::Archive("d/neutral.type",Bytes(first),true));o::BaseFileSystem files(root);files.RegisterArchives();CHECK(files.Read("d/neutral.type")==std::vector<std::uint8_t>(Bytes(first).begin(),Bytes(first).end()));test::helpread::Write(archive,test::helpread::Archive("d/neutral.type",Bytes(second),true));const auto before=files.FileNameOwner()->Index();
    CHECK(client::ReadNativeTypeDefinition(files,"neutral").name=="Fresh");CHECK(files.FileNameOwner()->Index()==(before+3)%10);CHECK(files.Read("d/neutral.type")==std::vector<std::uint8_t>(Bytes(first).begin(),Bytes(first).end()));std::filesystem::rename(archive,root/"closed.tarc"); // Parse 이후 FILE/키가 살아 남지 않는다.
}

TEST_CASE(TypeLiveFileRead_CurrentDiskAndSecondaryUnicodeRoots) { // 원문Open의 디스크 우선/현재 보조 root를 같은 파일 owner로 읽는다.
    const auto root=Work(),secondary=Work(),replacement=Work();Text(root/"d/neutral.type","typename Disk {}");Text(secondary/"d/neutral.type","typename Secondary {}");Text(replacement/"d/neutral.type","typename Changed {}");o::BaseFileSystem files(root);files.SetSecondaryDirectory(secondary);CHECK(client::ReadNativeTypeDefinition(files,"neutral").name=="Disk");std::filesystem::remove(root/"d/neutral.type");CHECK(client::ReadNativeTypeDefinition(files,"neutral").name=="Secondary");files.SetSecondaryDirectory(replacement);CHECK(client::ReadNativeTypeDefinition(files,"neutral").name=="Changed");
}

TEST_CASE(TypeLiveFileRead_RequiredMissingEmptyAndSyntaxFailureReleaseThenRetry) { // 선택된 실패를 성공 빈타입으로 바꾸지 않으며 다음 읽기가 현재 파일을 다시 연다.
    const auto root=Work();std::filesystem::create_directories(root);o::BaseFileSystem files(root);bool missing{};try { static_cast<void>(client::ReadNativeTypeDefinition(files,"neutral")); }catch (const o::ColorCacheUnableToOpen&) { missing=true; }CHECK(missing);Text(root/"d/neutral.type","");bool empty{};try { static_cast<void>(client::ReadNativeTypeDefinition(files,"neutral")); }catch (const std::runtime_error& error) { empty=std::string_view(error.what()).find("0x51c")!=std::string_view::npos; }CHECK(empty);
    Text(root/"d/neutral.type","typename Broken { foot_x = ; }");bool invalid{};try { static_cast<void>(client::ReadNativeTypeDefinition(files,"neutral")); }catch (const std::runtime_error& error) { invalid=std::string_view(error.what()).find("Invalid .type syntax")!=std::string_view::npos; }CHECK(invalid);std::filesystem::rename(root/"d/neutral.type",root/"invalid.type");Text(root/"d/neutral.type","typename Retry { foot_x = 2; }");const auto type=client::ReadNativeTypeDefinition(files,"neutral");CHECK(type.name=="Retry" && type.Number("foot_x")==2);
}

TEST_CASE(TypeLiveFileRead_TruncatedEncryptedArchiveRejectsAfterCloseAndKeepsTypedOpenFailure) { // metadata 이후 backing 파일의 부족과 미발견은 서로 다른 실패로 유지한다.
    const auto root=Work(),archive=root/"types.tarc",secondary=Work();const std::string text="typename Neutral {}";auto bytes=test::helpread::Archive("d/neutral.type",Bytes(text),true);test::helpread::Write(archive,bytes);o::BaseFileSystem files(root);files.RegisterArchives();bytes.resize(bytes.size()-3);test::helpread::Write(archive,bytes);bool partial{};try { static_cast<void>(client::ReadNativeTypeDefinition(files,"neutral")); }catch (const std::runtime_error& error) { partial=std::string_view(error.what())=="TYPE 전체읽기의 실제 부분 I/O"; }CHECK(partial);std::filesystem::rename(archive,root/"truncated.tarc");Text(secondary/"d/neutral.type",text);files.SetSecondaryDirectory(secondary);bool typed{};try { static_cast<void>(client::ReadNativeTypeDefinition(files,"neutral")); }catch (const o::ColorCacheUnableToOpen&) { typed=true; }CHECK(typed);test::helpread::Write(archive,test::helpread::Archive("d/neutral.type",Bytes(text),true));CHECK(client::ReadNativeTypeDefinition(files,"neutral").name=="Neutral");
}
