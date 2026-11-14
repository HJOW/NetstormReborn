// Alt/가운데 버튼 카메라 이동(RawCameraPan)을 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E437)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawCameraPan.cpp)에서 옮겼다. 값은 모두 손 계산이며 각 시험이 계산식을 적었다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputCode } from '../www/core/input-queue.js';
import { processCameraPan } from '../www/core/raw-camera-pan.js';

const MIDDLE = InputCode.middleButton; const SHIFT = 0x00100000; const RELEASE = InputCode.release;
// 기본 전역: 클라이언트 1024×768, 지도 512×384 가 원점 0,0 이다(축 비율이 모두 2).
const base = { scrollWhileMouseOut: 0, dialog: 0, active: 1, clientWidth: 1024, clientHeight: 768, viewX: 0, viewY: 0, viewWidth: 512, viewHeight: 384 };
// 모든 호출을 기록하는 장치 묶음이다. middle/shift 는 폴링 결과 코드, 전역은 호출마다 현재 객체를 읽는다.
function rig({ middle = 0, shift = RELEASE, globals = {} } = {}) {
    const log = []; const state = { globals: { ...base, ...globals }, middle, shift, popupChange: null };
    return { log, state, hooks: {
        poll: (code) => { log.push(['poll', code]); return { code: code === MIDDLE ? state.middle : state.shift, x: 0, y: 0 }; },
        globals: () => { log.push(['globals']); return state.globals; },
        closePopups: () => { log.push(['close']); if (state.popupChange) state.globals = { ...state.globals, ...state.popupChange }; },
        request: (x, y) => log.push(['request', x, y]),
    } };
}
const point = (x, y, code = MIDDLE) => ({ code, x, y });

test('processCameraPan: 가운데 폴링→전역→팝업 닫기→전역→Shift 폴링→목표 요청 순이며 x 는 FTOL(비율×(제한된 위치−중심))/8 이다', () => {
    // x: 비율 1024/512=2, 중심 0+256, 위치 300 → 2×44=88 → 88/8=11. y: 비율 768/384=2, 중심 192, 위치 100 → 2×(−92)=−184 → −23. Shift 폴링의 뗌 비트가 켜져 있으므로 두 축 그대로다.
    const r = rig(); assert.equal(processCameraPan(point(300, 100), r.hooks), true);
    assert.deepEqual(r.log, [['poll', MIDDLE], ['globals'], ['close'], ['globals'], ['poll', SHIFT], ['request', 11, -23]]);
});

test('processCameraPan: Shift 가 눌려 있으면(폴링 코드에 뗌 비트 없음) 절댓값이 큰 한 축만 남기고 같은 크기면 X 를 지운다', () => {
    // X=88(pointer 300), Y=-184: |Y| 가 크므로 X 를 지운다 → (0,-23).
    const held = rig({ shift: 0 }); processCameraPan(point(300, 100), held.hooks); assert.deepEqual(held.log.at(-1), ['request', 0, -23]);
    // X 가 크면 Y 를 지운다: pointer(412,200) → X=2×(412-256)=312, Y=2×(200-192)=16 → (39,0).
    const wide = rig({ shift: 0 }); processCameraPan(point(412, 200), wide.hooks); assert.deepEqual(wide.log.at(-1), ['request', 39, 0]);
    // 같은 크기(둘 다 88)면 X 를 지운다: pointer(300,236) → Y=2×(236-192)=88.
    const tie = rig({ shift: 0 }); processCameraPan(point(300, 236), tie.hooks); assert.deepEqual(tie.log.at(-1), ['request', 0, 11]);
    // 부호 있는 비교: X=-88, Y=40(pointer(212,212)) 이면 |X| 가 크므로 Y 를 지운다. 같은 크기에서 부호가 달라도 X 를 지운다.
    const neg = rig({ shift: 0 }); processCameraPan(point(212, 212), neg.hooks); assert.deepEqual(neg.log.at(-1), ['request', -11, 0]);
    const opposite = rig({ shift: 0 }); processCameraPan(point(212, 236), opposite.hooks); assert.deepEqual(opposite.log.at(-1), ['request', 0, 11]);
    // Shift 폴링 코드의 뗌 외 비트는 무시한다(뗌 비트만 본다).
    const other = rig({ shift: 0x7fffffff }); processCameraPan(point(300, 100), other.hooks); assert.deepEqual(other.log.at(-1), ['request', 0, -23]);
    const released = rig({ shift: RELEASE | 5 }); processCameraPan(point(300, 100), released.hooks); assert.deepEqual(released.log.at(-1), ['request', 11, -23]);
});

test('processCameraPan: 위치는 지도 [원점, 원점+폭] 로 제한하고 FTOL/8 모두 0 방향으로 절삭한다(바닥/반올림 아님)', () => {
    // 제한: pointer 2000 → 512 → 비율 2 × (512-256)=512 → 64. pointer -50 → 0 → -512 → -64.
    const clamp = rig(); processCameraPan(point(2000, -50), clamp.hooks); assert.deepEqual(clamp.log.at(-1), ['request', 64, -48]);
    // 원점 100: 중심 356, pointer 100 → 2×(100-356)=-512 → -64.
    const origin = rig({ globals: { viewX: 100 } }); processCameraPan(point(100, 192), origin.hooks); assert.deepEqual(origin.log.at(-1), ['request', -64, 0]);
    // 절삭: 비율 1281/512=2.501953125, 위치 300 → 중심 256 → 44×2.501953125=110.0859375 → FTOL 110 → 110/8=13.75 → 13. 반대쪽은 -110.08 → -110 → -13.75 → -13(바닥이면 -14).
    const trunc = rig({ globals: { clientWidth: 1281, clientHeight: 1281, viewHeight: 512, viewY: 0 } });
    processCameraPan(point(300, 256 + 44), trunc.hooks); assert.deepEqual(trunc.log.at(-1), ['request', 13, 13]);
    const down = rig({ globals: { clientWidth: 1281, clientHeight: 1281, viewHeight: 512 } }); processCameraPan(point(212, 256 - 44), down.hooks); assert.deepEqual(down.log.at(-1), ['request', -13, -13]);
    // 비정수 비율: 클라 1000/폭 300=3.3333333333333335, 중심 150, 위치 225 → ×75=250.00000000000003 → 250 → 31. 음수 -250 → -31.
    const frac = rig({ globals: { clientWidth: 1000, viewWidth: 300 } }); processCameraPan(point(225, 192), frac.hooks); assert.deepEqual(frac.log.at(-1), ['request', 31, 0]);
    const fracNeg = rig({ globals: { clientWidth: 1000, viewWidth: 300 } }); processCameraPan(point(75, 192), fracNeg.hooks); assert.deepEqual(fracNeg.log.at(-1), ['request', -31, 0]);
    // 홀수 폭 7: 중심 = 원점 + 7/2(정수 3), 위치 6 → 6-3=3, 비율 1024/7 → 3×146.2857142857143=438.85714285714283 → 438 → 54.
    const odd = rig({ globals: { viewWidth: 7 } }); processCameraPan(point(6, 192), odd.hooks); assert.deepEqual(odd.log.at(-1), ['request', 54, 0]);
});

test('processCameraPan: 폭 0 은 비율이 무한대/NaN 이라 FTOL 이 0 을 주고 음수 폭은 상한 선택이 뒤집힌 채 원문 CMP 순서를 따른다', () => {
    // 폭 0, 클라 1024: 비율 무한대 × (위치-중심=0) = NaN → 0. 클라 0 도 0/0=NaN → 0. 위치가 어디든 제한된 위치=원점이라 차이 0 이다.
    const zero = rig({ globals: { viewWidth: 0 } }); processCameraPan(point(300, 192), zero.hooks); assert.deepEqual(zero.log.at(-1), ['request', 0, 0]);
    const zeroClient = rig({ globals: { viewWidth: 0, clientWidth: 0 } }); processCameraPan(point(300, 192), zeroClient.hooks); assert.deepEqual(zeroClient.log.at(-1), ['request', 0, 0]);
    // 폭 -100, 클라 1000: 중심 = 0+(-50), 제한된 위치 = min(max(0,p), -100) = -100 → 차이 -50 → 비율 -10 → 500 → 62.
    const flipped = rig({ globals: { viewWidth: -100, clientWidth: 1000 } }); processCameraPan(point(40, 192), flipped.hooks); assert.deepEqual(flipped.log.at(-1), ['request', 62, 0]);
    // 위치가 원점 아래여도 같다(제한된 위치가 폭 기준 상한이다).
    const below = rig({ globals: { viewWidth: -100, clientWidth: 1000 } }); processCameraPan(point(-300, 192), below.hooks); assert.deepEqual(below.log.at(-1), ['request', 62, 0]);
});

test('processCameraPan: 원점+폭/중심은 signed 32비트로 감기고 FTOL 의 하위 DWORD 가 INT_MIN 이면 Abs 도 음수로 남아 X 를 지운다', () => {
    // 원점 INT_MAX, 폭 2: 중심 = 원점+1 이 INT_MIN 으로 감기고 상한 = 원점+2 = INT_MIN+1, 위치 = min(INT_MAX, INT_MIN+1) → 차이 1, 비율 1000/2=500 → 500 → 62.
    const wrap = rig({ globals: { viewX: 0x7fffffff, viewWidth: 2, clientWidth: 1000 } }); processCameraPan(point(5, 192), wrap.hooks); assert.deepEqual(wrap.log.at(-1), ['request', 62, 0]);
    // INT_MIN 결과: 원점 INT_MIN+1, 폭 -4, 클라 8: 중심 = 원점-2 가 INT_MAX 로 감기고 상한 = 원점-4 = 2147483645, 위치 1073741823 → 차이 -1073741824, 비율 -2 → 2147483648 → 하위 DWORD INT_MIN.
    // 요청은 INT_MIN/8 = -268435456 이다(Shift 가 없을 때).
    const globals = { viewX: -2147483647, viewWidth: -4, clientWidth: 8 };
    const free = rig({ globals }); processCameraPan(point(1073741823, 200), free.hooks); assert.deepEqual(free.log.at(-1), ['request', -268435456, 2]);
    // y=2×(200-192)=16. Shift 가 눌려 있으면 Abs(INT_MIN)이 INT_MIN(음수)이라 |x| > |y| 가 거짓이므로 X 를 지우고 y 를 남긴다 → (0, 2).
    const held = rig({ globals, shift: 0 }); processCameraPan(point(1073741823, 200), held.hooks); assert.deepEqual(held.log.at(-1), ['request', 0, 2]);
    // Y 축에서도 같다(클라이언트/원점/폭/위치 모두 Y 쪽 값을 쓴다): 원점 INT_MIN+1, 높이 -4, 클라 8, 위치 1073741823 → y 가 INT_MIN. X 는 pointer 300 → 88 → 11.
    const yAxis = { viewY: -2147483647, viewHeight: -4, clientHeight: 8 };
    const yFree = rig({ globals: yAxis }); processCameraPan(point(300, 1073741823), yFree.hooks); assert.deepEqual(yFree.log.at(-1), ['request', 11, -268435456]);
    const yHeld = rig({ globals: yAxis, shift: 0 }); processCameraPan(point(300, 1073741823), yHeld.hooks); assert.deepEqual(yHeld.log.at(-1), ['request', 11, 0]);
    // 2^53 을 넘는 곱도 하위 DWORD 만 남긴다: 비율 2147483647/-4=-536870911.75, 차이 -2147483647 → 곱 ≈1.15e18 의 정수부를 BigInt 로 32비트 감아 기대값을 얻는다.
    const big = rig({ globals: { viewX: -2147483647, viewWidth: -4, clientWidth: 0x7fffffff } }); processCameraPan(point(0, 192), big.hooks);
    const low = Number(BigInt.asIntN(32, BigInt(Math.trunc(-536870911.75 * -2147483647)))); assert.deepEqual(big.log.at(-1), ['request', (low / 8) | 0, 0]);
});

test('processCameraPan: 창 밖/뗌/대화상자 관문이면 false 이며 팝업을 닫지도 요청하지도 않고, 좌표 -1/비활성은 팝업만 닫고 true 다', () => {
    // 창 밖(outside)이고 밖 이동이 꺼져 있으면 거부. 켜져 있으면 진행한다.
    const out = rig(); assert.equal(processCameraPan(point(300, 100, MIDDLE | InputCode.outside), out.hooks), false); assert.deepEqual(out.log, [['poll', MIDDLE], ['globals']]);
    const outOn = rig({ globals: { scrollWhileMouseOut: 3 } }); assert.equal(processCameraPan(point(300, 100, MIDDLE | InputCode.outside), outOn.hooks), true); assert.deepEqual(outOn.log.at(-1), ['request', 11, -23]);
    // 포인터와 가운데 버튼이 모두 뗌이면 거부. 한쪽만이면 진행한다.
    const both = rig({ middle: RELEASE }); assert.equal(processCameraPan(point(300, 100, MIDDLE | RELEASE), both.hooks), false); assert.equal(both.log.length, 2);
    const onlyPointer = rig({ middle: 0 }); assert.equal(processCameraPan(point(300, 100, MIDDLE | RELEASE), onlyPointer.hooks), true);
    const onlyMiddle = rig({ middle: RELEASE }); assert.equal(processCameraPan(point(300, 100, MIDDLE), onlyMiddle.hooks), true);
    // 대화상자가 비영이면 거부한다(다른 값이어도 비영이다).
    const dialog = rig({ globals: { dialog: -1 } }); assert.equal(processCameraPan(point(300, 100), dialog.hooks), false); assert.equal(dialog.log.length, 2);
    // 좌표 -2 는 -1 이 아니다(비트 or 로 -1 과 섞이지 않는다): x=-2 → 제한된 위치 0 → 2×(0-256)=-512 → -64. y=-2 → 제한된 위치 0 → 2×(0-192)=-384 → -48.
    const minus2x = rig(); assert.equal(processCameraPan(point(-2, 192), minus2x.hooks), true); assert.deepEqual(minus2x.log.at(-1), ['request', -64, 0]);
    const minus2y = rig(); assert.equal(processCameraPan(point(256, -2), minus2y.hooks), true); assert.deepEqual(minus2y.log.at(-1), ['request', 0, -48]);
    // 좌표 -1 이나 비활성: 팝업은 닫고(전역은 팝업 뒤 다시 읽음) 요청 없이 true.
    for (const [pointer, globals] of [[point(-1, 100), {}], [point(300, -1), {}], [point(300, 100), { active: 0 }]]) { const r = rig({ globals }); assert.equal(processCameraPan(pointer, r.hooks), true); assert.deepEqual(r.log, [['poll', MIDDLE], ['globals'], ['close'], ['globals']]); }
    // 팝업 닫기 뒤 전역을 다시 읽는다(닫는 동안 활성이 꺼지면 요청하지 않는다).
    const changing = rig(); changing.state.popupChange = { active: 0 }; assert.equal(processCameraPan(point(300, 100), changing.hooks), true); assert.deepEqual(changing.log, [['poll', MIDDLE], ['globals'], ['close'], ['globals']]);
});

test('processCameraPan: 장치 누락은 도달한 위치에서만 Error 이며 이후 효과는 일어나지 않는다', () => {
    assert.throws(() => processCameraPan(point(300, 100), {}), { name: 'Error', message: /장치 미연결/ });
    const r = rig(); assert.throws(() => processCameraPan(point(300, 100), { poll: r.hooks.poll }), /장치 미연결/); assert.deepEqual(r.log, [['poll', MIDDLE]]);
    const noClose = rig(); assert.throws(() => processCameraPan(point(300, 100), { poll: noClose.hooks.poll, globals: noClose.hooks.globals }), /장치 미연결/); assert.deepEqual(noClose.log, [['poll', MIDDLE], ['globals']]);
    const noRequest = rig(); assert.throws(() => processCameraPan(point(300, 100), { poll: noRequest.hooks.poll, globals: noRequest.hooks.globals, closePopups: noRequest.hooks.closePopups }), /장치 미연결/); assert.deepEqual(noRequest.log.at(-1), ['poll', SHIFT]);
    // 거부되는 관문과 좌표 -1 경로는 팝업/요청 장치를 요구하지 않는다.
    const gate = rig({ globals: { dialog: 1 } }); assert.equal(processCameraPan(point(300, 100), { poll: gate.hooks.poll, globals: gate.hooks.globals }), false);
});
