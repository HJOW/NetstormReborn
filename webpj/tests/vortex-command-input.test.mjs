// 신전 가상 6c/Descend/ViewMeta 일반 입력 분배 vortex-command-input.js 를 cpppj `VortexCommandInput.cpp` 의 분기·단락·WORD 폭 계약으로 대조한다(WB04-E536).
// 원문에 이 분배기만 따로 관측한 fixture 는 없으므로(cpppj 에도 전용 시험이 없다) 소스의 각 분기를 직접 시험한다.
//  - 이름표: 네 이름만 ASCII 대소문자를 무시하고 조회하며 별칭·부분 일치·NUL 뒤 글자는 미등록(0xffffffff)이다.
//  - 세 신전 명령: 낮은 WORD 대상·높은 WORD 확인 인자·대상 0 무장치·뗌 소비·수정키 제거 비교·event.code 전체를 명령에 전달.
//  - ViewMeta: 현재 명령 비교·뗌·signed 서버 WORD(-1 허용·-2 거부·상한 이상 거부)·단락 평가·낮은 WORD 지역.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E536)
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputCode } from '../www/core/input-queue.js';
import { dispatchVortexCommandInput, dispatchViewMetaInput, vortexCommandScriptCode, VORTEX_COMMANDS } from '../www/core/vortex-command-input.js';

test('신전 명령 이름표: 네 이름만 ASCII 대소문자를 무시하고 조회하며 나머지는 공통 조회를 잇는다', () => {
    assert.equal(vortexCommandScriptCode('WipeOut'), 0x0300006f);
    assert.equal(vortexCommandScriptCode('ASCEND'), 0x0300007c);
    assert.equal(vortexCommandScriptCode('descend'), 0x030000a7);
    assert.equal(vortexCommandScriptCode('ViewMeta'), 0x02000013);
    // 별칭·부분 일치·꼬리 글자·NUL 뒤 글자는 미등록이다(원문은 NUL 을 자르지 않고 전체를 비교한다).
    for (const name of ['', 'wipe', 'wipeouts', 'view meta', 'ascend\0', 'metaview']) assert.equal(vortexCommandScriptCode(name), 0xffffffff, name);
    assert.deepEqual(VORTEX_COMMANDS, [0x0300006f, 0x0300007c, 0x030000a7]);
});

test('신전 일반 입력: 낮은 WORD 는 대상 SID, 높은 WORD 는 확인 인자이고 대상 0 과 뗌은 장치 없이 소비된다', () => {
    const calls = [];
    const command = (target, code, parameter) => { calls.push([target, code, parameter]); return 123; };
    // 모든 수정키·마우스·폴링 비트가 붙어도 수정키를 지운 코드로 비교하고, 명령에는 사건 코드 전체가 간다.
    for (const code of VORTEX_COMMANDS) {
        calls.length = 0;
        const full = (code | InputCode.shift | InputCode.alt) >>> 0;
        assert.equal(dispatchVortexCommandInput({ code: full, x: 7, y: 0x00020003 }, command), true);
        assert.deepEqual(calls, [[3, full, 2]]);
    }
    // y 가 음수여도 하위 32비트 WORD 로 읽는다(0xfffeabcd → 대상 0xabcd, 인자 0xfffe).
    calls.length = 0;
    assert.equal(dispatchVortexCommandInput({ code: 0x0300007c, x: 0, y: -0x00015433 }, command), true);
    assert.deepEqual(calls, [[0xabcd, 0x0300007c, 0xfffe]]);
    // 대상 0 이면 장치를 요구하지 않고 소비만 한다(미연결이어도 예외 없음).
    assert.equal(dispatchVortexCommandInput({ code: 0x0300006f, x: 0, y: 0x00050000 }, undefined), true);
    assert.equal(dispatchVortexCommandInput({ code: 0x0300006f, x: 0, y: 0 }, undefined), true);
    // 뗌은 대상이 있어도 효과 없이 소비한다.
    calls.length = 0;
    assert.equal(dispatchVortexCommandInput({ code: (0x030000a7 | InputCode.release) >>> 0, x: 0, y: 5 }, command), true);
    assert.equal(calls.length, 0);
    // 다른 코드는 소비하지 않고 대상이 있는데 장치가 없으면 성공으로 대체하지 않는다.
    assert.equal(dispatchVortexCommandInput({ code: 0x0300006e, x: 0, y: 5 }, command), false);
    assert.equal(dispatchVortexCommandInput({ code: 0x02000013, x: 0, y: 5 }, command), false);
    assert.throws(() => dispatchVortexCommandInput({ code: 0x0300007c, x: 0, y: 5 }, undefined), /미연결/);
});

test('ViewMeta 일반 입력: 현재 명령만 소비하고 signed 서버 WORD 범위에 따라 잘못된 서버 안내 또는 낮은 WORD 지역 Descend 를 요청한다', () => {
    const trace = [];
    const hooks = {
        serverCount: () => { trace.push('count'); return 3; },
        invalidServer: () => trace.push('invalid'),
        descend: (territory) => trace.push(`descend:${territory}`),
    };
    const command = 0x02000013;
    // 서버 WORD = y 높은 16비트(signed). 0..2 는 유효, -1 은 허용(서버 없음), 3 이상과 -2 이하는 거부한다.
    const y = (server, territory) => (((server & 0xffff) << 16) | (territory & 0xffff)) | 0;
    const cases = [
        [y(0, 7), ['count', 'descend:7']],
        [y(2, 0xffff), ['count', 'descend:65535']],
        [y(-1, 9), ['count', 'descend:9']],
        [y(3, 1), ['count', 'invalid']],
        [y(0x7fff, 1), ['count', 'invalid']],
        // -2 는 서버 개수를 읽지 않고 곧바로 거부한다(`server < -1 ||` 단락).
        [y(-2, 1), ['invalid']],
        [y(-32768, 1), ['invalid']],
    ];
    for (const [value, expected] of cases) {
        trace.length = 0;
        assert.equal(dispatchViewMetaInput({ code: command, x: 0, y: value }, command, hooks), true, String(value));
        assert.deepEqual(trace, expected, String(value));
    }
    // 수정키를 지운 코드로 비교하고 현재 명령이 아니면 소비하지 않으며 장치를 요구하지 않는다.
    trace.length = 0;
    assert.equal(dispatchViewMetaInput({ code: (command | InputCode.control) >>> 0, x: 0, y: y(1, 4) }, command, hooks), true);
    assert.deepEqual(trace, ['count', 'descend:4']);
    assert.equal(dispatchViewMetaInput({ code: 0x02000014, x: 0, y: 0 }, command, undefined), false);
    // 뗌은 장치 없이 소비한다.
    assert.equal(dispatchViewMetaInput({ code: (command | InputCode.release) >>> 0, x: 0, y: y(9, 9) }, command, undefined), true);
    // 선택한 분기의 장치만 요구한다: 개수·안내·Descend 중 필요한 것이 없으면 성공으로 대체하지 않는다.
    assert.throws(() => dispatchViewMetaInput({ code: command, x: 0, y: y(0, 1) }, command, {}), /미연결/);
    assert.throws(() => dispatchViewMetaInput({ code: command, x: 0, y: y(0, 1) }, command, { serverCount: () => 1 }), /미연결/);
    assert.throws(() => dispatchViewMetaInput({ code: command, x: 0, y: y(-2, 1) }, command, { serverCount: () => 1 }), /미연결/);
    // 현재 명령 값은 호출 순간의 DWORD 로 비교한다(상위 비트가 붙은 명령 값도 마스크 뒤 전체 DWORD 비교).
    assert.equal(dispatchViewMetaInput({ code: 0x02000013, x: 0, y: 0 }, 0x02000013 >>> 0, { serverCount: () => 1, invalidServer() {}, descend() {} }), true);
});
