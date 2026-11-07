// Boardcoord 의 각도 정규화(0041cdb0)와 PATCH 끝각 처리를 옮긴 모듈이다(cpppj `o/RawBoardAngle.cpp/.h`, 10.78 경로).
// 순서: float 각도가 0 이면 부호와 무관하게 +0 을 반환 → double 로 넓혀 CRT fmod(modulus, 일반 소비자는 360)로 나머지를 구하고 음수면 360 을 더한다(53비트 x87 저장) → 결과가 360 이상이면
//   현재 checking 이 비0 이고 361 이상일 때 원문 진단('angle < 361.0f', '\Ns\O\Boardcoord.cpp', 0xb3)을 요청하고 어떤 경우든 float 359.9999(비트 43b3fffd)를 반환한다 → 그 밖에는 결과를 그대로 반환한다.
// modulus 는 원문의 별도 double 전역(일반 소비자는 360)이고 진단 장치는 { checking(), assertion(식, 파일, 줄) } 이다. 진단 장치 누락은 진단이 선택될 때만 거부한다. CD 의 binary32 중간 저장 경로는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E15: Boardcoord 각도 정규화 이식으로 추가)

// 끝각으로 반환하는 float 359.9999 의 값이다(비트 43b3fffd). (2026-11-08 WB04-E15)
const END_ANGLE = Math.fround(359.9999);

// 진단의 원문 식과 파일, 줄 번호이다. (2026-11-08 WB04-E15)
const ANGLE_EXPRESSION = 'angle < 361.0f';
const ANGLE_FILE = '\\Ns\\O\\Boardcoord.cpp';
const ANGLE_LINE = 0xb3;

// angle 은 float32 로 표현된 각도 수, diagnostics = { checking(), assertion(식, 파일, 줄) }, modulus 는 double 나눗수이다. 결과는 double 이다. (2026-11-08 WB04-E15)
export function normalizeBoardAngle(angle, diagnostics = {}, modulus = 360) {
    // 실제 CRT 의 0 입력 꼬리는 부호와 무관하게 fldz 로 +0 을 반환한다.
    if (angle === 0) return 0;
    const remainder = angle % modulus;
    const result = remainder < 0 ? remainder + 360 : remainder;
    if (result >= 360) {
        if ((diagnostics.checking?.() ?? 0) !== 0 && result >= 361) {
            if (typeof diagnostics.assertion !== 'function') throw new Error('Boardcoord 각도 원문 진단 장치 미연결');
            diagnostics.assertion(ANGLE_EXPRESSION, ANGLE_FILE, ANGLE_LINE);
        }
        // 실제 PATCH 005022a0 의 float 비트 43b3fffd 를 반환한다.
        return END_ANGLE;
    }
    return result;
}
