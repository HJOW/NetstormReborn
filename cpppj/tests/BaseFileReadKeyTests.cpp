// 생성/열기/키/부분 읽기의 독립 PE 결과와 실제 디스크·TAFF 소비자를 함께 확인한다.
#include "RawSceneSupport.h"
#include "o/RawBaseFileIO.h"
#include "o/BaseFile.h"
#include <array>
#include <optional>
#include <new>

namespace { // 원문에 제공한 장치 입력과 실제 작성 BYTE만 재생한다.
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
constexpr std::uintptr_t kReadFile=0x12200000U,kReadKey=0x12300000U,kNewReadKey=kReadKey+0x4000U; // 관측기의 서로 겹치지 않는 실제 입력 토큰이다.
std::int32_t ReadSigned(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원문 signed long으로 비트를 해석한다.
std::vector<std::uint8_t> ReadUnhex(std::string_view text) { // 입력의 전체 BYTE와 첫 NUL 뒤의 데이터도 보존한다.
    std::vector<std::uint8_t> bytes;for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));return bytes;
}
std::string ReadHex(std::span<const std::uint8_t> bytes) { return bytes.empty()?"-":Hex(bytes); } // 빈 원시 물리만 fixture의 대시로 표시한다.
// 실제 필드와 CRT 입력/부분 작성/콜백 변이를 한 사례 안에서 공유한다.
struct ReadKeyScene {
    RawBaseFileState state; // 제품과 장치 콜백이 함께 읽고 쓰는 현재 여섯 필드다.
    RawBaseFileIOHooks hooks; // 원문 CRT/할당/진단과 대응하는 명시 경계다.
    std::uint32_t position{},available{},mask{},scenario{},opens{}; // 현재 FILE 위치/공급량과 열기·콜백 입력이다.
    std::uintptr_t initialKey{}; // 읽기 시작 때의 작업 버퍼 사용 관문을 결과 표시용으로 보관한다.
    std::vector<std::uint8_t> destination,allocation; // 원문의 출력과 새 키 할당 전체다.
    std::array<std::uint8_t,4096> scratch{}; // 명시 초기 BYTE를 가진 같은 작업 버퍼이며 묶음 사이에 초기화하지 않는다.
    std::array<std::uint8_t,32> source{}; // NULL과 별개인 키 입력 물리이며 콜백도 같은 BYTE를 변경한다.
    std::string events; // 현재 호출 인자와 순서를 기록한다.
    explicit ReadKeyScene(const std::vector<std::string>& row) { // 결과를 계산하지 않고 원시 입력만 설치한다.
        state={Number(row[1]),Number(row[2]),ReadSigned(Number(row[3])),ReadSigned(Number(row[4])),Number(row[5]),Number(row[6])};initialKey=state.key;position=Number(row[7]);available=Number(row[8]);mask=Number(row[14]);scenario=Number(row[15]);destination.resize(Number(row[10]));
        for (std::size_t index=0;index<destination.size();++index) destination[index]=static_cast<std::uint8_t>(index*7+53); // 원문 출력 물리의 명시 초기값이다.
        for (std::size_t index=0;index<scratch.size();++index) scratch[index]=static_cast<std::uint8_t>(index*11+165); // 덜 읽힌 BYTE의 비교 근거가 되는 입력이다.
        if (row[11]!="null") { const auto bytes=ReadUnhex(row[11]);std::copy(bytes.begin(),bytes.end(),source.begin()); }
        hooks.open=[this](auto name,auto mode)->std::uintptr_t { Event("O:"+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()})+':'+std::string(mode));if (scenario==1) { state.flags=255;state.offset=19;state.length=23; }return mask&(1U<<opens++) ? kReadFile:0U; };
        hooks.seek=[this](auto file,auto offset,auto origin) { Event("S:"+std::to_string(file)+':'+std::to_string(static_cast<std::uint32_t>(offset))+':'+std::to_string(origin));position=static_cast<std::uint32_t>(offset);if (scenario==2) state.file=0; };
        hooks.tell=[this](auto file) { Event("T:"+std::to_string(file));if (scenario==2) { state.flags^=1U;state.offset=9; }return ReadSigned(position); };
        hooks.read=[this](auto file,auto bytes,auto size,auto count) { const auto read=std::min(size*count,available);CHECK(bytes.size()==static_cast<std::size_t>(size)*count);
            for (std::size_t index=0;index<read;++index) bytes[index]=static_cast<std::uint8_t>((position+index)*29+7); // CRT가 실제 작성한 앞부분만 변경한다.
            Event("R:"+std::to_string(file)+':'+std::to_string(size)+':'+std::to_string(count)+':'+std::to_string(read));available-=read;position+=read;if (scenario==3) { state.file=kReadFile+64;state.key=kReadKey+128;state.keyLength=5; }
        };
        hooks.freeKey=[this](auto key) { Event("F:"+std::to_string(key));if (scenario==1) { state.key=kReadKey+128;state.keyLength=99;source[0]='z';source[1]=0; } };
        hooks.allocateKey=[this](auto length)->std::uintptr_t { Event("N:"+std::to_string(length));allocation.assign(length,205);if (scenario==2) source[1]=0;return kNewReadKey; };
        hooks.mutableKey=[this](auto key,auto length)->std::span<std::uint8_t> { CHECK(key==kNewReadKey && length==allocation.size());return allocation; };
        hooks.key=[](auto key,auto length)->std::span<const std::uint8_t> { static constexpr std::array<std::uint8_t,3> first{'a','b','c'};static constexpr std::array<std::uint8_t,5> second{'v','w','x','y','z'}; // 두 명시 읽기 키다.
            if (key==kReadKey && length<=first.size()) return first;if (key==kReadKey+128 && length<=second.size()) return second;throw std::logic_error("명시 읽기 키 토큰/물리 오류"); };
        hooks.assertion=[this](auto line) { Event("A:"+std::to_string(line));if (scenario==4 && line==0x38b) state.keyLength=3; };
    }
    void Event(std::string text) { if (!events.empty()) events+=';';events+=text; } // 원문 호출 순서를 보존한다.
    std::string Fields() const { return std::to_string(state.file)+','+std::to_string(state.flags)+','+std::to_string(static_cast<std::uint32_t>(state.offset))+','+std::to_string(static_cast<std::uint32_t>(state.length))+','+std::to_string(state.key)+','+std::to_string(state.keyLength); } // 마지막 여섯 필드만 저장한다.
    std::string Run(const std::vector<std::string>& row) { // 이 입력이 선택한 제품 몸체 하나만 호출한다.
        const auto& op=row[0];const auto offset=ReadSigned(Number(row[9]));const std::string_view name("asset\0tail",10);
        if (op=="construct") ConstructBaseFile(state);
        else if (op=="disk") return std::to_string(OpenBaseFileDisk(state,name,Number(row[12]),Number(row[13]),hooks));
        else if (op=="subfile") return std::to_string(OpenBaseFileSubfile(state,name,offset,33,hooks));
        else if (op=="key") SetBaseFileKey(state,row[11]=="null" ? std::nullopt:std::optional<std::span<const std::uint8_t>>(source),hooks);
        else if (op=="read") ReadBaseFile(state,destination,offset,hooks,scratch);
        else throw std::logic_error("명시 BaseFile 읽기 검사 종류 오류");return "-";
    }
};
}

// 전체 출력/작업 버퍼와 키 복사 뒤 미작성 tail을 원문의 최초 관측과 비교한다.
TEST_CASE(BaseFileReadKey_IndependentNative235FullBuffersAndCurrentFields) {
    const auto rows=LoadFixture(NETSTORM_BASEFILE_READ_KEY_FIXTURE).rows;CHECK(rows.size()==235);std::size_t index{};
    for (const auto& row:rows) { // 암호 부분 읽기의 짧은 공급량과 연속 묶음도 같은 현재 작업 물리를 사용한다.
        ReadKeyScene scene(row);const auto before=netstorm::test::FailureCount();CHECK(scene.Run(row)==row[16]);CHECK(scene.Fields()==row[17]);CHECK(std::to_string(scene.position)==row[18]);CHECK((scene.events.empty()?"-":scene.events)==row[19]);CHECK(ReadHex(scene.destination)==row[20]);CHECK((row[0]=="read" && scene.initialKey && !scene.destination.empty()?Hex(scene.scratch):"-")==row[21]);CHECK(ReadHex(scene.allocation)==row[22]);CHECK(Hex(scene.source)==row[23]);
        if (netstorm::test::FailureCount()!=before) std::fprintf(stderr,"BaseFileReadKey row=%zu op=%s\n",index,row[0].c_str());++index;
    }
}

// 호스트 물리 부족/할당 실패를 빈 성공으로 바꾸지 않고 앞선 원문 효과를 보존한다.
TEST_CASE(BaseFileReadKey_HostBoundariesKeepEarlierKeyAndReadEffects) {
    RawBaseFileState state{kReadFile,0x81,7,9,kReadKey,3};RawBaseFileIOHooks hooks;std::size_t freed{};hooks.freeKey=[&](auto key) { CHECK(key==kReadKey);++freed; };hooks.allocateKey=[](auto length)->std::uintptr_t { CHECK(length==4);return 0; };
    const std::array<std::uint8_t,4> key{'a','b','c',0};bool threw{};try { SetBaseFileKey(state,std::span<const std::uint8_t>(key),hooks); }catch (const std::bad_alloc&) { threw=true; }CHECK(threw && freed==1 && state.key==0 && state.keyLength==3 && state.file==kReadFile);
    state.key=kReadKey;hooks.tell=[](auto) { return 15; };std::array<std::uint8_t,1> bytes{88};threw=false;try { ReadBaseFile(state,bytes,-1,hooks); }catch (const std::out_of_range&) { threw=true; }CHECK(threw && bytes[0]==88);
}

// 새 실제 디스크/TAFF 경로에서4096 경계와 빈 파일·Unicode·엔트리별 키 시작을 확인한다.
TEST_CASE(BaseFileReadKey_ActualDiskAndEncryptedArchiveConsumers) {
    const std::filesystem::path root=NETSTORM_BASEFILE_READ_KEY_WORK;const auto directory=root/std::filesystem::path(u8"한글-😀");std::filesystem::create_directories(directory/"d");
    std::vector<std::uint8_t> first(8201),second(4097);for (std::size_t index=0;index<first.size();++index) first[index]=static_cast<std::uint8_t>(index*17+3); // 기대 평문은 두 독립 입력 패턴이다.
    for (std::size_t index=0;index<second.size();++index) second[index]=static_cast<std::uint8_t>(index*19+11); // 두 번째 엔트리는 키가 다시0에서 시작해야 한다.
    { std::ofstream output(directory/"d"/"plain.bin",std::ios::binary);output.write(reinterpret_cast<const char*>(first.data()),static_cast<std::streamsize>(first.size()));CHECK(static_cast<bool>(output)); }
    { std::ofstream output(directory/"d"/"empty.bin",std::ios::binary);CHECK(static_cast<bool>(output)); }
    const std::array<std::string,2> names{"\\d\\first.bin","\\d\\second.bin"};const auto record=8+names[0].size()+1,table=record+8+names[1].size()+1,dataOffset=0x34+table;std::vector<std::uint8_t> archive(dataOffset+first.size()+second.size());
    const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),archive.begin());Put(archive,0x14,2);Put(archive,0x18,2);Put(archive,0x1c,0x2c);Put(archive,0x20,0x34);Put(archive,0x24,static_cast<std::uint32_t>(table));Put(archive,0x28,static_cast<std::uint32_t>(dataOffset));Put(archive,0x2c,0);Put(archive,0x30,static_cast<std::uint32_t>(record));
    Put(archive,0x34,0);Put(archive,0x38,static_cast<std::uint32_t>(first.size()));std::copy(names[0].begin(),names[0].end(),archive.begin()+0x3c);Put(archive,0x34+record,static_cast<std::uint32_t>(first.size()));Put(archive,0x38+record,static_cast<std::uint32_t>(second.size()));std::copy(names[1].begin(),names[1].end(),archive.begin()+static_cast<std::ptrdiff_t>(0x3c+record));
    auto encryptedFirst=first,encryptedSecond=second;ApplyXor(encryptedFirst);ApplyXor(encryptedSecond);std::copy(encryptedFirst.begin(),encryptedFirst.end(),archive.begin()+static_cast<std::ptrdiff_t>(dataOffset));std::copy(encryptedSecond.begin(),encryptedSecond.end(),archive.begin()+static_cast<std::ptrdiff_t>(dataOffset+first.size()));
    const auto path=directory/"resources.tarc";{ std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(archive.data()),static_cast<std::streamsize>(archive.size()));CHECK(static_cast<bool>(output)); }
    BaseFileSystem files(directory);CHECK(files.RegisterArchive(path));CHECK(files.Read("d/plain.bin")==first && files.Read("d/empty.bin").empty());CHECK(files.Read("d/first.bin")==first && files.Read("d/second.bin")==second);CHECK(!files.TryRead("d/missing.bin"));CHECK(ReadFileBytes(path)==archive);
}
