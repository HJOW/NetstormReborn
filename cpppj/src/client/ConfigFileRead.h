// 실제 Client의 Config 파일만 공통40380/현재 FILE 부모를 사용한다.
#pragma once
#include "o/ConfigInterface.h"
#include <filesystem>

namespace netstorm::o { class BaseFileSystem; } // 같은 현재 root/TAFF/이름 고리를 빌린다.
namespace netstorm::client { // 공유 Fort/Editor 값 reader와 독립인 Config 접합이다.
o::FileSpecQueryHooks MakeConfigFileSearchHooks(std::function<std::filesystem::path()> directory); //실제검색직전현재root를읽는다.일반비패턴입력은root/검색장치를호출하지않는다.
o::ConfigFileLoadHooks MakeConfigFileReadHooks(const o::BaseFileSystem& files); // 생성만으로 I/O/이름고리를 쓰지 않으며 실제 선택한 파일·인자에서만 Probe/Load한다.
}
