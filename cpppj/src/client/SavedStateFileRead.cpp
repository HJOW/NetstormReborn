// 파일 전체를0패딩하지 않고 기존 BaseFile의 실제 부분 읽기를 같은 원문 reader에 공급한다.
#include "client/SavedStateFileRead.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientFortGlobals.h"
#include "client/ClientStaticStrings.h"
#include "client/SavedStateFiles.h"
#include "client/CurrentAllocationBytes.h"
#include "client/FortSelection.h"
#include "o/RawSavedStateRead.h"
#include "o/RawPlayerCoreRecords.h"
#include "o/RawGameAuxState.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <array>
#include <bit>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX //Win32 매크로와 표준 타입/알고리즘의 충돌을 막는다.
#endif
#include <windows.h>
#endif

namespace netstorm::client {
namespace { // 동기 읽기의 실제 FILE만 소유하며 원문24BYTE ABI는 호스트 토큰과 구별한다.
struct SavedStateReaderFile {
    o::RawBaseFileState state{};o::RawBaseFileIOHooks io; // 같은 실제 File의 현재 포인터/읽기/반납 장치다.
    SavedStateReaderFile() { // 원문 ANSI 파일 열기만 Windows10/11 Unicode CRT로 바꾼다.
        o::ConstructBaseFile(state);io.open=[](auto name,auto mode) {
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.read=[](auto file,auto bytes,auto size,auto count) { static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 부분 읽기 뒤 나머지 입력 BYTE는 그대로 둔다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 원문의 미사용 fclose 반환을 새 실패로 해석하지 않는다.
    }
    ~SavedStateReaderFile() { o::DestructBaseFile(state,io); } // 미완성 Open/예외의 잔여 파일도 실제 한 번 반납한다.
};
}
// 부재 관문 전에는 scratch/Player를 읽거나 초기화하지 않으며 존재할 때만 실제 하위를 요구한다.
std::int32_t ReadClientSavedState(Client& client) {
    const auto path=platform::PathToUtf8(SavedStateFilePath(client.GameDirectory(),client.StaticStrings()->FileNames(),client.UserSavedStateFile()));std::array<std::uint8_t,o::kSavedStateReadScratchBytes> scratch;SavedStateReaderFile file;o::SavedStateReadHooks hooks;
    hooks.path=[&path] { return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(path.c_str()),path.size()+1); };
    hooks.exists=[&scratch](auto name) {
        const auto actual=platform::PathFromUtf8(name);
#ifdef _WIN32
        const bool exists=::GetFileAttributesW(actual.c_str())!=INVALID_FILE_ATTRIBUTES; // 원문은 디렉터리도 비영이며 이후 실제 open이 실패할 수 있다.
#else
        std::error_code error;const bool exists=std::filesystem::exists(actual,error);
#endif
        if (exists) CurrentAllocationBytes(scratch);return exists ? 1:0; // 짧은 읽기에 남는 입력을 명시 현재 CRT BYTE로 공급하며 원본32비트 stack 배치 일치는 주장하지 않는다.
    };
    hooks.open=[&client,&file](auto name,auto,auto mode,auto create) { client.Files().Open(file.state,name,mode,create,file.io);return 1U; }; //1은 이 지역 File 대여 식별자이며 실제 FILE 포인터를DWORD로 절삭한 값이 아니다.
    hooks.read=[&file](auto token,auto bytes,auto offset) { if (token!=1 || !file.state.file) throw std::logic_error("저장상태 reader의 실제 File 대여 없음");o::ReadBaseFile(file.state,bytes,std::bit_cast<std::int32_t>(offset),file.io);return 0U; }; // 원문 부모는 반환 길이를 검사하지 않는다.
    hooks.close=[&file](auto token,auto) { if (token!=1) throw std::logic_error("저장상태 reader File 반납 식별자 오류");o::DestructBaseFile(file.state,file.io); };
    hooks.pending=[&client](auto name) { const std::string source(name);SetPendingFortName(client.FortGlobals()->PendingBytes(),{reinterpret_cast<const std::uint8_t*>(source.c_str()),source.size()+1},[] { std::clog<<"저장상태 pending 이름 점 진단\n"; }); };
    hooks.initializeCore=o::InitializePlayerCore;hooks.metadata=[&client](auto owner,auto subscriber,auto core) { client.CoreDataGlobals()->StoreMetadata(owner,subscriber,core); };
    auto& local=*client.LocalStateGlobals();o::SavedStateReadGlobals globals;
    globals.owner=[&local]()->std::uint32_t& { return local.RestoreOwner(); };globals.subscriber=[&local]()->std::uint32_t& { return local.RestoreSubscriber(); };globals.previousHandle=[&local] { return local.PreviousHandle(); };
    globals.players=[&client] { return client.PlayerRecords()->Records(); };globals.tail={[&client]()->std::uint32_t& { return client.GameplayGlobals()->GameAux()->SpellCount(); },[&local]()->std::uint32_t& { return local.SourceType(); },[&local]()->std::uint32_t& { return local.GameTicks(); }};
    return o::ReadSavedState(scratch,globals,hooks);
}
}
