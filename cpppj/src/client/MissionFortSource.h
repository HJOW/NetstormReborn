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
}
