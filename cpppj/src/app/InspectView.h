// 새로 쓰는 코드(원본에 없음): Renderer·UserInput을 옮기기 전까지 창에 내용을 보여 주는 검사용 화면.
// 원본 방식의 화면 장치(8비트 DIB, 원본 팔레트, VFX 그리기)와 요새 읽기·영역 배치가 실제로 동작하는지 눈으로 확인하는 데 쓴다.
// 게임 화면을 재현한 것이 아니다(지형·그림자·프레임 선택·그리기 순서가 원본과 다르다). Renderer를 옮기면 없앤다.
#pragma once
#include "client/ClientMain.h"
#include <string>
#include <vector>

namespace netstorm::app {
class InspectView {
public:
    // 보여 줄 내용: "types"는 타입 기본 프레임 표, 그 밖의 값은 미션 이름으로 보고 요새의 오브젝트를 놓는다.
    InspectView(netstorm::client::Client& client, std::string view);
private:
    // 월드에 놓은 오브젝트 하나: 월드 칸 좌표, 타입(로딩 순서), 그릴 프레임.
    struct Placed {
        int cellX{}, cellY{};
        std::size_t asset{};
        std::size_t frame{};
    };
    // 초기화가 끝난 뒤 한 번: 미션이면 요새를 읽어 오브젝트를 놓는다.
    void Ready(netstorm::client::Client& client);
    // 프레임마다: 화면을 지우고 내용을 그린다.
    void Draw(netstorm::client::Client& client, std::uint8_t* buffer);
    // 프레임마다: 입력 큐를 비우고 Esc면 참(종료)을 돌려준다. 화살표 키로 화면을 옮긴다.
    bool Input(netstorm::client::Client& client);
    // 타입 기본 프레임을 격자로 그린다.
    void DrawTypes(netstorm::client::Client& client, std::uint8_t* buffer);
    // 놓은 오브젝트를 월드 칸 좌표에 그린다.
    void DrawFort(netstorm::client::Client& client, std::uint8_t* buffer);

    std::string view_;
    std::vector<Placed> placed_;
    int scrollX_{}, scrollY_{};
};
}
