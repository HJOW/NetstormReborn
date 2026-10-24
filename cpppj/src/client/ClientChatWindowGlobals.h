// ChatWindow의 현재 전역과 공유 로그 상태를 Client 수명에 보관한다.
#pragma once
#include "client/ChatWindow.h"
#include <map>
#include <stdexcept>
namespace netstorm::client { // 실제 Client Chat UI 전역의 소유 계층이다.
struct ClientChatWindowGlobals { // 표시 노드가 교체되어도 로그/초기화 목록/등록 여부는 같은 주소다.
    ChatWindowState window; //548740/44·54da91와 실제 세 완성 목록이다.
    StyleTextBufferState log; //548758의 실제 공유128줄 로그이며 공통 StyleText 저장소에 직접 연결한다.
    std::uint32_t logFile{}; //54874c의 최초0이며 append callback은 비0일 때만 파일 장치를 요구한다.
    const void* uiOwner{}; // Native 게임 값이 아닌 호스트 단일 조각/글꼴 저장소 대여 관문이다.
    std::map<std::uint32_t,std::string> strings;std::map<std::string,std::uint32_t> stringTokens; // 원문 정적/벡터 C문자열의 안정된 호스트 주소 별칭이다. 게임 값 캐시가 아니다.
    std::uint32_t nextString{0x71000000U}; // 입력70000000 고리/Help62000000..6fffffff와 구별하는 호스트 토큰 범위다.
    std::uint32_t Store(std::string_view text) { // 메뉴가 빌린 Client 수명 문자열을 같은 바이트의 안정된 별칭으로 제공한다.
        const auto found=stringTokens.find(std::string(text));if (found!=stringTokens.end()) return found->second;if (nextString==0x7fffffffU) throw std::overflow_error("Chat 문자열 주소 별칭 소진");const auto token=nextString++;strings.emplace(token,text);stringTokens.emplace(text,token);return token;
    }
};
}
