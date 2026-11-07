// 원문의 행 바이트는 유지하고 새 파일의 실제 위치만 5차 사용자 홈 정책으로 옮긴다.
#include "client/RootServerListDisk.h"
#include "client/ClientMain.h"
#include "client/ClientNsInfoGlobals.h"
#include "o/RawFileName.h"
#include "platform/Utf8Path.h"
#include <cstdio>
#include <memory>

namespace netstorm::client { // 호출자는 같은 실제 Client를 읽기/갱신/저장 전체에 사용한다.
namespace { // 파일 조회는 디렉터리를 생성하지 않으며 저장 시에만 생성한다.
std::filesystem::path UserFile(const Client& client) { return (client.UserData() ? client.UserData()->Saves():platform::UserDataRoot::Resolve().Saves())/L"!rootservers.dat"; } // 정책 없는 호스트도 설치 디렉터리 쓰기로 되돌리지 않는다.
}
void LoadClientRootServerList(Client& client) { // 저장 파일이 없을 때만 이전 설치 자료 순서로 읽는다.
    auto bytes=client.Files().TryRead(platform::PathToUtf8(UserFile(client)));
    if (!bytes) bytes=client.Files().TryRead(client.Configuration().SubstituteLocal("{DataDir}/!rootservers.dat"));
    if (!bytes) bytes=client.Files().TryRead(client.Configuration().SubstituteLocal("{DataDir}/rootservers.dat"));
    if (bytes) client.NsInfoGlobals()->LoadServerNames(*bytes);
}
void SaveClientRootServerList(Client& client) { // 원문 열기 실패는 조용히 복귀하며 성공한 FILE은 모든 경로에서 반납한다.
    const auto root=platform::PathToUtf8(client.GameDirectory()),data=client.Configuration().SubstituteLocal("{DataDir}");
    const std::array<o::FileNamePart,3> parts{{{-2,root},{-2,data},{-3,"!rootservers.dat"}}};static_cast<void>(client.Files().FileNameOwner()->Format(parts)); //00458e30의 실제 root/DataDir/파일 한 칸 작성 효과 뒤 물리 저장 위치만 사용자 홈으로 옮긴다.
    const auto path=UserFile(client);platform::UserDataRoot::EnsureDirectory(path.parent_path());
    const std::unique_ptr<std::FILE,decltype(&std::fclose)> file(::_wfopen(path.c_str(),L"w+b"),&std::fclose);if (!file) return;
    for (const auto& row:client.NsInfoGlobals()->servers) { // 원문 첫NUL까지의 행을 쓰고 LF가 하나도 없을 때만 LF 한 개를 추가한다.
        const auto text=std::string_view(row).substr(0,row.find('\0'));static_cast<void>(std::fwrite(text.data(),text.size(),1,file.get()));if (text.find('\n')==text.npos) static_cast<void>(std::fwrite("\n",1,1,file.get()));
    }
}
}
