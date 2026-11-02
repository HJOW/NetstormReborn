// 같은 전체 BaseFile 열기 정책을 CreateHelpList의 실제 CRT writer에 연결한다.
#pragma once
#include "client/QuickHelpList.h"
#include <filesystem>

namespace netstorm::o { class BaseFileSystem; } //실제 Client의 같은 VFS/10칸 경로 소유자를 동기 호출 동안 빌린다.
namespace netstorm::client { //경로/FILE은 지역 부모가 끝날 때까지 같은 장치가 소유한다.
//구성 자체는 파일을 열거나 이름 고리를 소비하지 않는다. source가 있으면 실제 전체41b4c0 정책을 재사용한다.
QuickHelpListFileHooks MakeHelpListDiskHooks(const std::filesystem::path& directory,const o::BaseFileSystem* source=nullptr);
}
