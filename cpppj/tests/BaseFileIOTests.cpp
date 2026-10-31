// 실제 BaseFile의 위치/길이/소멸/암호 쓰기를 독립 PE 호출·전체 BYTE와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBaseFileIO.h"
#include "client/ObjectDumpDisk.h"
#include "o/BaseFile.h"

namespace { // fixture 입력 장치만 구성하며 기대 분기는 원문 출력에서 읽는다.
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
constexpr std::uintptr_t kFile=0x12200000U,kKey=0x12300000U; // 격리 PE가 사용하는 명시 FILE/키 토큰이다.
// 원문 현재 필드와 같은 CRT 입력·콜백 변이를 재생한다.
struct IOScene {
    RawBaseFileState state; // 이 사례의 실제 여섯 필드이며 콜백과 제품 몸체가 함께 쓴다.
    RawBaseFileIOHooks hooks; // 원문 CRT/진단 경계에 대응하는 명시 장치다.
    std::uint32_t position{},end{},fileFlags{},scenario{},failed{}; // 실제 FILE 위치/끝/표시와 콜백 입력이다.
    std::string events; // 원문과 같은 호출 인자 및 전체 쓰기 BYTE다.
    std::vector<std::uint8_t> input; // 원본은 수정하지 않는 쓰기 입력이다.
    // 뒤쪽 의미 없는 필드까지 원시 입력 그대로 보관한다.
    explicit IOScene(const std::vector<std::string>& row) {
        state={Number(row[1]),Number(row[2]),Signed(Number(row[3])),Signed(Number(row[4])),Number(row[5]),Number(row[6])};
        position=Number(row[7]);end=Number(row[8]);fileFlags=Number(row[9]);scenario=Number(row[13]);failed=Number(row[14]);input.resize(Number(row[12]));
        for (std::size_t index=0;index<input.size();++index) input[index]=static_cast<std::uint8_t>(index*29+7); // PE와 같은 명시 입력이며 출력에서 역산하지 않는다.
        hooks.seek=[this](auto file,auto offset,auto origin) { Event("S:"+std::to_string(file)+':'+std::to_string(static_cast<std::uint32_t>(offset))+':'+std::to_string(origin));if (!failed) position=static_cast<std::uint32_t>(offset)+(origin==1 ? position : origin==2 ? end : 0U); };
        hooks.tell=[this](auto file) { Event("T:"+std::to_string(file));if (scenario==2) { state.flags^=1U;state.offset=9;state.length=11; }return Signed(position); };
        hooks.eofFlags=[this](auto) { return fileFlags; };
        hooks.write=[this](auto file,auto bytes,auto size,auto count) { CHECK(bytes.size()==static_cast<std::size_t>(size)*count);Event("W:"+std::to_string(file)+':'+std::to_string(size)+':'+std::to_string(count)+':'+(bytes.empty()?"-":Hex(bytes)));position+=failed ? size*count/2 : size*count; };
        hooks.close=[this](auto file) { Event("C:"+std::to_string(file));if (scenario==1) state={kFile+64,255,99,100,kKey+128,5}; };
        hooks.freeKey=[this](auto key) { Event("F:"+std::to_string(key)); };
        hooks.key=[](auto key,auto length)->std::span<const std::uint8_t> { static constexpr std::array<std::uint8_t,3> first{'a','b','c'};static constexpr std::array<std::uint8_t,5> second{'v','w','x','y','z'}; // 두 실제 할당의 입력 BYTE다.
            if (key==kKey && length<=first.size()) return first;if (key==kKey+128 && length<=second.size()) return second;throw std::logic_error("명시 키 토큰/물리 범위 오류"); };
        hooks.assertion=[this](auto line) { Event("A:"+std::to_string(line));if (scenario==4 && line==0x3aa) state.keyLength=3;if (scenario==5 && line==0x3a4) state.flags&=~1U; };
    }
    static std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } // 원시 DWORD를 signed long으로 읽는다.
    void Event(std::string text) { if (!events.empty()) events+=';';events+=text; } // 원문 호출 순서대로 하나의 열에 추가한다.
    std::string Fields() const { return std::to_string(state.file)+','+std::to_string(state.flags)+','+std::to_string(static_cast<std::uint32_t>(state.offset))+','+std::to_string(static_cast<std::uint32_t>(state.length))+','+std::to_string(state.key)+','+std::to_string(state.keyLength); } // 최종 여섯 필드만 직렬화한다.
    std::string Run(const std::vector<std::string>& row) { // 미선택 CRT 장치를 추가로 호출하지 않고 실제 본체 하나만 실행한다.
        const auto& op=row[0];const auto offset=Signed(Number(row[10]));
        if (op=="seek") SeekBaseFile(state,offset,Signed(Number(row[11])),hooks);
        else if (op=="eof") return std::to_string(BaseFileEof(state,hooks));
        else if (op=="length") return std::to_string(static_cast<std::uint32_t>(BaseFileLength(state,hooks)));
        else if (op=="close") CloseBaseFile(state,hooks);
        else if (op=="destruct") DestructBaseFile(state,hooks);
        else if (op=="write") WriteBaseFile(state,input,offset,hooks);
        else throw std::logic_error("명시 BaseFile 검사 종류 오류");return "-";
    }
};
}

// EOF의 엄격 비교/부분 파일 끝 seek 부호/전체 XOR BYTE/실패 후 계속/현재 필드 재조회를 비교한다.
TEST_CASE(BaseFileIO_IndependentNative322CursorCloseAndChunkedWrite) {
    const auto rows=LoadFixture(NETSTORM_BASEFILE_IO_FIXTURE).rows;CHECK(rows.size()==322);std::size_t index{};
    for (const auto& row:rows) { // 원문의 최초 관측만 기대값이며 제품 실행 전에는 결과를 계산하지 않는다.
        IOScene scene(row);const auto original=scene.input;const auto before=netstorm::test::FailureCount();
        CHECK(scene.Run(row)==row[15]);CHECK(scene.Fields()==row[16]);CHECK(std::to_string(scene.position)==row[17]);CHECK((scene.events.empty()?"-":scene.events)==row[18]);CHECK(scene.input==original);
        if (netstorm::test::FailureCount()!=before) std::fprintf(stderr,"BaseFileIO fixture row=%zu op=%s\n",index,row[0].c_str());++index;
    }
}

// 미선택 장치를 요구하지 않으며 예외 뒤 앞선 호출/필드 효과를 복구하지 않는다.
TEST_CASE(BaseFileIO_MissingBoundariesAndCloseKeepEarlierEffects) {
    RawBaseFileState state{0,0x81,7,9,0,3};CloseBaseFile(state,{});CHECK(state.flags==0x81 && state.offset==7 && state.length==9);
    SeekBaseFile(state,1,3,{});CHECK(BaseFileLength(state,{})==9); // 부분 파일의 미지원 origin은 자연 생략이다.
    state.file=kFile;RawBaseFileIOHooks hooks;hooks.close=[&](auto file) { CHECK(file==kFile);state.flags=0x82;throw std::runtime_error("명시 fclose 중단"); };
    bool threw{};try { DestructBaseFile(state,hooks); }catch (const std::runtime_error&) { threw=true; }CHECK(threw && state.file==kFile && state.flags==0x82 && state.offset==7);
    state={kFile,0x80,7,9,kKey,0};std::size_t calls{};hooks.assertion=[&](auto line) { CHECK(line==0x3aa);++calls; };hooks.tell=[&](auto) { ++calls;return 15; };hooks.key=[](auto,auto) { return std::span<const std::uint8_t>{}; };
    const std::array<std::uint8_t,1> data{4};threw=false;try { WriteBaseFile(state,data,-1,hooks); }catch (const std::out_of_range&) { threw=true; }CHECK(threw && calls==2 && state.keyLength==0);
}

// 실제 제품의 객체 덤프 장치가 같은 쓰기/소멸 몸체를 사용하며 한글·보충 평면 경로도 보존한다.
TEST_CASE(BaseFileIO_RealObjectDumpDiskUsesSharedWriteAndDestruct) {
    using namespace netstorm::client;const std::filesystem::path root=NETSTORM_BASEFILE_IO_WORK;std::filesystem::create_directories(root);
    for (const auto& leaf:{std::filesystem::path("ascii"),std::filesystem::path(u8"한글-😀")}) { // 각 새 작업 폴더에서 w+b/빈 쓰기/추가 쓰기/닫기를 확인한다.
        const auto directory=root/leaf;std::filesystem::create_directories(directory);const auto hooks=MakeObjectDumpDiskHooks(directory,[](auto) {},[] {});
        std::array<std::uint8_t,24> file{};hooks.construct(file);const auto path=hooks.path();hooks.openWrite(file,path,2,0);static_cast<void>(hooks.write(file,{},0xffffffffU));
        std::vector<std::uint8_t> bytes(8201);for (std::size_t index=0;index<bytes.size();++index) bytes[index]=static_cast<std::uint8_t>(index*17+3); // 원문 변환과 무관한 디스크 입력 전체다.
        static_cast<void>(hooks.write(file,bytes,0xffffffffU));const std::array<std::uint8_t,3> tail{0,255,7};static_cast<void>(hooks.write(file,tail,0xffffffffU));hooks.destruct(file);
        bytes.insert(bytes.end(),tail.begin(),tail.end());CHECK(ReadFileBytes(directory/"squids.txt")==bytes);
    }
}
