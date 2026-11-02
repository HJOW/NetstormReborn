// 원문 전체 열기 부모의 실제필드/모든장치인자/고리와 VFS 실제TAFF/디스크 수명을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawBaseFileOpenPolicy.h"
#include "o/BaseFile.h"
#include "o/RawColorMapCache.h"
#include "platform/Utf8Path.h"
#include <bit>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <new>

namespace { // 최초 CPU 출력은 기대값으로만 읽고 제품 장치 입력은 각 행의 입력열에서만 구성한다.
using namespace netstorm::o;using namespace netstorm::test::rawscene; // 같은 실제 FILE/고리 타입과 원시 fixture 해석이다.
constexpr std::uintptr_t kFile=0x12200000U,kOldKey=0x12300000U,kNewKey=0x12304000U; // Native의 격리 FILE/이전키/새할당 토큰과 같은DWORD다.
struct OpenPolicyFailure {}; // 원문 예외객체 생성 진입에 대응하는 집중 검사 경계다.
std::string Bytes(std::string_view hex) { // NUL 뒤 입력도 삭제하지 않고 실제 물리로 되돌린다.
    std::string result;if (hex=="-") return result;for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16));return result; // 두16진수마다 한BYTE다.
}
std::string HexText(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 원문 C문자열의 인코딩을 재해석하지 않는다.
std::string JoinEvents(const std::vector<std::string>& events) { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result.empty()?"-":result; } // 실제 사건열을 Native와 같은 순서로 직렬화한다.
std::filesystem::path OpenWork() { return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-basefile-open-757/tests"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()); } // 현재 검사만 새로 생성할 보호폴더 밖의 고유 경로다.
void WriteOpenFile(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) { std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));CHECK(file.good()); } // scratch 입력만 작성하고 원본/기존fixture는 받지 않는다.
std::vector<std::uint8_t> OpenTaff() { // 한 항목의 실제 TAFF 헤더/인덱스/디렉터리/암호 payload 입력이다.
    std::vector<std::uint8_t> bytes(0x44);const std::string magic="TAFF v0.2\x1a";std::copy(magic.begin(),magic.end(),bytes.begin());Put(bytes,0x14,1);Put(bytes,0x18,2);Put(bytes,0x1c,0x2c);Put(bytes,0x20,0x30);Put(bytes,0x24,0x10);Put(bytes,0x28,0x40);Put(bytes,0x2c,0);Put(bytes,0x30,0);Put(bytes,0x34,4);const std::string name="\\d\\a";std::copy(name.begin(),name.end(),bytes.begin()+0x38);
    for (unsigned index=0;index<4;++index) bytes[0x40+index]=static_cast<std::uint8_t>("ABCD"[index]^"mydoghasfleas"[index]); // 독立 입력4BYTE만 고정키로 암호화한다.
    return bytes;
}
struct OpenDisk { // 실제 FILE과 할당키를 raw 상태의 현재 토큰에 연결한 검사 소유자다.
    RawBaseFileState state{};RawBaseFileIOHooks io;std::vector<std::uint8_t> key; // 같은 여섯 필드/CRT 장치/실제 키 할당이다.
    OpenDisk() { // OS만 실제 wide CRT이며 분기/부분파일/키 처리는 제품 공통 몸체를 쓴다.
        ConstructBaseFile(state);io.open=[](auto name,auto mode) { const auto path=netstorm::platform::PathFromUtf8(name);const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str())); };
        io.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); };io.seek=[](auto token,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(token),offset,origin)); };io.tell=[](auto token) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(token))); };
        io.read=[](auto token,auto data,auto size,auto count) { static_cast<void>(std::fread(data.data(),size,count,reinterpret_cast<FILE*>(token))); };io.write=[](auto token,auto data,auto size,auto count) { static_cast<void>(std::fwrite(data.data(),size,count,reinterpret_cast<FILE*>(token))); };
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return kNewKey; };io.mutableKey=[this](auto token,auto size) { CHECK(token==kNewKey && size<=key.size());return std::span(key).first(size); };io.key=[this](auto token,auto size) { CHECK(token==kNewKey && size<=key.size());return std::span<const std::uint8_t>(key).first(size); };io.freeKey=[this](auto token) { CHECK(token==kNewKey);key.clear(); };
    }
    ~OpenDisk() { DestructBaseFile(state,io); } // 부분 열기/할당 예외에서도 현재 FILE/키를 한 번 반납한다.
};
}

TEST_CASE(BaseFileOpenPolicy_NativeAllModesCreateSourcesFieldsAndRing) { // 원문 정상/최종거부/키할당예외와 모든 장치인자/현재256BYTE를 행별로 대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_BASEFILE_OPEN_POLICY_FIXTURE).rows) { // 입력열과 Native 출력열을 별도로 유지한다.
        CHECK(row.size()==15);auto physical=Bytes(row[0]);physical+='\0';const auto nul=physical.find('\0');const auto original=std::string_view(physical.data(),nul);const bool device=original.find(':')!=original.npos || original.starts_with("\\\\");const auto mode=Number(row[1]),create=Number(row[2]),mask=Number(row[3]),archiveOpen=Number(row[4]),encrypted=Number(row[5]),initial=Number(row[6]),createAlt=Number(row[7]),allocateFail=Number(row[8]);
        RawBaseFileState state{0x1001,0x82,7,9,kOldKey,3};RawFileName names;for (std::uint32_t call=0;call<initial;++call) static_cast<void>(names.Format({})); // 원문 초기 고리번호만 설정한다.
        std::vector<std::string> events;std::vector<std::uint8_t> key;RawBaseFileIOHooks io;BaseFileOpenPolicyHooks hooks; // 분기 기대값 대신 명시 입력 mask/메타데이터/CRT만 공급한다.
        io.open=[&](auto name,auto fileMode)->std::uintptr_t { events.push_back("O:"+HexText(name)+":"+std::string(fileMode));const bool archive=name=="primary.tarc" || name=="secondary.tarc";const auto site=device && name==original ? 0U : name.starts_with("C:\\base") ? 1U : 3U;const bool success=archive ? archiveOpen!=0 : (mask&(1U<<site)) || (createAlt && fileMode=="w+b" && mode==1 && create);return success?kFile:0; };
        io.seek=[&](auto token,auto offset,auto origin) { events.push_back("S:"+std::to_string(token)+":"+std::to_string(static_cast<std::uint32_t>(offset))+":"+std::to_string(origin)); };io.freeKey=[&](auto token) { events.push_back("F:"+std::to_string(token)); };
        io.allocateKey=[&](auto size)->std::uintptr_t { events.push_back("N:"+std::to_string(size));if (allocateFail) throw std::bad_alloc();key.assign(size,205);return kNewKey; };io.mutableKey=[&](auto token,auto size) { CHECK(token==kNewKey && size==key.size());return std::span(key); };
        hooks.primaryRoot=[]()->std::string_view { return "C:\\base"; };hooks.secondaryRoot=[]()->std::string_view { return "D:\\disc"; };hooks.archive=[&](auto name,bool secondary)->std::optional<BaseFileArchiveOpen> { events.push_back("A:"+std::to_string(secondary?1:0)+":"+HexText(name));if (!(mask&(1U<<(secondary?4:2)))) return {};return BaseFileArchiveOpen{secondary?"secondary.tarc":"primary.tarc",secondary?0x234:0x123,secondary?0x456:0x345,encrypted}; };
        hooks.unableToOpen=[&](auto name) { events.push_back("E:"+HexText(name));throw OpenPolicyFailure{}; };std::string outcome="ok";
        try { OpenBaseFileByPolicy(state,{reinterpret_cast<const std::uint8_t*>(physical.data()),physical.size()},mode,create,names,io,hooks); }catch (const OpenPolicyFailure&) { outcome="unable"; }catch (const std::bad_alloc&) { outcome="allocation"; }
        const std::array<std::uintptr_t,6> fields{{state.file,state.flags,static_cast<std::uint32_t>(state.offset),static_cast<std::uint32_t>(state.length),state.key,state.keyLength}};std::string final;for (const auto value:fields) { if (!final.empty()) final+=',';final+=std::to_string(value); } // 원문 필드의 DWORD 비트를 host 포인터 폭과 구별해 직렬화한다.
        CHECK(outcome==row[9] && JoinEvents(events)==row[10] && final==row[11] && names.Index()==Number(row[12]) && Hex(names.CurrentBytes())==row[13] && (key.empty()?"-":Hex(key))==row[14]);++count;
    }
    CHECK(count==1364);
}

TEST_CASE(BaseFileOpenPolicy_MetadataDoesNotReadPayloadAndKeepsRealArchiveName) { // 등록 뒤 파일이 이동해도 metadata는 내용/OS를 다시 읽지 않는다.
    const auto root=OpenWork()/std::filesystem::path(u8"메타😀"),path=root/L"MiXeD😀.tarc",moved=root/L"moved.tarc";const auto bytes=OpenTaff();WriteOpenFile(path,bytes);BaseFileSystem files(root);CHECK(files.RegisterArchive(path));std::filesystem::rename(path,moved);const auto metadata=files.FindArchiveOpen("d/a");
    auto expected=std::filesystem::absolute(path).lexically_normal();expected.make_preferred();CHECK(metadata && metadata->fileName==netstorm::platform::PathToUtf8(expected) && metadata->offset==0x40 && metadata->length==4 && metadata->encrypted==2 && files.FileNameOwner()->Index()==0);CHECK(!files.FindArchiveOpen("d/missing") && !files.FindArchiveOpen("d/a",true));
}

TEST_CASE(BaseFileOpenPolicy_ActualArchiveReadsCurrentDiskPayloadAndInstallsKey) { // 등록 snapshot과 실제 FILE/부분읽기의 수명을 구별한다.
    const auto root=OpenWork()/std::filesystem::path(u8"읽기😀"),path=root/L"data.tarc";auto bytes=OpenTaff();WriteOpenFile(path,bytes);BaseFileSystem files(root);CHECK(files.RegisterArchive(path));bytes[0x40]=static_cast<std::uint8_t>('Z'^'m');WriteOpenFile(path,bytes);OpenDisk disk;files.Open(disk.state,"d/a",0,0,disk.io);std::array<std::uint8_t,4> output{},scratch{};ReadBaseFile(disk.state,output,-1,disk.io,scratch);
    CHECK((output==std::array<std::uint8_t,4>{'Z','B','C','D'}) && disk.state.flags==1 && disk.state.offset==0x40 && disk.state.length==4 && disk.state.key==kNewKey && disk.state.keyLength==13 && files.FileNameOwner()->Index()==2);CHECK(files.Read("d/a")==std::vector<std::uint8_t>({'A','B','C','D'})); // 기존 값형 Read API의 등록 snapshot 계약은 그대로다.
}

TEST_CASE(BaseFileOpenPolicy_ArchiveOpenFailureStopsBeforeSecondaryDisk) { // 이미 찾은 TAFF 열기 실패를 다른 출처의 정상파일로 숨기지 않는다.
    const auto root=OpenWork(),primary=root/"primary",secondary=root/"secondary",path=primary/"data.tarc";WriteOpenFile(path,OpenTaff());const std::array<std::uint8_t,1> fallback{{'S'}};WriteOpenFile(secondary/"d/a",fallback);BaseFileSystem files(primary,secondary);CHECK(files.RegisterArchive(path));std::filesystem::rename(path,primary/"moved.tarc");OpenDisk disk;bool failed{};
    try { files.Open(disk.state,"d/a",0,0,disk.io); }catch (const ColorCacheUnableToOpen&) { failed=true; }
    CHECK(failed && disk.state.file==0 && disk.state.key==0 && files.FileNameOwner()->Index()==2 && ReadFileBytes(secondary/"d/a")==std::vector<std::uint8_t>{'S'});
}

TEST_CASE(BaseFileOpenPolicy_ActualRelativeWriterFallsBackToSecondaryAndDirectFailureDoesNot) { // 상대mode2와 직접mode2의 원문 실패 분기를 실제 OS에 연결한다.
    const auto root=OpenWork(),primary=root/"primary",secondary=root/"secondary";std::filesystem::create_directories(primary);std::filesystem::create_directories(secondary/"d");BaseFileSystem files(primary,secondary);const std::array<std::uint8_t,3> bytes{{3,1,4}};
    { OpenDisk disk;files.Open(disk.state,"d/new.bin",2,0,disk.io);WriteBaseFile(disk.state,bytes,-1,disk.io);CHECK(files.FileNameOwner()->Index()==3); } // primary/d는 없으므로 같은mode2의secondary가 새 파일을 생성한다.
    CHECK(ReadFileBytes(secondary/"d/new.bin")==std::vector<std::uint8_t>(bytes.begin(),bytes.end()));const auto before=files.FileNameOwner()->Index();OpenDisk disk;bool failed{};const auto direct=netstorm::platform::PathToUtf8(primary/"missing/new.bin");
    try { files.Open(disk.state,direct,2,0,disk.io); }catch (const ColorCacheUnableToOpen&) { failed=true; }
    CHECK(failed && disk.state.file==0 && files.FileNameOwner()->Index()==(before+1)%10 && !std::filesystem::exists(secondary/"missing/new.bin"));
}

TEST_CASE(BaseFileOpenPolicy_FileSpecFailureAndOrdinaryExceptionsCloseSameHandle) { // 실제 원문264BYTE/SEH ABI와 구별한 호스트 FileSpec의 부분검색 수명을 검증한다.
    RawBaseFileState state{};RawFileName names;RawBaseFileIOHooks io;BaseFileOpenPolicyHooks hooks;std::vector<std::uintptr_t> closed;hooks.search.find=[](auto) { return FileSpecFindResult{0x1234,"chosen"}; };hooks.search.close=[&](auto handle) { closed.push_back(handle); };hooks.primaryRoot=[]()->std::string_view { throw std::runtime_error("명시 root 공급 예외"); };const std::string name="d\\*\0";bool failed{};
    try { OpenBaseFileByPolicy(state,{reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1},0,0,names,io,hooks); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && closed==std::vector<std::uintptr_t>{0x1234} && names.Index()==1 && state.file==0);
}
