// 미션이 선택한 같은 파일 읽기의 원문 이미지와 실제 Template 파일 이름을 함께 전달한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace netstorm::client { // 논리 미션 이름과 저장에 사용할 파일 경로의 수명을 분리한다.
struct MissionFortSourceImage { // 한 번 선택한 fortSpec 경로와 그 읽기 결과다. 재조회로 서로 다른 파일을 섞지 않는다.
    std::vector<std::uint8_t> image; // 실제 선택 source가 읽은 같은 Fort 파일 원문이다.
    std::string fileName; // 실제 manager+c에 설치할 파일 경로다. 편집 작업본은 같은 writable namespace의 경로다.
};
using MissionFortFileReader=std::function<MissionFortSourceImage(std::string_view)>; //입력은 논리 요새 이름이며 출력은 같은 읽기의 파일 이미지/경로다.
struct MissionFortFileSelection { // 파일을 열기 전에 같은 manager 이름을 비교할 수 있도록 선택과 읽기를 나눈다.
    std::string fileName; // 선택 순간의 fortSpec과 mapper가 정한 실제 manager 파일 이름이다.
    std::function<std::vector<std::uint8_t>()> read; // 선택한 경로와 source를 소유하며 호출할 때만 같은 파일을 읽는다.
};
using MissionFortFileSelector=std::function<MissionFortFileSelection(std::string_view)>; // 논리 요새 이름으로 현재 경로를 한 번 선택하며 파일 I/O는 수행하지 않는다.
}
