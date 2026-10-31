// 기존 독립 PE의 실제 저장 BYTE와 효과/전체 슬롯 관측으로 새 스트림 경로를 대조한다.
#include "o/RawFortObjectStream.h"
#include "o/RawFortContentsLoad.h"

namespace { // 명시 파일/메모리 경계만 공급하며 원본의 생성/분기 결과는 계산하지 않는다.
struct ObjectStreamMemory { // 독립 MemStream의 물리16BYTE와 입력 BYTE를 함께 소유한다.
    std::array<std::uint8_t,16> fields{}; // data 토큰1/pos/end/alloc이며 callback 사이에도 같은 주소다.
    std::vector<std::uint8_t> bytes; // 논리 end 밖 물리 영역도 명시 입력으로 유지할 수 있다.
    explicit ObjectStreamMemory(std::vector<std::uint8_t> value):bytes(std::move(value)) { Put(fields,0,1);Put(fields,8,static_cast<std::uint32_t>(bytes.size()));Put(fields,12,static_cast<std::uint32_t>(bytes.size())); } // 실제 읽기 결과는 미리 쓰지 않는다.
    void Access(const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>& operation) { // 매 읽기에서 현재 fields/버퍼를 빌린다.
        MemStreamHooks memory;memory.buffer=[this](auto token)->std::span<std::uint8_t> { CHECK(token==1);return bytes; };operation(fields,memory);
    }
};
std::vector<std::uint8_t> ObjectStreamHex(std::string_view text) { // 저장된 원문 패킷 hex만 BYTE로 복원한다.
    CHECK(text.size()%2==0);std::vector<std::uint8_t> bytes;
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return bytes; // 기대 효과는 읽지 않는다.
}
FortObjectStreamHooks ObjectStreamDevices(FortLoadScene& scene,std::vector<std::unique_ptr<ObjectStreamMemory>>& memory,std::uint32_t& conversion,std::array<std::uint8_t,255>& table,const FortObjectLoadHooks& effects) { // 같은 기존 원문 장면의 실제 효과와 직접 Contents reader를 조립한다.
    FortObjectStreamHooks hooks;hooks.access=[&memory](auto token,const auto& operation) { if (!token || token>memory.size()) throw std::out_of_range("원문 Objects 검사 stream 범위");memory[token-1]->Access(operation); };
    hooks.manager=[](auto) { return 0x1111U; };hooks.select=[&memory](auto manager,auto name,auto writable) { CHECK(manager==0x1111U && writable==1);const auto index=name=="Chaff" ? 20U : static_cast<unsigned>(std::stoul(std::string(name.substr(4))));CHECK(index<memory.size());return index+1U; };
    hooks.prepareTypes=[](auto) {}; // 기존 Native fixture도004bd800을 명시 외부 경계로 두었다. 새 실제 준비 완료로 세지 않는다.
    hooks.conversion=[&conversion] { return conversion; };hooks.checking=[] { return 0U; };hooks.conversionBuffer=[&table](auto token)->std::span<const std::uint8_t> { CHECK(token==1);return table; };hooks.releaseConversion=[&conversion] { conversion=0; };
    hooks.assertion=[](auto line) { throw std::logic_error("Objects 원문 검사 예상 밖 진단:"+std::to_string(line)); };
    hooks.contents=[&scene,&conversion,&table,effects,access=hooks.access](Sid parent,auto stream) {
        FortContentsLoadHooks reader;reader.access=access;reader.conversionBuffer=[&table](auto token)->std::span<const std::uint8_t> { CHECK(token==1);return table; };
        reader.type=[&scene](auto type) { return FortContentsLoadType{scene.types.at(type).flags1,scene.types.at(type).contentListFlags}; };reader.parentListFlags=[&scene](auto sid) { return scene.types.at(scene.pool.Slot(Sid{static_cast<std::uint16_t>(sid)})[10]).containerListFlags; };
        reader.create=[effects](auto sid,auto type,auto quantity,auto list,auto flags,auto quality) { return std::uint32_t(effects.createContent(Sid{static_cast<std::uint16_t>(sid)},type,quantity,list,flags,quality).value); };
        reader.assertion=[](auto) { throw std::logic_error("Objects 원문 Contents 예상 밖 진단"); };static_cast<void>(LoadFortContents(parent.value,stream,9,0,{conversion,[] { return 0U; }},reader));
    };return hooks;
}
}
TEST_CASE(FortObjectStream_ReplaysIndependentPatch609ObjectsAndParent) { //573개 객체와36개 전체 부모를 동일 보호 기대값으로 대조한다.
    std::size_t checked{};for (const auto& row:Fixture()) { // 각 입력은 실제 새 풀/stream/현재 전역을 사용한다.
        if (row[0]!="originals") continue;FortLoadScene scene(OriginalEdition::Patch1078,Inputs(row[2]));const auto effects=scene.Hooks();RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,effects);
        std::vector<std::unique_ptr<ObjectStreamMemory>> memory;std::uint32_t conversion{};std::array<std::uint8_t,255> table{};
        for (std::size_t index=0;index<table.size();++index) table[index]=static_cast<std::uint8_t>(index); // PE 실행기에 제공한 identity 변환표 입력이다.
        if (row[1]=="O") { memory.push_back(std::make_unique<ObjectStreamMemory>(ObjectStreamHex(row[3])));if (scene.v[Stored]<246 && scene.v[Stored]!=scene.v[FortTypeInput]) { conversion=1;table[scene.v[Stored]]=static_cast<std::uint8_t>(scene.v[FortTypeInput]); } }
        else for (std::size_t index=0;index<21;++index) { // 원문 실행기의20 Terr와Chaff 저장 입력이며 정책에 따른 소비량은 로더가 정한다.
            std::vector<std::uint8_t> bytes{0};if (index==0 || index==1 || index==20) { bytes={2};for (int chunk=0;chunk<4;++chunk) { bytes.insert(bytes.end(),{99,1,0,0x21,static_cast<std::uint8_t>(scene.v[Stored]),static_cast<std::uint8_t>(scene.v[OwnerInput])}); } }memory.push_back(std::make_unique<ObjectStreamMemory>(std::move(bytes)));
        }
        const auto hooks=ObjectStreamDevices(scene,memory,conversion,table,effects);
        if (row[1]=="O") static_cast<void>(loader.ObjectStream(1,std::bit_cast<std::int32_t>(scene.v[OriginX]),std::bit_cast<std::int32_t>(scene.v[OriginY]),scene.v[DefaultOwner],hooks));else loader.LoadStream(static_cast<std::int32_t>(scene.v[Slot]),hooks);
        const auto state=std::to_string(scene.state.version)+','+std::to_string(scene.state.loadingDepth)+','+std::to_string(scene.state.scope)+','+std::to_string(scene.state.fortDestroyBlocked);
        const bool same=scene.Events()==row[4] && scene.Raws()==row[5] && std::to_string(scene.random.State())==row[6] && state==row[7];CHECK(same);if (!same) { std::fprintf(stderr,"Fort ObjectStream Native row=%zu mode=%s\n",checked,row[1].c_str());break; }++checked;
    }CHECK(checked==609);
}
TEST_CASE(FortObjectStream_TruncatedSecondObjectKeepsFirstAndPartialSecond) { // 뒤쪽 파일 오류가 선행 객체와 이미 증가한 cursor를 지우지 않는지 확인한다.
    auto values=Inputs(Fixture().front()[2]);FortLoadScene scene(OriginalEdition::Patch1078,values);const auto effects=scene.Hooks();RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,effects);
    auto bytes=ObjectStreamHex(Fixture().front()[3]);bytes.insert(bytes.end(),{0x21,100});std::vector<std::unique_ptr<ObjectStreamMemory>> memory;memory.push_back(std::make_unique<ObjectStreamMemory>(bytes));std::uint32_t conversion{};std::array<std::uint8_t,255> table{};const auto hooks=ObjectStreamDevices(scene,memory,conversion,table,effects);
    static_cast<void>(loader.ObjectStream(1,16,32,1,hooks));CHECK(scene.born==1 && Read(memory[0]->fields,4,4)==3);bool failed{};try { static_cast<void>(loader.ObjectStream(1,16,32,1,hooks)); }catch (const std::out_of_range&) { failed=true; }
    CHECK(failed && scene.born==2 && Read(memory[0]->fields,4,4)==bytes.size());CHECK(scene.pool.Slot(Sid{100})[34]==1 && scene.pool.Slot(Sid{101})[10]==100 && scene.state.loadingDepth==3); // 실패한 두 번째 생성도 원문의 실제 부분 효과다.
}
TEST_CASE(FortObjectStream_ParentFailureKeepsConversionAndOnlyUnwindsDepth) { // 원문 예외 helper는 깊이만 감소하고 정상 꼬리의 free/후처리는 실행하지 않는다.
    auto values=Inputs(Fixture().front()[2]);values[Flags2]=0x40000U;values[Mission]=1;values[Qa]=0;FortLoadScene scene(OriginalEdition::Patch1078,values);scene.state.chunksX=scene.state.chunksY=1;
    auto effects=scene.Hooks();std::size_t finished{};effects.finishOwner=[&](auto) { ++finished; };RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,effects);
    std::vector<std::unique_ptr<ObjectStreamMemory>> memory;memory.push_back(std::make_unique<ObjectStreamMemory>(std::vector<std::uint8_t>{2,99,2,0,0x21,100,2,0x21,100})); // 두 번째 owner BYTE만 없는 실제 저장 입력이다.
    std::uint32_t conversion{};std::array<std::uint8_t,255> table{};for (std::size_t index=0;index<table.size();++index) table[index]=static_cast<std::uint8_t>(index); // 준비 하위가 제공할 물리 변환표 입력이다.
    auto hooks=ObjectStreamDevices(scene,memory,conversion,table,effects);hooks.prepareTypes=[&](auto slot) { CHECK(slot==1 && scene.state.loadingDepth==4);conversion=1; };std::size_t released{};hooks.releaseConversion=[&] { ++released;conversion=0; };
    bool failed{};try { loader.LoadStream(1,hooks); }catch (const std::out_of_range&) { failed=true; }
    CHECK(failed && scene.born==2 && !finished && !released && conversion==1 && scene.state.loadingDepth==3 && scene.state.scope==1);CHECK(Read(memory[0]->fields,4,4)==9 && scene.pool.Slot(Sid{100})[34]==2 && scene.pool.Slot(Sid{101})[10]==100);
}
