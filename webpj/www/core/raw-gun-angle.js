// PATCH 10.78 Gunanim 의 프레임→Graph 방향 계산(00467610)을 옮긴 모듈이다(cpppj `o/RawGunAngle.h`).
//  - patchGunFrameIndex(frame, checking, assertion): 프레임을 5 로 나눈 signed 정수(0 쪽 절삭)를 인덱스로 하고 18 을 넘으면 19 로 한다. 현재 checking 이 비0 이고 인덱스가 0..19 밖이면 원문 진단
//     ('0 <= index && index < 20', '\Ns\O\Gunanim.cpp', 0x387)을 요청하며, 진단이 복귀해도 지역 인덱스는 바꾸지 않는다(음수 반환은 표 역참조의 허가가 아님). 진단 장치가 없으면 거부한다.
//   patchGunGraphFromFrame(frame, ...): 같은 인덱스로 DAT00540940 의 실제 20개 binary32 각도를 읽어 방향을 돌려준다: 각도 45 미만(그리고 315 이상)이면 0, 45~135 미만 1, 135~225 미만 2, 225~315 미만 3.
//     원본이 음수 인덱스에서 진단 뒤 표 밖 무효 메모리를 읽는 경로는 정상 방향 0 으로 가장하지 않고 RangeError 로 거부한다. 프레임 번호 자체를 각도로 간주하지 않는다.
// checking 은 호출 위치마다 현재 DWORD 를 읽는 getter 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E16: Gunanim 방향 계산 이식으로 추가)

// 진단의 원문 식과 파일, 줄 번호이다. (2026-11-08 WB04-E16)
const GUN_EXPRESSION = '0 <= index && index < 20';
const GUN_FILE = '\\Ns\\O\\Gunanim.cpp';
const GUN_LINE = 0x387;

// DAT00540940 의 실제 20개 binary32 각도이다(모두 정수라 단정도로 정확히 표현). (2026-11-08 WB04-E16)
const GUN_ANGLES = Object.freeze([0, 21, 35, 52, 71, 86, 97, 115, 121, 148, 180, 205, 237, 256, 272, 282, 291, 314, 328, 345]);

// 프레임을 인덱스로 바꾼다. frame 은 signed 32비트이며 checking() 은 현재 DWORD getter, assertion(식, 파일, 줄)은 선택 진단 장치이다. (2026-11-08 WB04-E16)
export function patchGunFrameIndex(frame, checking = () => 0, assertion = null) {
    // 음수도 0 쪽으로 절삭하며 원문 signed 나눗셈을 유지한다.
    let index = ((frame | 0) / 5) | 0;
    if (index > 18) index = 19;
    if (checking() !== 0 && (index < 0 || index >= 20)) {
        if (typeof assertion !== 'function') throw new Error('Gunanim 인덱스 원문 진단 장치 미연결');
        assertion(GUN_EXPRESSION, GUN_FILE, GUN_LINE);
    }
    return index;
}

// 프레임에서 Graph 방향(0..3)을 구한다. (2026-11-08 WB04-E16)
export function patchGunGraphFromFrame(frame, checking = () => 0, assertion = null) {
    // 진단에서 복귀한 동일 지역 인덱스를 사용한다.
    const index = patchGunFrameIndex(frame, checking, assertion);
    if (index < 0) throw new RangeError('Gunanim 음수 인덱스의 원본 무효 메모리 경로');
    const angle = GUN_ANGLES[index];
    if (angle >= 45) {
        if (angle < 135) return 1;
        if (angle < 225) return 2;
        if (angle < 315) return 3;
    }
    return 0;
}
