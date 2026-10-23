// 기존 원문 부모를 실제 Storage 소유자에 연결한 제안만 격리해 검사한다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawDataManagerSelection.h"
#include <algorithm>
#include <array>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 실제20BYTE 헤더의 현재 DWORD만 조회한다.
std::uint32_t Value(std::span<const std::uint8_t> bytes,std::size_t at=0) {
    std::uint32_t result{};for (std::size_t index=0;index<4;++index) result|=std::uint32_t(bytes[at+index])<<(index*8);return result; // 네 BYTE의 원문 순서다.
}
// 선택한 필드 이외의 stale 헤더를 보존한다.
void Field(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 같은 DWORD만 쓴다.
}
// 파일의35개 길이 레코드만 구성하며 원본 출력 기대를 이 생성기에서 계산하지 않는다.
std::vector<std::uint8_t> Image(std::vector<std::uint8_t> subscriber={7,0,0,0,1,0,'A'},std::vector<std::uint8_t> state={3,5,7}) {
    std::vector<std::uint8_t> result{'F',0xfe};
    for (std::size_t index=0;index<FortTemplate::kSectionCount;++index) { // 두 실제 payload 외에는 길이2의 빈 섹션이다.
        const auto bytes=index==0 ? std::span(subscriber) : index==1 ? std::span(state) : std::span<std::uint8_t>{};const auto length=bytes.size()+2;
        result.push_back(static_cast<std::uint8_t>(length));result.push_back(static_cast<std::uint8_t>(length>>8));result.insert(result.end(),bytes.begin(),bytes.end());
    }return result;
}
// 실제 호출한 하위 계약의 거부만 검사의 bool로 읽는다.
template<class Function> bool Rejects(Function function) { try { function(); }catch (const std::exception&) { return true; }return false; }
// 명시 입력의 실제 하위 경계를 기본으로 연결하며 읽기 결과만 검사마다 바꾼다.
TemplateFileLoadHooks Effects(std::vector<std::uint8_t> image) {
    TemplateFileLoadHooks hooks;hooks.read=[image=std::move(image)](auto) { return image; };
    hooks.sameName=[](auto first,auto second) { return !CompareDataManagerSectionNames(first,second); }; // 합성 공식 ASCII 파일명의 실제 비교 모듈을 재사용한다.
    hooks.wrongName=[] { throw std::logic_error("검사에서 선택하지 않은 이름 불일치 진단"); };hooks.readOnly=[](auto) { return false; };
    hooks.typeCount=[] { return 188; };hooks.tooManyTypes=[] { throw std::logic_error("검사에서 선택하지 않은 TYPE 진단"); };
    hooks.rejectFile=[](auto) { throw std::logic_error("검사에서 선택하지 않은 파일 제거 요청"); };hooks.currentSlot=[] { return 1U; };hooks.refreshTitle=[] {};
    return hooks;
}
// stream의 원문 위치와 실제 할당 주소를 같은 Storage 빌림에서 관측한다.
struct StreamValue { const std::uint8_t* allocation{};std::uint32_t position{},end{}; }; // 호스트 주소는 동일성 비교에만 쓰고 DWORD 토큰으로 바꾸지 않는다.
// 동기 operation 밖에는 span을 보관하지 않으며 할당 주소 값만 비교한다.
StreamValue Observe(CampaignTemplateStorage& storage,TemplateWriteStream stream) {
    StreamValue result;storage.AccessSection(stream,[&](auto fields,const auto& memory) { result.position=Value(fields,4);result.end=Value(fields,8);if (Value(fields)) result.allocation=memory.buffer(Value(fields)).data(); });return result;
}
}

// 기존 파일의 이름 비교는 실제 State/current/노드 수명/헤더를 모두 그대로 둔다.
TEST_CASE(CampaignTemplateLoadFile_ExistingNamePreservesActualOwnerAndCursor) {
    CampaignTemplateStorage storage;storage.InstallFile(1,FortTemplate{},Image(),"Keep.fort");const auto state=storage.SelectSection(1,"State",1);
    storage.State(1).position=2;const auto observed=Observe(storage,state);auto header=storage.Headers().subspan(20,20);Field(header,8,7);Field(header,12,0xaabbccdd);Field(header,16,42);
    const std::vector<std::uint8_t> before(header.begin(),header.end());std::size_t compares{},diagnostics{};TemplateFileLoadHooks hooks;
    hooks.sameName=[&](auto first,auto second) { ++compares;CHECK(first=="Keep.fort");return !CompareDataManagerSectionNames(first,second); };hooks.wrongName=[&] { ++diagnostics; };
    storage.LoadFile(1,FortTemplate{},"KEEP.FORT",hooks);storage.LoadFile(1,FortTemplate{},"Other.fort",hooks);
    CHECK(compares==2 && diagnostics==1 && storage.OwnedImages()==1 && storage.CurrentSection(1)=="State");CHECK(std::equal(header.begin(),header.end(),before.begin()));
    CHECK(Observe(storage,state).allocation==observed.allocation && Observe(storage,state).position==2 && storage.Template(1).Section("State").data()==observed.allocation);
}

// 생성/버전0/현재 header 공개 뒤 readonly, +16 초기화 뒤 실제 read, 현재슬롯 재조회 뒤 Title 순서다.
TEST_CASE(CampaignTemplateLoadFile_PublishesManagerBeforeReadAndKeepsActualBytes) {
    CampaignTemplateStorage storage;storage.SerializationVersion()=0x12345678;auto header=storage.Headers().subspan(20,20);Field(header,4,7);Field(header,12,0x55667788);Field(header,16,99);
    auto hooks=Effects(Image());std::string trace;std::uint32_t manager{};std::int32_t types=256;
    hooks.readOnly=[&](auto name) { trace+='R';manager=Value(header);CHECK(name=="fresh.fort" && manager && !Value(header,4) && Value(header,12)==0x55667788 && Value(header,16)==99 && storage.OwnedImages()==1 && storage.SerializationVersion()==0);return true; };
    hooks.read=[&](auto name) { trace+='F';CHECK(name=="fresh.fort" && Value(header)==manager && Value(header,8)==1 && !Value(header,16));types=188;return Image(); };
    hooks.typeCount=[&] { trace+='Y';return types; };hooks.currentSlot=[&] { trace+='I';CHECK(storage.SectionOrder(1).size()==35);return 1U; };
    hooks.refreshTitle=[&] { trace+='T';CHECK(Value(header)==manager && storage.Template(1).Section("State")[1]==5); };
    storage.LoadFile(1,FortTemplate{},"fresh.fort",hooks);CHECK(trace=="RFYIT" && storage.FileHeader(1)==(std::array<std::uint32_t,2>{'F',1}));
    const auto state=storage.FindSection(1,"State");CHECK(storage.Template(1).Section("State").data()==Observe(storage,state).allocation && !storage.State(1).position && storage.State(1).end==3);
    std::uint32_t readOnly{},owner{};std::array<std::uint8_t,9> hidden{};std::vector<std::uint8_t> saved;TemplateFileWriteHooks file;
    file.permissions=[](auto name,auto mode) { CHECK(name=="fresh.fort" && mode==0x180); };file.write=[&](auto,auto bytes) { saved.assign(bytes.begin(),bytes.end()); };
    storage.SaveFile(1,{readOnly,owner,hidden},{[] { return 188U; },[](auto) { throw std::logic_error("예상 밖 이미지 진단"); }},file);CHECK(saved==Image());
}

// stale readonly 비영은 판정을 생략하고1로 쓰며 다른 현재슬롯에는 제목 hook을 요구하지 않는다.
TEST_CASE(CampaignTemplateLoadFile_PresetReadOnlyAndNonCurrentSelectedEffects) {
    CampaignTemplateStorage storage;auto header=storage.Headers().subspan(20,20);Field(header,8,0x87654321);auto hooks=Effects(Image());hooks.readOnly={};hooks.currentSlot=[] { return 2U; };hooks.refreshTitle={};
    storage.LoadFile(1,FortTemplate{},"preset.fort",hooks);CHECK(Value(header) && Value(header,8)==1 && storage.OwnedImages()==1);
}

// 원문 정상 false는 실제 owner를 반납하지만 부가필드와 callback이 수행한 쓰기를 보존한다.
TEST_CASE(CampaignTemplateLoadFile_NormalFalseDestroysOnlyCurrentManager) {
    CampaignTemplateStorage storage;auto header=storage.Headers().subspan(20,20);Field(header,12,0x10203040);auto hooks=Effects({});hooks.typeCount={};hooks.currentSlot={};hooks.refreshTitle={};
    hooks.read=[&](auto) { CHECK(Value(header) && storage.OwnedImages()==1 && !Value(header,16));Field(header,4,17);Field(header,8,3);Field(header,16,19);storage.SerializationVersion()=27;return std::vector<std::uint8_t>{}; };
    storage.LoadFile(1,FortTemplate{},"absent.fort",hooks);CHECK(!Value(header) && Value(header,4)==17 && Value(header,8)==3 && Value(header,12)==0x10203040 && Value(header,16)==19 && storage.OwnedImages()==0 && storage.SerializationVersion()==27);
    CHECK(Rejects([&] { storage.Template(1); }));
}

// 잘못된 서명 요청이 정상 복귀하면 같은 실제 서명 필드를 본 뒤 부모의 정상 false 정리를 따른다.
TEST_CASE(CampaignTemplateLoadFile_RejectedSignatureHasNormalFalseCleanup) {
    CampaignTemplateStorage storage;auto header=storage.Headers().subspan(20,20);auto hooks=Effects({'X',0xfe});std::size_t rejects{};
    hooks.rejectFile=[&](auto name) { ++rejects;CHECK(name=="reject.fort" && Value(header) && storage.FileHeader(1)==(std::array<std::uint32_t,2>{'X',1}));Field(header,12,57);Field(header,16,59); };
    storage.LoadFile(1,FortTemplate{},"reject.fort",hooks);CHECK(rejects==1 && !Value(header) && Value(header,12)==57 && Value(header,16)==59 && !storage.OwnedImages());
}

// 파일 장치/선택된 진단의 throw는 정상 false가 아니며 이미 공개한 manager를 롤백하지 않는다.
TEST_CASE(CampaignTemplateLoadFile_ExceptionKeepsPublishedManagerAndMissingEffects) {
    CampaignTemplateStorage storage;auto header=storage.Headers().subspan(20,20);auto hooks=Effects(Image());hooks.read=[](auto)->std::vector<std::uint8_t> { throw std::runtime_error("명시 파일 장치 중단"); };
    CHECK(Rejects([&] { storage.LoadFile(1,FortTemplate{},"stopped.fort",hooks); }) && Value(header) && storage.OwnedImages()==1 && !Value(header,4) && !Value(header,16));
    const auto manager=Value(header);TemplateFileLoadHooks existing;existing.sameName=[](auto first,auto second) { return first==second; };storage.LoadFile(1,FortTemplate{},"stopped.fort",existing);CHECK(Value(header)==manager && storage.OwnedImages()==1);
    storage.Clear(1);CHECK(!storage.OwnedImages());TemplateFileLoadHooks absent;
    CHECK(Rejects([&] { storage.LoadFile(1,FortTemplate{},"unbound.fort",absent); }) && Value(header) && storage.OwnedImages()==1); // 실제 readonly의 미연결 위치에서만 중단한다.
}

// 첫 섹션 append 뒤의 물리 잘림은 이미 붙인 같은 노드와 pos를 남긴다.
TEST_CASE(CampaignTemplateLoadFile_PartialReadKeepsAttachedActualNode) {
    CampaignTemplateStorage storage;auto hooks=Effects({'F',0xfe,5,0,11,13,17,8,0});auto header=storage.Headers().subspan(20,20);
    CHECK(Rejects([&] { storage.LoadFile(1,FortTemplate{},"partial.fort",hooks); }) && Value(header) && storage.OwnedImages()==1);
    CHECK(storage.SectionOrder(1)==(std::vector<std::string>{"Subscriber"}));const auto stream=storage.FindSection(1,"Subscriber");const auto bytes=storage.Template(1).Section("Subscriber");
    CHECK(stream.section && bytes.size()==3 && bytes[0]==11 && bytes[1]==13 && bytes[2]==17 && Observe(storage,stream).end==3 && !Observe(storage,stream).position);
    storage.Clear(1);CHECK(!Value(header) && storage.OwnedImages()==0);
}

// readonly callback이 header를 다른 실제 owner로 돌리면 그 manager에 원문 병합을 수행한다.
TEST_CASE(CampaignTemplateLoadFile_RereadsRedirectedManagerAndPreservesEmptyExistingSection) {
    CampaignTemplateStorage storage;storage.InstallFile(2,FortTemplate{},Image({21,22,23},{31,32,33,34}),"target.fort");const auto target=Value(storage.Headers().subspan(40,20));
    const auto oldSubscriber=storage.FindSection(2,"Subscriber"),oldState=storage.SelectSection(2,"State",1);storage.State(2).position=2;const auto before=Observe(storage,oldState);auto header=storage.Headers().subspan(20,20);std::uint32_t abandoned{};
    auto hooks=Effects(Image({41,42},{}));hooks.readOnly=[&](auto name) { CHECK(name=="requested.fort");abandoned=Value(header);Field(header,0,target);return false; };
    hooks.read=[&](auto name) { CHECK(name=="target.fort" && Value(header)==target);return Image({41,42},{}); };
    storage.LoadFile(1,FortTemplate{},"requested.fort",hooks);CHECK(abandoned && abandoned!=target && Value(header)==target && storage.OwnedImages()==2 && &storage.Template(1)==&storage.Template(2));
    const auto retained=storage.FindSection(1,"State");CHECK(retained.section==oldState.section && retained.generation==oldState.generation && Observe(storage,retained).allocation==before.allocation && Observe(storage,retained).position==2);
    CHECK(storage.Template(2).Section("State").size()==4 && storage.Template(2).Section("State")[0]==31 && storage.Template(1).Section("Subscriber").size()==2 && storage.Template(1).Section("Subscriber")[0]==41);
    CHECK(Rejects([&] { storage.AccessSection(oldSubscriber,[](auto,const auto&) {}); }));CHECK(storage.SectionOrder(1).size()==35);Field(header,0,abandoned);storage.Clear(1);storage.Clear(2);CHECK(!storage.OwnedImages()); // 명시로보관한원래owner도검사끝에반납한다.
}

// 읽기 callback 뒤 header가0이면 성공한 manager를 유지해도 제목/현재슬롯을 선행 조회하지 않는다.
TEST_CASE(CampaignTemplateLoadFile_PostReadNullHeaderSkipsTitleAndKeepsOwner) {
    CampaignTemplateStorage storage;auto header=storage.Headers().subspan(20,20);auto hooks=Effects(Image());std::uint32_t saved{};
    hooks.read=[&](auto) { saved=Value(header);Field(header,0,0);return Image(); };hooks.currentSlot={};hooks.refreshTitle={};
    storage.LoadFile(1,FortTemplate{},"detached.fort",hooks);CHECK(saved && !Value(header) && storage.OwnedImages()==1);Field(header,0,saved);CHECK(storage.SectionOrder(1).size()==35);storage.Clear(1);CHECK(!storage.OwnedImages());
}
