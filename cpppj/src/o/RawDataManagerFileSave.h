// 원본10.78의 DataManager 저장 부모를 실제 이미지/현재 전역/파일 장치와 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 이미지 생성 효과 뒤 다시 읽을 같은 원본 전역이다.
struct DataManagerFileSaveState {
    const std::uint32_t& readOnlyForts; //0054db30의 현재 비영 저장 억제 값이다.
    const std::uint32_t& localOwner; //00540c70의 현재 숨김 표 인덱스다.
    std::span<const std::uint8_t> hiddenPlayers; //0052f81e부터의 실제 현재 숨김 BYTE다.
};
// 저장 규칙을 대신 계산하지 않는 이미지/가상 이름/CRT/파일 쓰기 경계다.
struct DataManagerFileSaveHooks {
    std::function<void(std::span<std::uint8_t>,std::span<std::uint8_t,20>)> image; //0044d640이 같은 manager와 초기 MarkStream20에 쓴다.
    std::function<std::string_view(std::span<std::uint8_t>)> fileName; //현재 manager 가상+0이며 권한/쓰기에서 각각 조회한다.
    std::function<void(std::string_view,std::uint32_t)> permissions; //CRT chmod의 현재 이름과0x180 요청이다. 반환값은 원본처럼 사용하지 않는다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> write; //현재 이름/앞서 보관한 이미지 포인터/쓰기 인자 길이의0041bb90이다.
    std::function<void(std::uint32_t)> release; //정상 반환에서 보관한 이미지 포인터를 한 번만 반납한다.
};
//0044d8d0: 이미지 생성→현재 저장/숨김 관문→두 이름 조회/권한/쓰기→이미지 반납이다.
//호스트 예외에서도 이미지 소유 할당을 반납하며 원본 SEH의 모든 변이를 복원했다는 뜻은 아니다.
void SaveDataManagerFile(std::span<std::uint8_t> manager,const DataManagerFileSaveState& state,const DataManagerFileSaveHooks& hooks);
}
