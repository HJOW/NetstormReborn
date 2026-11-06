// 원본 File 쓰기를 같은 BaseFile 본체/Unicode CRT와 사용자 홈의 저장 파일에 연결한다.
#include "client/SavedStateFileWrite.h"
#include "client/ClientMain.h"
#include "client/ClientFortGlobals.h"
#include "client/ClientStaticStrings.h"
#include "client/SavedStateFiles.h"
#include "platform/Utf8Path.h"
#include "o/BaseFile.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace netstorm::client {
namespace { // 한 Session의 동기 File 대여만 소유한다.
struct SavedStateWriterFile {
    o::RawBaseFileState state{};o::RawBaseFileIOHooks io; // 현재 실제 FILE/키와 기존41a910/41b410의 장치다.
    SavedStateWriterFile() { // 원문 ANSI 대신 Unicode CRT만 바꾸고 반환/부분쓰기 의미를 보존한다.
        o::ConstructBaseFile(state);io.open=[](auto name,auto mode) {
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str()));
#else
            const std::string narrow(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),narrow.c_str()));
#endif
        };
        io.write=[](auto file,auto bytes,auto size,auto count) { static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 원문은 fwrite(data,size,1)의 반환을 사용하지 않는다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // fclose 반환으로 새 실패 분기를 추가하지 않는다.
    }
    ~SavedStateWriterFile() { o::DestructBaseFile(state,io); } // 미완성 열기/Session 반납의 잔여 FILE도 한 번 반납한다.
};
}
// 경로 고리와 VFS 정책은 실제 open 위치에서 수행하고 이름/FILE을 미리 캐시하지 않는다.
o::SavedStateWriteHooks MakeSavedStateFileWriteHooks(Client& client) {
    const auto file=std::make_shared<SavedStateWriterFile>();o::SavedStateWriteHooks hooks;
    hooks.currentFort=[&client] { return std::span<const std::uint8_t>(client.FortGlobals()->CurrentBytes()); };
    hooks.open=[&client,file](auto mode,auto create) {
        if (file->state.file) throw std::logic_error("저장상태 FILE 중복 열기");const auto path=SavedStateFilePath(client.GameDirectory(),client.StaticStrings()->FileNames(),client.UserSavedStateFile());
        client.Files().Open(file->state,platform::PathToUtf8(path),mode,create,file->io);
    };
    hooks.write=[file](auto bytes,auto offset) { o::WriteBaseFile(file->state,bytes,offset,file->io); };hooks.close=[file] { o::DestructBaseFile(file->state,file->io); };
    hooks.wallSeconds=[&client] { return client.WallSeconds(); };hooks.assertion=[](auto expression,auto line) { std::clog<<"저장상태 State.cpp 진단 "<<line<<": "<<expression<<'\n'; };return hooks;
}
}
