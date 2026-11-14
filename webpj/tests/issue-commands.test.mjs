// 초기 점 명령(flash/desc/cheatorama) 등록과 콜백(RawIssueCommands)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E437)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawIssueCommands.cpp)에서 옮겼다. 치트 값 0x845fed 는 십진 8675309 이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { registerIssueCommands, invokeIssueCommand, ISSUE_FLASH_COMMAND, ISSUE_DESCRIPTION_COMMAND, ISSUE_CHEAT_COMMAND, ISSUE_CHEAT_VALUE } from '../www/ui/issue-commands.js';

// 모든 효과를 기록하는 장치 묶음이다.
function rig() { const log = []; return { log, hooks: { flash: (value) => log.push(['flash', value]), sound: (name) => log.push(['sound', name]), description: (a, b, c) => log.push(['description', a, b, c]) } }; }

test('registerIssueCommands: flash 두 번(중복 유지)·desc·cheatorama 를 이 순서/콜백/표시 값으로 등록하고 상수는 원문 번호다', () => {
    assert.equal(ISSUE_FLASH_COMMAND, 0x4d5300); assert.equal(ISSUE_DESCRIPTION_COMMAND, 0x4d5340); assert.equal(ISSUE_CHEAT_COMMAND, 0x4d5360); assert.equal(ISSUE_CHEAT_VALUE, 0x845fed); assert.equal(ISSUE_CHEAT_VALUE, 8675309);
    const calls = []; registerIssueCommands((name, callback, visible) => calls.push([name, callback, visible]));
    assert.deepEqual(calls, [['flash', 0x4d5300, 0], ['flash', 0x4d5300, 0], ['desc', 0x4d5340, 1], ['cheatorama', 0x4d5360, 1]]);
    // 등록기가 없으면 첫 호출에서 Error 이고 중간에 던지면 앞 등록은 남는다.
    assert.throws(() => registerIssueCommands(undefined), { name: 'Error', message: /선택 장치/ });
    const partial = []; assert.throws(() => registerIssueCommands((name) => { if (partial.length === 2) throw new Error('stop'); partial.push(name); }), /stop/); assert.deepEqual(partial, ['flash', 'flash']);
});

test('invokeIssueCommand flash: 인자가 있으면 atol(인자) 없으면 0 으로 flash 한 뒤 thunderQuietDistant.wav 를 요청하고 0 을 돌려준다', () => {
    const r = rig(); const cheat = { value: 0 };
    assert.equal(invokeIssueCommand(0x4d5300, '7', cheat, r.hooks), 0); assert.deepEqual(r.log, [['flash', 7], ['sound', 'thunderQuietDistant.wav']]);
    // 앞 공백/부호/숫자 뒤 글자 무시·32비트 감김·16진수 미지원·빈 문자열 있음은 모두 CRT atol 이다.
    for (const [text, expected] of [['  -12x', -12], ['+5', 5], ['0x10', 0], ['abc', 0], ['', 0], ['4294967295', -1], ['4294967296', 0], ['2147483648', -2147483648]]) { r.log.length = 0; invokeIssueCommand(0x4d5300, text, cheat, r.hooks); assert.deepEqual(r.log[0], ['flash', expected], text); }
    for (const absent of [null, undefined]) { r.log.length = 0; assert.equal(invokeIssueCommand(0x4d5300, absent, cheat, r.hooks), 0); assert.deepEqual(r.log, [['flash', 0], ['sound', 'thunderQuietDistant.wav']]); }
    assert.equal(cheat.value, 0);
});

test('invokeIssueCommand flash: 장치 누락은 도달한 위치에서만 Error 이며 flash 효과는 sound 누락 전에 이미 나갔다', () => {
    const r = rig(); assert.throws(() => invokeIssueCommand(0x4d5300, '3', { value: 0 }, { sound: r.hooks.sound }), { name: 'Error', message: /선택 장치/ }); assert.deepEqual(r.log, []);
    assert.throws(() => invokeIssueCommand(0x4d5300, '3', { value: 0 }, { flash: r.hooks.flash }), /선택 장치/); assert.deepEqual(r.log, [['flash', 3]]);
    // description 장치는 flash 경로에서 요구하지 않는다.
    invokeIssueCommand(0x4d5300, '1', { value: 0 }, { flash: r.hooks.flash, sound: r.hooks.sound });
});

test('invokeIssueCommand desc: description(-1, 인자|null, 0) 만 요청하고 빈 문자열은 있는 인자이며 flash/sound 는 요구하지 않는다', () => {
    const r = rig(); const only = { description: r.hooks.description };
    assert.equal(invokeIssueCommand(0x4d5340, '/data', { value: 0 }, only), 0); assert.equal(invokeIssueCommand(0x4d5340, '', { value: 0 }, only), 0);
    assert.equal(invokeIssueCommand(0x4d5340, null, { value: 0 }, only), 0); assert.equal(invokeIssueCommand(0x4d5340, undefined, { value: 0 }, only), 0);
    assert.deepEqual(r.log, [['description', -1, '/data', 0], ['description', -1, '', 0], ['description', -1, null, 0], ['description', -1, null, 0]]);
    assert.throws(() => invokeIssueCommand(0x4d5340, 'x', { value: 0 }, {}), { name: 'Error', message: /선택 장치/ });
});

test('invokeIssueCommand cheatorama: 인자가 없으면 TypeError, 첫 토큰의 atol 이 정확히 0x845fed 일 때만 공유 DWORD 에 저장한다', () => {
    const hooks = {};
    for (const absent of [null, undefined]) assert.throws(() => invokeIssueCommand(0x4d5360, absent, { value: 0 }, hooks), TypeError);
    // 일치: 앞 공백/부호/따옴표/뒤 글자/쉼표·탭·개행 뒤 인자/32비트 감김(8675309 + 2^32).
    for (const text of ['8675309', '  8675309', '+8675309', '"8675309"', '8675309x', '8675309,1', '8675309\t2', '8675309\n2', '8675309 extra', '4303642605', '\t8675309', '08675309x1']) { const cheat = { value: 0 }; assert.equal(invokeIssueCommand(0x4d5360, text, cheat, hooks), 0, text); assert.equal(cheat.value, 0x845fed, text); }
    // 불일치: 다른 값/16진수/음수/둘째 토큰/인용 안 공백/빈 문자열/인접 값은 저장하지 않고 기존 값을 보존한다(지우지도 않는다).
    for (const text of ['', '0', '8675308', '8675310', '0x845fed', '-8675309', '1,8675309', '1 8675309', '"867 5309"', 'x8675309', '18675309']) { const cheat = { value: 5 }; assert.equal(invokeIssueCommand(0x4d5360, text, cheat, hooks), 0, text); assert.equal(cheat.value, 5, text); }
    const kept = { value: 0x845fed }; invokeIssueCommand(0x4d5360, '1', kept, hooks); assert.equal(kept.value, 0x845fed);
});

test('invokeIssueCommand: 알려지지 않은 콜백은 null(다음 분배기로)이고 어떤 장치도 읽지 않으며 DWORD 콜백은 감겨 비교한다', () => {
    const cheat = { value: 3 };
    for (const callback of [0, 0x4d5301, 0x4d52ff, 0x4d5320, 0x4d5380, -1]) assert.equal(invokeIssueCommand(callback, 'x', cheat, {}), null, String(callback));
    assert.equal(cheat.value, 3);
    // 2^32 + 콜백은 같은 DWORD 다.
    const r = rig(); assert.equal(invokeIssueCommand(0x1004d5340, 'a', cheat, r.hooks), 0); assert.deepEqual(r.log, [['description', -1, 'a', 0]]);
});
