// 신전(Vortex) 소유자 변경에서 섬을 찾지 못했을 때(missingIsland)의 원문 상세 문자열과 진단 위치를 옮긴 모듈이다(cpppj `o/RawVortexOwnerDiagnostic.cpp/.h`, 10.78 전용: 004dc58f..004dc5f8).
// 원문 256바이트 지역 버퍼에 `islandList.totalIslandIds=%d\nislandId=%d\npos=(%d,%d)\noldPlayerId=%d\nnewPlayerId=%d\n` 을 채운 뒤 실제 상세 저장 장치(detail)에 한 번 쓰고, 이어서 진단(assertion '0', `\Ns\O\Vortex.cpp` 줄 0x235)을 올린다.
// 진단이 정상 복귀하면 이 함수도 복귀한다. 상세 기록 뒤 진단 장치가 없으면 첫 기록을 되돌리지 않고 거부한다. 숫자는 모두 signed 10진수이며, 좌표는 현재 슬롯 +14/+18 의 단정도 float 을 CRT ftol(signed 64비트 절삭)의 하위 DWORD 로 바꾼 값이다
// (비유한/범위 밖은 무효 FISTP 의 하위 0). 옛 소유자는 현재 슬롯 +34 BYTE, 새 소유자와 섬 목록 개수는 요청 DWORD 의 signed 해석이다.
// TYPE/free 상태로 앞선 진단 접두를 취소하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E78: 신전 소유자 진단 이식으로 추가)

// 진단 식과 원문 파일 이름·줄, 슬롯의 옛 소유자 위치와 좌표 위치, 원문 지역 버퍼 폭이다. (2026-11-08 WB04-E78)
const ASSERTION_EXPRESSION = '0';
const SOURCE_FILE = '\\Ns\\O\\Vortex.cpp';
const SOURCE_LINE = 0x235;
const OWNER_AT = 34;
const X_AT = 14;
const Y_AT = 18;
const BUFFER_BYTES = 256;

// 비정렬 좌표 DWORD 를 little endian 단정도 float 으로 읽는다. (2026-11-08 WB04-E78)
const view = new DataView(new ArrayBuffer(4));
function readFloat(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원문 CRT ftol 의 signed64 절삭 하위 DWORD(signed)이다. 무효 FISTP 의 하위 DWORD 는 0 이다. (2026-11-08 WB04-E78)
function coordinate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// hooks = { detail(문자열), assertion(식, 파일, 줄) } 이다. 선택한 장치만 요구하며 상세를 먼저 쓴 뒤 진단을 올린다. (2026-11-08 WB04-E78)
export function reportVortexOwnerMissingIsland(pool, sid, island, owner, count, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('신전 Owner 진단은 PATCH 전용입니다');
    // TYPE/free 로 앞선 진단 접두를 취소하지 않는다.
    const raw = pool.slot(sid);
    const text = `islandList.totalIslandIds=${count | 0}\nislandId=${island | 0}\npos=(${coordinate(readFloat(raw, X_AT))},${coordinate(readFloat(raw, Y_AT))})\noldPlayerId=${raw[OWNER_AT]}\nnewPlayerId=${owner | 0}\n`;
    if (text.length >= BUFFER_BYTES) throw new RangeError('신전 Owner 원문 상세 버퍼 범위 오류');
    if (typeof hooks?.detail !== 'function') throw new Error('신전 Owner 실제 상세 저장 장치 미연결');
    hooks.detail(text);
    if (typeof hooks.assertion !== 'function') throw new Error('신전 Owner 실제 진단 장치 미연결');
    hooks.assertion(ASSERTION_EXPRESSION, SOURCE_FILE, SOURCE_LINE);
}
