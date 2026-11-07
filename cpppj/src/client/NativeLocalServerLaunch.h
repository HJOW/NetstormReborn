//원문State37의현재설정이름/여섯검색순서/실제lan실행장치를구별한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { //같은Client의현재파일명고리/파일출처/외부실행장치를빌린다.
struct NativeLocalServerLaunchHooks {
    std::function<void()> progress; //현재Becoming local TCP/IP server안내를표시하는장치다.
    std::function<void()> closeDiscovery; //004e2d20의현재12/13을닫는실제장치다.
    std::function<std::string()> executable; //00441300의rootServerExe를PathSpec이아닌현재문자열로읽는다.
    std::function<std::string(std::string_view,std::string_view)> format; //현재0054d988과prefix/name을같은459c60의-2/-2/-3으로조합한다.
    std::function<std::uint32_t(std::string_view)> exists; //0041ab10의실제파일출처이며모든비영값을채택한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> launch; //004dd6b0의program/lan/wait0을실제새서버실행장치에전달한다.
};
o::LocalMissionParentResult InstallNativeLocalServerLaunch(o::LocalMissionStateStorage state,const NativeLocalServerLaunchHooks& hooks); //원문37의안내→닫기→설정→검색/실패34또는실행뒤38이며공통144BYTE확정은바깥분배기에남긴다.
}
