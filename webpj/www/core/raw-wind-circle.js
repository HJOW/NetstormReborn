// 윈드 소환 생산기의 원주 좌표 계산(41e020 / 특별 Wind 생산 12단계의 원주 위치)을 옮긴 모듈이다(cpppj `o/RawWindCircle.cpp/.h` 의 windCirclePoint, 10.78 전용).
//  - windCirclePoint(x, y, count, index): 원주를 count 등분(각 간격 = 360/count 정수 나눗셈)한 index+1 번째(index 는 DWORD 감김) 점이다. 각도는 `(360/count) × (index+1) × 3.141593 × 0.0055555556900799274(float)` 이며,
//    결과는 `(cos(각) × count + x, sin(각) × count + y)` 를 각각 float 로 저장한 두 값이다. count 가 0 이면 나눗셈 오류로 거부한다. 표준 pi 나 VFX 높이로 치환하지 않는다.
// 부모의 현재 중심/원문 count·index 를 읽고 SHP 헤더(높이·hotspot)로 보정하는 windCirclePosition 은 윈드 생산 프로세스(RawWindAviaryProcess) 이식 때 288행 원주 프레임 관측과 함께 옮긴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E121: RawWindCircle 의 원주 점 이식으로 추가)

// 원본 502328 의 double pi 근사와 502320 의 float 도 변환 계수(float 로 저장된 값)이다. 원본 float 저장 경계마다 먼저 반올림한다. (2026-11-08 WB04-E121)
const PI = 3.141593;
const DEGREE_SCALE = Math.fround(0.0055555556900799274);

// 원주 위의 한 점을 구한다. 사용: const [px, py] = windCirclePoint(x, y, 6, 2). 입력 x,y 는 float 값이고 count/index 는 int32 이다. (2026-11-08 WB04-E121)
export function windCirclePoint(x, y, count, index) {
    if (!count) throw new RangeError('Wind 원주 개수 0 나눗셈');
    // 원본 add 의 하위 DWORD 감김이다.
    const next = ((index >>> 0) + 1) | 0;
    const angle = Math.trunc(360 / (count | 0)) * next * PI * DEGREE_SCALE;
    return [Math.fround(Math.cos(angle) * count + x), Math.fround(Math.sin(angle) * count + y)];
}
