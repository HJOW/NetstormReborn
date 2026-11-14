// 문자 편집 Gump(TextEntryGump)의 생성/부착/스크롤/점멸/문자열 접근/소멸을 기준선 소스 계약과 손 계산 값으로 검증한다. 입력 처리/Console/Draw 는 text-entry-gump-input.test.mjs 가 맡는다. (2026-11-14 WB04-E439)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TextEntryGump.cpp)에서 옮겼다. 편집 버퍼는 실제 TextEntryStorage/TextEntryBuffer 이고 글 폭은 바이트당 8·높이 16이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { TextEntryGump, TextEntryGumpNode, ConsoleGumpNode, TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { rig, built, cstr, bytesOf } from './text-entry-gump-support.mjs';

const names = (log) => log.map((entry) => entry[0]);

test('TextEntryGumpState/노드 기본값: 종류 15/10·사건 번호·0 필드', () => {
    const state = new TextEntryGumpState(); assert.deepEqual({ ...state }, { kind: 15, consoleKind: 10, brown: 0, black: 0, yellow: 0, enterEvent: 0x02000038, changeEvent: 0x020000bf, sliderChange: 0x02000036, sliderStep: 0x0200002e });
    const node = new ConsoleGumpNode(); assert.equal(node instanceof TextEntryGumpNode, true); for (const key of ['verticalSlider', 'horizontalSlider', 'ownsBuffer', 'reservedC4', 'reserved10C', 'reserved124', 'blinkAt', 'blinkPhase', 'readOnly', 'shiftAnchor', 'shiftEnd', 'scrollLine', 'scrollColumn', 'showVertical', 'showHorizontal', 'forwardInput', 'multiline', 'changeNotice', 'recipientHighlight', 'maxLength', 'excludedChars', 'buffer', 'autocomplete', 'submit', 'resetWord', 'wordStart', 'wordLength']) assert.equal(node[key], 0, key);
});

test('생성자: 노드/기본/버퍼 조회 훅은 필수이고 같은 노드가 아니면 Error 다', () => {
    const r = rig(); const state = new TextEntryGumpState();
    for (const name of ['node', 'base', 'buffer']) assert.throws(() => new TextEntryGump(state, { ...r.hooks, [name]: undefined }), { name: 'TypeError', message: /같은 노드/ }, name);
    assert.throws(() => new TextEntryGump(state), TypeError);
    // 파생 노드와 기본 노드가 다른 객체이거나 없는 토큰은 Error 다.
    const other = new TextEntryGump(state, { ...r.hooks, base: () => new TextEntryGumpNode() }); assert.throws(() => other.setText(1, null), { name: 'Error', message: /공유 노드 없음/ }); assert.throws(() => r.gump.setText(0, null), /공유 노드 없음/); assert.throws(() => r.gump.setText(9, null), /공유 노드 없음/);
});

test('construct: 배경→필드→버퍼 구성→idle→초기 본문 Reset/Redraw→두 Slider 생성/부착→스크롤 0→전체 선택 순서로 필드를 쓴다', () => {
    const r = rig({ scale: 2 }); const node = r.root; node.flags = 0x10; node.excludedChars = 99; node.textContext.fill(7);
    assert.equal(r.gump.construct(1, 200, 60, 'hello', 0x1234, 0x9988, 0x5566), 1);
    assert.deepEqual([node.vtable, node.command, node.buffer, node.ownsBuffer, node.maxLength, node.blinkPhase, node.blinkAt], [0x508e10, 15, 300, 1, 0, 0, 100.5]);
    assert.deepEqual([node.baseWidth, node.width, node.baseHeight, node.height, node.scrollLine, node.scrollColumn], [200, 400, 60, 120, 0, 0]); assert.deepEqual(Array.from(node.textContext), [0, 0x1234, 0x5566, 0, 0, 0, 1, 1]); assert.equal(node.flags, 0x400410);
    assert.deepEqual([node.readOnly, node.forwardInput, node.changeNotice, node.multiline, node.showVertical, node.showHorizontal, node.recipientHighlight, node.excludedChars, node.verticalSlider, node.horizontalSlider], [0, 0, 0, 1, 0, 0, 0, 0, 200, 201]);
    assert.equal(r.buffer().text(), 'hello'); assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd, node.shiftAnchor, node.shiftEnd], [0, 5, 0, 5]); assert.equal(r.buffer().state().capacity, 0x800);
    // 두 Slider: 세로 10×100, 가로 100×10, 보이지 않으며 부모 우측 끝/하단 끝 기준으로 부착한다.
    assert.equal(r.sliderOf(200).flags, 0x400); assert.equal(r.sliderOf(201).flags, 0x400);
    assert.deepEqual(r.log.slice(0, 4), [['constructBackground', 1, 0x9988], ['allocate', 0x28, 300], ['idle', 1, true], ['redraw', 1, false]]);
    assert.deepEqual(r.log.filter((entry) => ['allocate', 'constructSlider', 'attach'].includes(entry[0])).slice(1), [['allocate', 0xd8, 200], ['constructSlider', 200, 10, 100, 0x11, 0x22], ['allocate', 0xd8, 201], ['constructSlider', 201, 100, 10, 0x11, 0x22], ['attach', 200, 190, 0, 1, 1], ['attach', 201, 0, 50, 1, 1]]);
    // 마지막: 스크롤 0,0 의 Slider 값 처리 뒤 Redraw 다.
    assert.deepEqual(names(r.log).slice(-7), ['sliderRange', 'sliderPage', 'sliderPosition', 'sliderRange', 'sliderPage', 'sliderPosition', 'redraw']);
});

test('construct: 초기 본문이 없으면 빈 본문이고 선택/shift 필드를 건드리지 않으며, 초기 빈 문자열은 있는 것으로 전체(0) 선택한다', () => {
    const r = rig(); r.root.shiftEnd = 9; r.gump.construct(1, 100, 40, undefined, 1, 2, 3); assert.equal(r.buffer().text(), ''); assert.deepEqual([r.buffer().state().selectionBegin, r.buffer().state().selectionEnd, r.root.shiftEnd], [0, 0, 0]);
    // construct 가 먼저 shiftEnd 를 0 으로 지운다. 초기 ''/null 구분: null 은 없음.
    const empty = rig(); empty.root.shiftEnd = 9; empty.gump.construct(1, 100, 40, '', 1, 2, 3); assert.equal(empty.root.shiftEnd, 0); assert.equal(empty.buffer().text(), '');
    const nul = rig(); nul.gump.construct(1, 100, 40, 'ab\0cd', 1, 2, 3); assert.equal(nul.buffer().text(), 'ab'); assert.equal(nul.root.shiftEnd, 2);
    const absent = rig(); absent.gump.construct(1, 100, 40, null, 1, 2, 3); assert.equal(absent.buffer().text(), '');
});

test('construct: 실제 버퍼 확보 실패는 assertion(0x31) 뒤 계속하다 현재 버퍼 없음 Error 로 멈추고, 장치 누락은 TypeError 다', () => {
    const r = rig({ failBuffer: true }); assert.throws(() => r.gump.construct(1, 100, 40, 'x', 1, 2, 3), { name: 'Error', message: /현재 편집 버퍼 없음/ }); assert.deepEqual(r.log.filter((entry) => entry[0] === 'assertion'), [['assertion', 0x31]]); assert.equal(r.root.buffer, 0); assert.equal(r.root.ownsBuffer, 1);
    for (const name of ['constructBackground', 'allocate', 'constructSlider', 'idle', 'attach', 'now']) { const x = rig(); const gump = new TextEntryGump(x.state, { ...x.hooks, [name]: undefined }); assert.throws(() => gump.construct(1, 100, 40, null, 1, 2, 3), { name: 'TypeError', message: /생성 경계/ }, name); }
});

test('constructConsole: 기본 생성 뒤 Console 종류(10)·autocomplete/제출/단어 0·재설정 1·수신 대상 강조 1 이고 Console 노드가 아니면 Error 다', () => {
    const r = rig({ console: true }); r.root.autocomplete = 5; r.root.submit = 6; r.root.wordLength = 7; r.root.wordStart = 8; r.root.resetWord = 0;
    assert.equal(r.gump.constructConsole(1, 200, 60, 'hi', 1, 2, 3), 1); assert.deepEqual([r.root.vtable, r.root.command, r.root.autocomplete, r.root.submit, r.root.wordLength, r.root.wordStart, r.root.resetWord, r.root.recipientHighlight], [0x506a6c, 10, 0, 0, 0, 0, 1, 1]); assert.equal(r.root.multiline, 1);
    const plain = rig(); assert.throws(() => plain.gump.constructConsole(1, 200, 60, null, 1, 2, 3), { name: 'Error', message: /Console 공유 파생 노드/ }); assert.deepEqual(plain.log, []);
});

test('attach: 공통 부착 뒤 두 Slider 를 Push 하고 크기를 맞추며 표시 필드대로 보이게 하고 우하단 기준 재부착한 뒤 현재 스크롤을 다시 적용한다', () => {
    const r = built({}, 'hello'); r.root.showVertical = 1; r.root.showHorizontal = 0; r.sliderOf(200).flags = 0; r.sliderOf(201).flags = 1;
    r.gump.attach(1, 5, 6, 7, 2);
    assert.deepEqual(r.log.slice(0, 5), [['baseAttach', 1, 5, 6, 7, 2], ['push', 200], ['push', 201], ['attach', 200, 190, 0, 1, 1], ['attach', 201, 0, 50, 1, 1]]);
    assert.deepEqual([r.sliderOf(200).baseHeight, r.sliderOf(200).height, r.sliderOf(201).baseWidth, r.sliderOf(201).width], [60, 60, 200, 200]); assert.deepEqual([r.sliderOf(200).flags & 1, r.sliderOf(201).flags & 1], [1, 0]);
    assert.equal(r.log.at(-1)[0], 'redraw');
    for (const name of ['baseAttach', 'push', 'attach']) assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, [name]: undefined }).attach(1, 0, 0, 0, 0), { name: 'TypeError', message: /부착 경계/ }, name);
});

test('setScroll: 줄 clamp 뒤 열 clamp 는 쓰기 전의 현재 scrollLine 줄 길이로 하고 가로→세로 Slider 순서로 범위/page/위치를 주며 되읽은 position 을 저장한다', () => {
    // 6줄: 길이 41/…, 높이 32 → 줄 page 2(32/16), 너비 200 → 열 page 25. 첫 줄은 40자+LF.
    const text = `${'x'.repeat(40)}\nb\nc\nd\ne\nf`; const r = built({}, text, 200, 32);
    r.gump.setScroll(1, 99, 100);
    // 줄 99 → 줄 수 6 이하이므로 5, 열 100 → 쓰기 전 scrollLine 0 의 줄 길이 41 이하이므로 40. 가로 범위는 쓴 뒤의 새 줄 5('f', 길이 1)로 (0,0)이다.
    assert.deepEqual(r.log.filter((entry) => entry[0].startsWith('slider')), [['sliderRange', 201, 0, 0], ['sliderPage', 201, 25], ['sliderPosition', 201, 40], ['sliderRange', 200, 0, 5], ['sliderPage', 200, 2], ['sliderPosition', 200, 5]]);
    // 되읽은 값: 가로 범위 (0,0)→(0,1) page 1 이라 열은 0, 세로 범위 (0,5) page 2 → 위치 5 는 3 으로 제한된다.
    assert.deepEqual([r.root.scrollColumn, r.root.scrollLine], [0, 3]); assert.equal(r.log.at(-1)[0], 'redraw');
    // 음수는 0 으로 올린다.
    r.gump.setScroll(1, -4, -9); assert.deepEqual([r.root.scrollLine, r.root.scrollColumn], [0, 0]);
});

test('setScroll: 열 clamp 는 새 줄이 아니라 쓰기 전 scrollLine 의 줄 길이를 쓰고 현재 줄 길이는 두 번 읽으며, Slider 값 처리 장치가 없으면 TypeError 다', () => {
    const r = built({}, `a\n${'l'.repeat(14)}\nz`); r.root.scrollLine = 1;
    r.gump.setScroll(1, 0, 10);
    // 줄 1 의 길이(15)가 10 보다 크므로 열은 10 그대로 가로 Slider 에 전달된다(새 줄 0 은 길이 2).
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), [['sliderPosition', 201, 10]]);
    // 가로 범위는 새 줄 0 의 길이 2 로 (0, 1) 이다.
    assert.deepEqual(r.log.find((entry) => entry[0] === 'sliderRange' && entry[1] === 201), ['sliderRange', 201, 0, 1]);
    const clamped = built({}, `a\n${'l'.repeat(14)}\nz`); clamped.root.scrollLine = 1; clamped.gump.setScroll(1, 0, 99); assert.deepEqual(clamped.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 14]);
    for (const name of ['sliderRange', 'sliderPage', 'sliderPosition']) assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, [name]: undefined }).setScroll(1, 0, 0), { name: 'TypeError', message: /Slider 값 처리/ }, name);
});

test('setScroll/relativeScroll: 글 크기 0 이면 FTOL 이 0 page 를 주고 delta 는 현재 값에 signed 감김 덧셈한다', () => {
    const r = built({}, 'abcdef'); r.hooks.measure = () => [0, 0]; const gump = new TextEntryGump(r.state, r.hooks); gump.setScroll(1, 0, 0); assert.deepEqual(r.log.filter((entry) => entry[0] === 'sliderPage'), [['sliderPage', 201, 0], ['sliderPage', 200, 0]]);
    const s = built({}, `${'x'.repeat(40)}\nb\nc\nd\ne\nf`, 200, 32); s.root.scrollLine = 1; s.root.scrollColumn = 2; s.log.length = 0; s.gump.relativeScroll(1, 1, 3); assert.deepEqual(s.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 200), ['sliderPosition', 200, 2]); assert.deepEqual(s.log.find((entry) => entry[0] === 'sliderPosition' && entry[1] === 201), ['sliderPosition', 201, 1]);
    // 감김: INT_MAX + 1 → INT_MIN → 음수 clamp 로 0.
    s.root.scrollLine = 0x7fffffff; s.log.length = 0; s.gump.relativeScroll(1, 1, 0); assert.equal(s.root.scrollLine, 0);
});

test('idle: dead/읽기 전용이면 무효과이고 시각이 점멸 시각 이상일 때만 위상+1·Redraw·(두 번째 시각+0.5)로 다음 점멸을 예약한다', () => {
    const times = [100.5, 101, 5]; let at = 0; const r = built({ clock: () => times[Math.min(at++, times.length - 1)] }); at = 0; r.root.blinkAt = 100.5; r.root.blinkPhase = 3;
    // 첫 now()=100.5 >= blinkAt, Redraw 뒤 두 번째 now()=101 → blinkAt 101.5.
    r.gump.idle(1); assert.deepEqual([r.root.blinkPhase, r.root.blinkAt], [4, 101.5]); assert.deepEqual(r.log, [['redraw', 1, false]]);
    // 아직 시각이 안 되었으면 아무것도 하지 않는다(now 한 번만 읽는다).
    r.log.length = 0; at = 2; r.root.blinkAt = 200; r.gump.idle(1); assert.deepEqual(r.log, []); assert.equal(r.root.blinkPhase, 4);
    const dead = built({}); dead.root.flags |= 0x10000; dead.root.blinkAt = 0; dead.gump.idle(1); assert.deepEqual(dead.log, []);
    const readOnly = built({}); readOnly.root.readOnly = 1; readOnly.root.blinkAt = 0; readOnly.gump.idle(1); assert.deepEqual(readOnly.log, []);
    // 시계가 없어도 dead/읽기 전용은 요구하지 않고 활성 노드에서만 TypeError 다. 위상은 32비트로 감긴다.
    const noClock = built({}); noClock.root.readOnly = 1; new TextEntryGump(noClock.state, { ...noClock.hooks, now: undefined }).idle(1); noClock.root.readOnly = 0; assert.throws(() => new TextEntryGump(noClock.state, { ...noClock.hooks, now: undefined }).idle(1), { name: 'TypeError', message: /점멸 시계/ });
    const wrap = built({}); wrap.root.blinkAt = 0; wrap.root.blinkPhase = 0xffffffff; wrap.gump.idle(1); assert.equal(wrap.root.blinkPhase, 0);
});

test('setText/appendText/copyText: Reset+Redraw, 끝 삽입(선택 복구는 요청 때만), 실제 CopyAll 위임이다', () => {
    const r = built({}, 'abc'); r.gump.setText(1, bytesOf('xy')); assert.equal(r.buffer().text(), 'xy'); assert.deepEqual(r.log, [['redraw', 1, false]]); r.gump.setText(1, null); assert.equal(r.buffer().text(), '');
    const a = built({}, 'abc'); a.buffer().select(1, 2); a.gump.appendText(1, bytesOf('!!')); assert.equal(a.buffer().text(), 'abc!!'); assert.deepEqual([a.buffer().state().selectionBegin, a.buffer().state().selectionEnd], [5, 5]);
    a.buffer().select(1, 2); a.gump.appendText(1, bytesOf('?'), true); assert.equal(a.buffer().text(), 'abc!!?'); assert.deepEqual([a.buffer().state().selectionBegin, a.buffer().state().selectionEnd], [1, 2]);
    const out = new Uint8Array(16); assert.equal(a.gump.copyText(1, out), 6); assert.equal(cstr(out), 'abc!!?'); assert.equal(a.gump.copyText(1, new Uint8Array(3)), 0xffffffff);
});

test('setExcluded: 이전 소유를 먼저 반납하고 null 이면 비우며 새 문자열의 LF 를 지운다(지운 자리를 다시 검사)', () => {
    const r = built({}); r.gump.setExcluded(1, 'a\n\nb\nc'); const first = r.root.excludedChars; assert.ok(first); assert.equal(cstr(r.storage.bytes(first)), 'abc');
    r.log.length = 0; r.gump.setExcluded(1, 'xyz'); assert.deepEqual(r.log.filter((entry) => entry[0] === 'releaseBytes'), [['releaseBytes', first]]); assert.equal(cstr(r.storage.bytes(r.root.excludedChars)), 'xyz');
    r.log.length = 0; r.gump.setExcluded(1, null); assert.equal(r.root.excludedChars, 0); assert.deepEqual(names(r.log), ['releaseBytes']); r.log.length = 0; r.gump.setExcluded(1, undefined); assert.deepEqual(r.log, []);
    r.gump.setExcluded(1, ''); assert.equal(cstr(r.storage.bytes(r.root.excludedChars)), '');
    // 확보 실패(0)는 assertion(0x85) 뒤 원문처럼 0 주소 조회에서 멈춘다.
    const bad = built({}); const gump = new TextEntryGump(bad.state, { ...bad.hooks, allocateBytes: () => 0 }); assert.throws(() => gump.setExcluded(1, 'q'), RangeError); assert.deepEqual(bad.log.filter((entry) => entry[0] === 'assertion'), [['assertion', 0x85]]);
    for (const name of ['allocateBytes', 'bytes']) assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, [name]: undefined }).setExcluded(1, 'q'), { name: 'TypeError', message: /제외문자 실제 소유/ }, name);
    const small = built({}); assert.throws(() => new TextEntryGump(small.state, { ...small.hooks, bytes: () => new Uint8Array(1) }).setExcluded(1, 'abc'), { name: 'Error', message: /제외문자 할당\/범위/ });
    const noRelease = built({}); noRelease.root.excludedChars = 5; assert.throws(() => new TextEntryGump(noRelease.state, { ...noRelease.hooks, releaseBytes: undefined }).setExcluded(1, null), { name: 'TypeError', message: /제외문자 반납/ });
});

test('setHorizontal: 필드를 쓰고 void 가 아니면 Push 뒤 현재 x/y 로 재부착하며 void 이면 필드만 쓴다', () => {
    const r = built({}); r.root.x = 3; r.root.y = 4; r.root.flags = 0; r.gump.setHorizontal(1, 1); assert.equal(r.root.showHorizontal, 1); assert.deepEqual(r.log, [['push', 1], ['attach', 1, 3, 4, 0, 0]]);
    r.log.length = 0; r.root.flags = 8; r.gump.setHorizontal(1, 0); assert.equal(r.root.showHorizontal, 0); assert.deepEqual(r.log, []);
    r.root.flags = 0; assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, push: undefined }).setHorizontal(1, 1), { name: 'TypeError', message: /표시 재부착/ });
});

test('destruct: 소유한 버퍼만 Destroy→객체 반납하고 기본 소멸자를 부르며 빌린 버퍼는 건드리지 않고 Console 도 같은 경로다', () => {
    const r = built({}, 'abc'); assert.equal(r.storage.allocationCount(), 2); r.gump.destruct(1); assert.deepEqual(r.log, [['releaseBuffer', 300], ['destructBase', 1]]); assert.equal(r.root.buffer, 0); assert.equal(r.root.vtable, 0x508e10); assert.equal(r.storage.allocationCount(), 0);
    const borrowed = built({}, 'abc'); borrowed.root.ownsBuffer = 0; borrowed.gump.destruct(1); assert.deepEqual(borrowed.log, [['destructBase', 1]]); assert.equal(borrowed.root.buffer, 300);
    const c = built({ console: true }, 'abc'); c.gump.destruct(1, true); assert.equal(c.root.vtable, 0x508e10); assert.deepEqual(c.log, [['releaseBuffer', 300], ['destructBase', 1]]);
    const plain = built({}); assert.throws(() => plain.gump.destruct(1, true), { name: 'Error', message: /Console 공유 파생 노드/ });
    assert.throws(() => new TextEntryGump(r.state, { ...r.hooks, destructBase: undefined }).destruct(1), { name: 'TypeError', message: /기본 소멸자/ });
    const noRelease = built({}); assert.throws(() => new TextEntryGump(noRelease.state, { ...noRelease.hooks, releaseBuffer: undefined }).destruct(1), { name: 'TypeError', message: /버퍼 객체 반납/ });
});
