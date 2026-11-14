// 전용 PE fixture가 없는 StyleText 대괄호 `~[...]` 명령을 기준선 소스 계약·독립 구현(picture-resolver)·실제 자산과 대조한다. (2026-11-14 WB04-E421)
// cpppj에는 StyleTextOffsetAllowed 시험(KnownBugTests, NB02)뿐이라 나머지는 원본 소스의 검색 순서/재조회 순서를 시험으로 고정한 것이며 원본 직접 관측 수치가 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { StyleTextMarkup, StyleTextMarkupState, emptyStyleTextMarkupType, styleTextOffsetAllowed, bindStyleTextMarkupAssets, bindStyleTextMarkupPalette, bindStyleTextMarkupSound, loadStyleTextMarkupOptions } from '../www/ui/style-text-markup.js';
import { Markup, MarkupState, MarkupAssets, bindMarkup, bindMarkupAssets } from '../www/ui/markup-picture.js';
import { StyleText, StyleTextState, styleTextField, bindStyleTextFonts } from '../www/ui/style-text-fragments.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GameData } from '../www/assets/game-data.js';
import { createPictureResolver, parseCrtInteger } from '../www/assets/picture-resolver.js';
import { GumpText } from '../www/render/gump-text.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { stoneFonts } from './stone-button-support.mjs';

// 공통 소유 필드(head) 한 칸과 8DWORD 문맥 도우미다.
const owner = () => styleTextField({ head: 0 }, 'head');
const words = (values = []) => Uint32Array.from(Array.from({ length: 8 }, (_, i) => values[i] ?? 0));
// 소유 필드 head 에서 시작하는 조각 토큰 체인이다.
function chain(fragments, head) { const out = []; for (let token = head.value; token; token = fragments.fragments.get(token).next) out.push(token); return out; }
// 타입 한 칸의 그림 선택 필드 보기를 만든다(글자 표의 빈 칸은 -1).
function view(fields = {}) { return { ...emptyStyleTextMarkupType(), ...fields }; }

// 가상 타입 표·이름 표·소리 기록을 가진 독립 장치다. 그림 표(Markup)는 130×120 헤더 한 종류만 공급해 `#` 맞춤이 항상 0.5 배율이 된다.
function rig(options = {}) {
    const fragments = new StyleTextState(); const pictureState = new MarkupState(); pictureState.typeCount = 200;
    const shape = { token: 50, shape: 500, frameCount: 100, hotspotX: 0, hotspotY: 0 }; const header = { width: 130, height: 120, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 };
    const pictures = new Markup(fragments, pictureState, { type: () => shape, frame: () => header });
    const state = new StyleTextMarkupState(); const types = new Map(); const names = new Map(); const calls = []; const sounds = [];
    const hooks = { type: (type) => { calls.push(type); return types.get(type) ?? emptyStyleTextMarkupType(); }, findType: (name) => { calls.push(name); return names.get(name) ?? -1; }, black: () => 0x21, sound: (name) => sounds.push(name), typeCount: () => 200, frameHeight: () => 12, ...options.hooks };
    const markup = new StyleTextMarkup(fragments, pictures, state, hooks);
    // 한 명령을 적용해 문맥/태그/체인을 돌려준다.
    const run = (command, initial = {}) => { const context = words(initial.context); const head = owner(); const tag = { value: initial.tag ?? 0 }; markup.apply(command, context, head, tag); return { context, head, tag, tokens: chain(fragments, head) }; };
    return { fragments, pictureState, pictures, state, types, names, calls, sounds, hooks, markup, run, header };
}

test('StyleTextOffsetAllowed 는 두 좌표가 모두 -4..4 안일 때만 허용한다(cpppj KnownBugTests NB02 대응과 격자 전수)', () => {
    assert.ok(styleTextOffsetAllowed(0, 0) && styleTextOffsetAllowed(-4, 4) && styleTextOffsetAllowed(4, -4));
    assert.ok(!styleTextOffsetAllowed(5, 0) && !styleTextOffsetAllowed(0, -5));
    assert.ok(!styleTextOffsetAllowed(0, 1000) && !styleTextOffsetAllowed(1000, 0));
    assert.ok(!styleTextOffsetAllowed(-1000, 1000));
    // 모든 칸에서 한 축씩 독립으로 확인한다(두 축을 함께 벗어나는 칸이 한 축 조건 변형을 가리지 않도록).
    for (let x = -7; x <= 7; x += 1) for (let y = -7; y <= 7; y += 1) assert.equal(styleTextOffsetAllowed(x, y), Math.abs(x) <= 4 && Math.abs(y) <= 4, `${x},${y}`);
});

test('T 형식: 대소문자 구분 부분 검색·숫자만 캡처·ASCII 공백만 건너뜀·DWORD 감김과 unsigned 태그', () => {
    const s = rig();
    assert.equal(s.run('ab T  42 x').tag.value, 42);
    // 소문자·부호·탭은 T 형식이 아니다.
    for (const text of ['t 5', 'T-5', 'T\t9', 'T', 'T x']) assert.equal(s.run(text, { tag: 3 }).tag.value, 3, text);
    // 첫 T 가 실패하면 뒤의 T 후보로 넘어간다.
    assert.equal(s.run('TT7').tag.value, 7);
    assert.equal(s.run('T abT 9').tag.value, 9);
    // DWORD 감김과 unsigned 태그 저장이다.
    assert.equal(s.run('T4294967297').tag.value, 1);
    assert.equal(s.run('T99999999999').tag.value, 1215752191);
    assert.equal(s.run('T4294967295').tag.value, 0xffffffff);
    // 숫자 캡처는 첫 80바이트만 쓴다(80자리 0 뒤의 숫자는 무시).
    assert.equal(s.run(`T${'0'.repeat(80)}9`).tag.value, 0);
    assert.equal(s.run(`T${'0'.repeat(79)}9`).tag.value, 9);
});

test('S/C/B 공통 ` *. *(캡처)`: 점은 임의 글자·공백 되돌리기·캡처는 점/CR/LF/공백 제외 탭 포함·첫 NUL 에서 끊김', () => {
    const s = rig();
    const sound = (text) => { s.sounds.length = 0; s.run(text); return s.sounds.slice(); };
    assert.deepEqual(sound('S.click'), ['click']);
    // 점은 이스케이프되지 않아 임의 글자 한 개를 소비한다.
    assert.deepEqual(sound('SXabc'), ['abc']);
    assert.deepEqual(sound('S  . x'), ['x']);
    // 공백 반복을 되돌려 마지막 공백이 점 자리를 맡는다.
    assert.deepEqual(sound('S  x'), ['x']);
    assert.deepEqual(sound('S x'), ['x']);
    // 캡처는 점/CR/LF/공백에서 끝나고 탭은 포함한다.
    assert.deepEqual(sound('S.snd.wav'), ['snd']);
    assert.deepEqual(sound('S.a\tb c'), ['a\tb']);
    assert.deepEqual(sound('S.a\rb'), ['a']);
    assert.deepEqual(sound('S.a\nb'), ['a']);
    // 점 자리가 CR/LF 여도 성립한다.
    assert.deepEqual(sound('S\rnext'), ['next']);
    assert.deepEqual(sound('S\nnext'), ['next']);
    // 캡처가 없으면 성립하지 않는다.
    for (const text of ['S', 'S.', 'S..', 'S  ', 'S   ', 'S    ', 'S     ', 's.x']) assert.deepEqual(sound(text), [], text);
    assert.deepEqual(sound('S   x'), ['x']); assert.deepEqual(sound('S    x'), ['x']); assert.deepEqual(sound('S .  x'), ['x']);
    // 첫 NUL 뒤는 읽지 않는다.
    assert.deepEqual(sound('S.snd\0S.other'), ['snd']);
    assert.deepEqual(sound('\0S.snd'), []);
    // 캡처는 첫 80바이트만 소리 이름으로 쓴다.
    assert.deepEqual(sound(`S.${'a'.repeat(100)}`), ['a'.repeat(80)]);
});

test('명령 우선순위 I→S→T→C→B→E: 처음 성립한 형식 하나만 처리하고 비활성 옵션도 소비하며 모르는 형식은 무시', () => {
    const s = rig();
    // 텍스트에서 T 가 앞서도 S 가 먼저 검사된다.
    const first = s.run('T5 S.snd', { tag: 1 }); assert.deepEqual(s.sounds, ['snd']); assert.equal(first.tag.value, 1);
    // 같은 이유로 C 는 T 에 가려지고 E 는 B 에 가려진다.
    assert.equal(s.run('C.9 T6').tag.value, 6); assert.equal(s.run('C.9 T6').context[2], 0);
    const bgc = s.run('E1.2 B.7'); assert.equal(bgc.context[5], 7); assert.equal(bgc.context[0], 0);
    // 옵션이 꺼져도 성립한 형식은 소비하고 뒤 형식으로 넘어가지 않는다.
    s.state.allowSounds = 0; s.sounds.length = 0; const consumed = s.run('S.snd T5'); assert.deepEqual(s.sounds, []); assert.equal(consumed.tag.value, 0);
    s.state.allowForegroundColor = 0; assert.equal(s.run('C.9 B.4').context[5], 0);
    // 모르는 형식과 빈 명령은 문맥/태그/체인을 바꾸지 않는다.
    for (const text of ['Zzz', '', '  ', 'X.1']) { const r = s.run(text, { tag: 8, context: [1, 2, 3, 4, 5, 6, 7, 8] }); assert.deepEqual(Array.from(r.context), [1, 2, 3, 4, 5, 6, 7, 8]); assert.equal(r.tag.value, 8); assert.deepEqual(r.tokens, []); }
});

test('C/B/E 옵션: 문맥 칸·허용 옵션 0·E 형식은 `E<x>.<y>`(E.x.y 는 불성립)·검정 별칭/비트/좌표·NB02 두 좌표 조건', () => {
    const s = rig();
    const fg = s.run('C.255'); assert.deepEqual(Array.from(fg.context), [0, 0, 255, 0, 0, 0, 0, 0]);
    assert.equal(s.run('C.4294967295').context[2], 0xffffffff);
    const bg = s.run('B.7', { context: [9, 9, 9, 9, 9, 9, 9, 9] }); assert.deepEqual(Array.from(bg.context), [9, 9, 9, 9, 9, 7, 9, 9]);
    s.state.allowBackgroundColor = 0; assert.equal(s.run('B.7').context[5], 0); s.state.allowBackgroundColor = 1;
    // E: 그림자 비트 2 를 더하고 현재 기본 검정/좌표를 저장한다. 음수 좌표는 unsigned 로 저장된다.
    assert.deepEqual(Array.from(s.run('E2.3', { context: [1, 0, 0, 0, 0, 5, 0, 0] }).context), [3, 0, 0, 0, 0, 0x21, 2, 3]);
    assert.deepEqual(Array.from(s.run('E -1 . -4').context), [2, 0, 0, 0, 0, 0x21, 0xffffffff, 0xfffffffc]);
    // 첫 캡처가 점으로 시작할 수 없으므로 문서의 `E.x.y` 표기는 성립하지 않는다.
    assert.deepEqual(Array.from(s.run('E.2.3').context), [0, 0, 0, 0, 0, 0, 0, 0]);
    // NB02: 한 좌표만 범위 안이어도 적용하던 원문 악용을 거부한다.
    for (const text of ['E5.0', 'E0.5', 'E0.1000', 'E1000.0', 'E-5.-5']) assert.deepEqual(Array.from(s.run(text).context), [0, 0, 0, 0, 0, 0, 0, 0], text);
    assert.equal(s.run('E4.-4').context[0], 2); assert.equal(s.run('E-4.4').context[0], 2);
    s.state.allowOffset = 0; assert.equal(s.run('E1.1').context[0], 0);
    // 검정 장치는 실제 도달할 때만 요구한다: 범위 밖/비활성이면 필요 없고, 도달했는데 없으면 비트를 더하기 전에 거부한다.
    const bare = rig({ hooks: { black: undefined } }); bare.state.allowOffset = 0; bare.run('E1.1'); bare.state.allowOffset = 1; bare.run('E9.9');
    const ctx = words(); assert.throws(() => bare.markup.apply('E1.1', ctx, owner(), { value: 0 }), /검정색 경계/); assert.equal(ctx[0], 0);
    // 검정 훅이 던지면 이미 더한 그림자 비트가 남는다(원문 순서: 비트 → 색).
    const bomb = rig({ hooks: { black: () => { throw new Error('검정 실패'); } } }); const c2 = words([0, 0, 0, 0, 0, 77]); assert.throws(() => bomb.markup.apply('E1.1', c2, owner(), { value: 0 }), /검정 실패/); assert.equal(c2[0], 2); assert.equal(c2[5], 77);
});

test('S 소리: 허용 옵션이 0이 아니면 80바이트 캡처 이름으로 요청하고 도달했는데 장치가 없으면 거부', () => {
    const s = rig({ hooks: { sound: undefined } });
    // 옵션이 꺼져 있으면 장치가 없어도 형식만 소비한다.
    s.state.allowSounds = 0; s.run('S.snd');
    // 0 이 아닌 값은 모두 켜진 것이다(bool 로 자르지 않음).
    s.state.allowSounds = 7; assert.throws(() => s.run('S.snd'), /소리 경계/);
    const names = []; bindStyleTextMarkupSound(s.hooks, { playByName: (...args) => names.push(args) }); s.run('S.drop');
    assert.deepEqual(names, [['drop', 0, 0, 0, 0, 0]]);
});

test('I 그림: 숫자/이름 타입·0 이름 재조회·-1 건너뜀·80바이트 규칙·현재 태그와 체인 추가', () => {
    const s = rig(); s.types.set(5, view({ frameCount: 20, defaultFrame: 4, gumpFrame: 8, letterKinds: 3, first: [10, 14, 17, ...new Array(13).fill(-1)] })); s.names.set('foo', 5);
    const picture = (r, index = 0) => { const token = r.tokens[index]; return { ...s.pictureState.pictures.get(token), metadata: s.fragments.fragments.get(token).metadata, kind: s.fragments.fragments.get(token).kind }; };
    // 숫자 타입은 이름 조회를 하지 않는다.
    s.calls.length = 0; let r = s.run('I5.4', { tag: 0x87654321 }); assert.equal(r.tokens.length, 1); assert.deepEqual(picture(r), { type: 5, frame: 4, scale: 1, mapped: 0, metadata: 0x87654321, kind: 1 }); assert.ok(!s.calls.includes('5'));
    // 공백 허용 형식과 두 번째 캡처의 점 자리.
    assert.equal(picture(s.run('I 5 . 6')).frame, 6); assert.equal(picture(s.run('I5 .7')).frame, 7); assert.equal(picture(s.run('I5 7')).frame, 7);
    // 이름이거나 숫자 0 이면 이름으로 다시 조회한다.
    s.calls.length = 0; assert.equal(picture(s.run('Ifoo.2')).type, 5); assert.ok(s.calls.includes('foo'));
    s.calls.length = 0; assert.equal(s.run('I0.2').tokens.length, 0); assert.ok(s.calls.includes('0'));
    s.names.set('0', 5); assert.equal(picture(s.run('I0.2')).type, 5);
    // 없는 이름(-1)과 감겨서 -1 이 되는 숫자는 그림을 만들지 않는다.
    assert.equal(s.run('Inope.1').tokens.length, 0); assert.equal(s.run('I4294967295.1').tokens.length, 0); assert.equal(s.run('I-1.1').tokens.length, 0);
    // 이름 장치는 실제 도달할 때만 요구한다.
    const bare = rig({ hooks: { findType: undefined } }); bare.types.set(5, view({ frameCount: 9 })); assert.equal(bare.run('I5.1').tokens.length, 1); assert.throws(() => bare.run('Ifoo.1'), /타입 이름 경계/);
    // 타입 장치 없이 그림을 만들 수 없다.
    const none = rig({ hooks: { type: undefined } }); assert.throws(() => none.run('I5.1'), /현재 타입 경계/);
    // 이름 캡처는 첫 80바이트만 조회에 쓴다.
    s.calls.length = 0; s.run(`I${'a'.repeat(100)}.1`); assert.ok(s.calls.includes('a'.repeat(80))); assert.ok(!s.calls.includes('a'.repeat(100)));
    // 프레임 식은 79바이트까지 허용하고 80바이트부터 원본 지역 배열 범위 초과로 거부한다.
    assert.equal(picture(s.run(`I5.1${'x'.repeat(78)}`)).frame, 1); assert.throws(() => s.run(`I5.1${'x'.repeat(79)}`), RangeError); assert.throws(() => s.run(`I5.1${'x'.repeat(200)}`), /지역 배열/);
});

test('I 프레임 식: `*`/`#`/`&` 선택·글자 구간 signed 비교·suffix 덧셈 감김·0 미만/count 이상 클램프', () => {
    const s = rig(); s.types.set(5, view({ frameCount: 20, defaultFrame: 4, gumpFrame: 8, letterKinds: 3, first: [10, 14, 17, ...new Array(13).fill(-1)] })); s.types.set(6, view({ frameCount: 20, defaultFrame: 4, gumpFrame: -1 })); s.types.set(10, view({ frameCount: 20, defaultFrame: 4, gumpFrame: -2 }));
    const frameOf = (command) => { const r = s.run(command); assert.equal(r.tokens.length, 1, command); return s.pictureState.pictures.get(r.tokens[0]); };
    // `*`: gumpFrame, -1 이면 defaultFrame. 뒤 글자는 읽지 않는다.
    assert.equal(frameOf('I5.*').frame, 8); assert.equal(frameOf('I6.*').frame, 4); assert.equal(frameOf('I5.*99').frame, 8);
    // -2 는 -1(없음)과 다르므로 기본 프레임으로 돌아가지 않고 음수 클램프로 0 이 된다.
    assert.equal(frameOf('I10.*').frame, 0);
    // `#`: 같은 프레임 + 65×60 상자 맞춤(130×120 헤더 → 0.5), `&`: 맞춤 + 색 치환 BYTE 1.
    let p = frameOf('I5.#'); assert.deepEqual([p.frame, p.scale, p.mapped], [8, 0.5, 0]); p = frameOf('I5.&'); assert.deepEqual([p.frame, p.scale, p.mapped], [8, 0.5, 1]); p = frameOf('I5.*'); assert.deepEqual([p.scale, p.mapped], [1, 0]);
    // 맞춤 상자는 현재 옵션 상태를 쓴다(작은 상자일수록 배율이 작아진다).
    s.state.fitWidth = 13; s.state.fitHeight = 600; assert.equal(frameOf('I5.#').scale, Math.fround(0.1)); s.state.fitWidth = 65; s.state.fitHeight = 60;
    // 글자 구간: A..C 만 글자 식이고 첫 인덱스에 숫자 suffix 를 더한다.
    assert.equal(frameOf('I5.A').frame, 10); assert.equal(frameOf('I5.B3').frame, 17); assert.equal(frameOf('I5.C0').frame, 17 + 0); assert.equal(frameOf('I5.A-2').frame, 8);
    // 글자 구간 밖 글자·소문자·비ASCII(signed 음수)는 숫자 식 전체로 읽어 0 이다.
    assert.equal(frameOf('I5.D2').frame, 0); assert.equal(frameOf('I5.b2').frame, 0); assert.equal(frameOf('I5.\xc12').frame, 0); assert.equal(frameOf('I5.@2').frame, 0);
    // suffix 덧셈은 DWORD 로 감기고 클램프가 0 이나 count 로 돌린다.
    assert.equal(frameOf('I5.A4294967295').frame, 9); assert.equal(frameOf('I5.A4294967290').frame, 4);
    // 0 미만은 0, count 이상은 count(count-1 이 아님), 바로 아래는 그대로다.
    assert.equal(frameOf('I5.-3').frame, 0); assert.equal(frameOf('I5.19').frame, 19); assert.equal(frameOf('I5.20').frame, 20); assert.equal(frameOf('I5.21').frame, 20); assert.equal(frameOf('I5.99999').frame, 20);
    // 프레임이 없는 타입(count 0)에서는 0 도 count 와 같아 0 이고, 음수 count 는 0 으로 돌린다.
    s.types.set(7, view({ frameCount: 0 })); assert.equal(frameOf('I7.5').frame, 0); s.types.set(11, view({ frameCount: 6, letterKinds: 0 })); assert.equal(frameOf('I11.A3').frame, 0); assert.equal(frameOf('I11.3').frame, 3); s.types.set(8, view({ frameCount: -1 })); assert.equal(frameOf('I8.5').frame, 0);
    // 글자 종류 수가 범위 밖이면 글자 식에서만 거부한다(숫자 식은 이 값을 읽지 않는다).
    s.types.set(9, view({ frameCount: 5, letterKinds: 17 })); assert.throws(() => s.run('I9.A'), /글자 표 범위/); s.types.set(9, view({ frameCount: 5, letterKinds: -1 })); assert.throws(() => s.run('I9.A'), /글자 표 범위/); assert.equal(frameOf('I9.3').frame, 3);
    s.types.set(9, view({ frameCount: 5, letterKinds: 16, first: new Array(16).fill(1) })); assert.equal(frameOf('I9.P1').frame, 2);
});

test('I 타입 helper 는 패치 순서대로 호출마다 다시 조회한다(`*` 5회·글자 5회·음수 3회)', () => {
    const sequence = (views) => { const s = rig(); let n = 0; s.hooks.type = (type) => { s.calls.push(type); return views[Math.min(n++, views.length - 1)]; }; return s; };
    // `*`: gump → (-1 이면 default / 아니면 gump 재조회) → count → frame>=count 확인 → 대입 재조회.
    let s = sequence([view({ gumpFrame: -1 }), view({ defaultFrame: 9 }), view({ frameCount: 20 }), view({ frameCount: 5 }), view({ frameCount: 6 })]); s.calls.length = 0;
    let r = s.run('I5.*'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 6); assert.deepEqual(s.calls, [5, 5, 5, 5, 5]);
    s = sequence([view({ gumpFrame: 4 }), view({ gumpFrame: 11 }), view({ frameCount: 20 })]); r = s.run('I5.*'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 11); assert.equal(s.calls.length, 4);
    // 글자 식: 글자 종류 조회 뒤 첫 인덱스를 다시 조회한다.
    s = sequence([view({ letterKinds: 3, first: [100] }), view({ first: [30, 40] }), view({ frameCount: 90 }), view({ frameCount: 90 })]); r = s.run('I5.A2'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 32); assert.equal(s.calls.length, 4);
    // 숫자 식은 count 조회부터 시작하고 음수는 count 한 번만 조회한다.
    s = sequence([view({ frameCount: 20 })]); s.run('I5.-1'); assert.equal(s.calls.length, 1);
    s = sequence([view({ frameCount: 20 })]); s.run('I5.3'); assert.equal(s.calls.length, 2);
    s = sequence([view({ frameCount: 20 }), view({ frameCount: 7 }), view({ frameCount: 6 })]); r = s.run('I5.7'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 6); assert.equal(s.calls.length, 3);
    s = sequence([view({ frameCount: 20 }), view({ frameCount: 8 }), view({ frameCount: 6 })]); r = s.run('I5.7'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 7); assert.equal(s.calls.length, 2);
    s = sequence([view({ frameCount: 20 }), view({ frameCount: 3 }), view({ frameCount: 2 })]); r = s.run('I5.7'); assert.equal(s.pictureState.pictures.get(r.tokens[0]).frame, 2); assert.equal(s.calls.length, 3);
});

test('StyleText.parse 통합: T 태그는 문자열·그림·리터럴 `~~`·마지막 조각으로 이어지고 `~.` 은 문맥만 복구한다', () => {
    const s = rig(); s.types.set(5, view({ frameCount: 9 }));
    const text = new StyleText(s.fragments, s.markup.hooks({})); const head = owner();
    text.parse(head, 'a~[T7]b~[I5.1]c~~d~.e~[C.9]f', words([0, 0, 1]));
    const parts = chain(s.fragments, head).map((token) => { const n = s.fragments.fragments.get(token); return [n.kind === 1 ? 'pic' : Buffer.from(n.text.subarray(0, n.text.length - 1)).toString('latin1'), n.metadata, n.context[2]]; });
    assert.deepEqual(parts, [['a', 0, 1], ['b', 7, 1], ['pic', 7, 0], ['c', 7, 1], ['~', 7, 1], ['d', 7, 1], ['e', 7, 1], ['f', 7, 9]]);
    // 위 'pic' 의 문맥 칸은 그림에 없어 0 이다. hooks() 는 원래 묶음을 바꾸지 않고 기존 훅을 보존한다.
    const base = { textMetrics: () => [1, 1] }; const joined = s.markup.hooks(base); assert.equal(base.bracket, undefined); assert.equal(joined.textMetrics, base.textMetrics); assert.equal(typeof joined.bracket, 'function');
});

test('다중 줄 그림(applyScrolling): `*`/`#` 만 특수·`#` suffix 맞춤·숫자 타입 상한·제한 모드 검사와 옵션 위임', () => {
    const s = rig(); s.types.set(5, view({ frameCount: 20, defaultFrame: 4, gumpFrame: 8, letterKinds: 3, first: [10, 14, 17, ...new Array(13).fill(-1)] })); s.names.set('foo', 5);
    let restricted = false; const added = []; const append = (...args) => added.push(args); const context = words(); const tag = { value: 0 };
    const scroll = (command) => { added.length = 0; s.markup.applyScrolling(command, context, tag, () => restricted, append); return added.map((a) => a.slice()); };
    assert.deepEqual(scroll('I5.4'), [[5, 4, 0, 0]]); assert.deepEqual(scroll('I5.*'), [[5, 8, 0, 0]]);
    s.types.set(6, view({ frameCount: 20, defaultFrame: 4, gumpFrame: -1 })); s.types.set(10, view({ frameCount: 20, defaultFrame: 4, gumpFrame: -2 })); assert.deepEqual(scroll('I6.*'), [[6, 4, 0, 0]]); assert.deepEqual(scroll('I10.*'), [[10, 0, 0, 0]]); assert.deepEqual(scroll('Ifoo.B1'), [[5, 15, 0, 0]]);
    // `#` 는 접두로 소비된 뒤 suffix 에 남은 `#` 가 맞춤을 결정한다(`*#` 이 맞춤, `#` 단독은 접두만 소비).
    assert.deepEqual(scroll('I5.*#'), [[5, 8, 65, 60]]); assert.deepEqual(scroll('I5.#'), [[5, 8, 0, 0]]); assert.deepEqual(scroll('I5.##'), [[5, 8, 65, 60]]);
    s.state.fitWidth = 64; s.state.fitHeight = 59; assert.deepEqual(scroll('I5.*#'), [[5, 8, 64, 59]]); s.state.fitWidth = 65; s.state.fitHeight = 60; assert.deepEqual(scroll('I5.&'), [[5, 0, 0, 0]]);
    // 클램프(count 이상은 count)와 0 미만 0.
    assert.deepEqual(scroll('I5.30'), [[5, 20, 0, 0]]); assert.deepEqual(scroll('I5.-3'), [[5, 0, 0, 0]]);
    // 숫자 타입은 현재 타입 수 이상이면 그림 없음(이름 타입은 이 상한을 보지 않는다).
    assert.deepEqual(scroll('I200.1'), []); assert.deepEqual(scroll('I199.1').length, 1); s.names.set('big', 300); s.types.set(300, view({ frameCount: 4 })); assert.deepEqual(scroll('Ibig.1'), [[300, 1, 0, 0]]);
    assert.deepEqual(scroll('Inope.1'), []); assert.deepEqual(scroll('I4294967295.1'), []);
    // 제한 모드: 타입 1 이상·타입 수 미만·프레임 < count·SHP 높이 값 있음·높이 ≤16 이면 99999×12 로 맞춘다.
    restricted = true; const heights = []; s.hooks.frameHeight = (type, frame) => { heights.push([type, frame]); return s.nextHeight; };
    s.nextHeight = 16; assert.deepEqual(scroll('I5.4'), [[5, 4, 99999, 12]]); assert.deepEqual(heights, [[5, 4]]);
    s.nextHeight = 0; assert.equal(scroll('I5.4').length, 1); s.nextHeight = -9; assert.equal(scroll('I5.4').length, 1);
    s.nextHeight = 17; assert.deepEqual(scroll('I5.4'), []); s.nextHeight = null; assert.deepEqual(scroll('I5.4'), []); s.nextHeight = undefined; assert.deepEqual(scroll('I5.4'), []);
    // count 프레임(무자산)·타입 0 이하는 제한 모드에서 거부한다.
    s.nextHeight = 5; assert.deepEqual(scroll('I5.20'), []); assert.deepEqual(scroll('I5.19').length, 1); s.names.set('zero', 0); s.types.set(0, view({ frameCount: 5 }));
    assert.deepEqual(scroll('Izero.1'), []); assert.deepEqual(scroll('Ibig.1'), []);
    // 경계: 타입 1·프레임 0·타입 수와 같은 번호(count 가 있어도 거부)·마지막 프레임.
    s.names.set('one', 1); s.types.set(1, view({ frameCount: 5 })); s.names.set('edge', 200); s.types.set(200, view({ frameCount: 5 })); s.names.set('last', 199); s.types.set(199, view({ frameCount: 5 }));
    assert.deepEqual(scroll('Ione.0'), [[1, 0, 99999, 12]]); assert.deepEqual(scroll('Iedge.1'), []); assert.deepEqual(scroll('Ilast.4'), [[199, 4, 99999, 12]]); assert.deepEqual(scroll('I5.0'), [[5, 0, 99999, 12]]); restricted = false; assert.deepEqual(scroll('Izero.1'), [[0, 1, 0, 0]]); restricted = true;
    // 제한이 아니면 높이 장치를 읽지 않는다.
    restricted = false; heights.length = 0; scroll('I5.4'); assert.deepEqual(heights, []);
    // 옵션은 한 줄 파서와 같이 위임한다. I 가 아니면 그림 버퍼는 건드리지 않는다.
    assert.deepEqual(scroll('T5 B.3'), []); assert.equal(tag.value, 5); assert.equal(context[5], 0); tag.value = 0;
    assert.deepEqual(scroll('B.3 E1.1'), []); assert.equal(context[5], 3); assert.equal(context[0], 0); assert.deepEqual(scroll('T6'), []); assert.equal(tag.value, 6);
    assert.deepEqual(scroll('E1.1'), []); assert.equal(context[0], 2); assert.deepEqual(scroll('S.snd'), []); assert.deepEqual(s.sounds, ['snd']);
    // 첫 NUL 에서 끊고 80바이트 프레임 식은 거부하며 필요한 장치가 없으면 도달 때 거부한다.
    assert.deepEqual(scroll('I5.4\0I5.5'), [[5, 4, 0, 0]]); assert.deepEqual(scroll('\0I5.4'), []); assert.throws(() => scroll(`I5.${'1'.repeat(80)}`), /지역 배열/);
    assert.throws(() => s.markup.applyScrolling('I5.1', context, tag, undefined, append), /제한 필드/); assert.throws(() => s.markup.applyScrolling('I5.1', context, tag, () => false, undefined), /실제 버퍼/);
    const bare = rig({ hooks: { typeCount: undefined } }); bare.types.set(5, view({ frameCount: 9 })); assert.throws(() => bare.markup.applyScrolling('I5.1', words(), { value: 0 }, () => false, append), /타입 수 경계/); bare.markup.applyScrolling('Inope.1', words(), { value: 0 }, () => false, append);
    const noHeight = rig({ hooks: { frameHeight: undefined } }); noHeight.types.set(5, view({ frameCount: 9 })); noHeight.markup.applyScrolling('I5.1', words(), { value: 0 }, () => false, append); assert.throws(() => noHeight.markup.applyScrolling('I5.1', words(), { value: 0 }, () => true, append), /SHP 헤더 경계/);
});

test('다중 줄 그림 글자 식: 글자 종류 범위 거부·타입 helper 재조회 순서', () => {
    const s = rig(); s.types.set(9, view({ frameCount: 5, letterKinds: 17 })); const noop = () => {};
    assert.throws(() => s.markup.applyScrolling('I9.A', words(), { value: 0 }, () => false, noop), /글자 표 범위/);
    let n = 0; const views = [view({ gumpFrame: -1 }), view({ defaultFrame: 2 }), view({ frameCount: 9 }), view({ frameCount: 9 })]; s.hooks.type = (type) => { s.calls.push(type); return views[Math.min(n++, 3)]; }; s.calls.length = 0;
    const added = []; s.markup.applyScrolling('I5.*', words(), { value: 0 }, () => false, (...a) => added.push(a)); assert.deepEqual(added, [[5, 2, 0, 0]]); assert.equal(s.calls.length, 4);
});

test('실제 GameAssets 연결: 188타입·이름·특수/글자 프레임·SHP 헤더 높이와 빈/범위 밖 타입', () => {
    const raw = realCaptureAssets(); const game = new GameAssets(raw.table, raw.database); const hooks = {}; bindStyleTextMarkupAssets(hooks, game);
    assert.equal(hooks.typeCount(), 188);
    // 이름은 대소문자 무시이고 못 찾으면 -1 이다.
    assert.equal(hooks.findType('fortGump'), 92); assert.equal(hooks.findType('FORTGUMP'), 92); assert.equal(hooks.findType('nope'), -1); assert.equal(hooks.findType('92'), -1);
    // 실제 타입 정의의 그림 선택 필드다. fortGump(92)는 논리 81 프레임이다.
    const fort = hooks.type(92); assert.equal(fort.frameCount, 81); assert.equal(fort.first.length, 16); assert.deepEqual(fort.first, Array.from(raw.table.view(92).first));
    assert.deepEqual([fort.defaultFrame, fort.gumpFrame, fort.letterKinds], [raw.table.definitions[22].defaultFrame, raw.table.definitions[22].gumpFrame, raw.table.view(92).letterKinds]);
    // 매번 새 값 객체를 돌려주고 안정된 표를 변경할 수 없다.
    const again = hooks.type(92); assert.notEqual(again, fort); again.first[0] = 99; assert.notEqual(hooks.type(92).first[0], 99);
    // 내장 무자산/여분 타입은 빈 보기, 표 밖 번호는 거부한다.
    for (const type of [0, 69, 186, 187]) assert.deepEqual(hooks.type(type), emptyStyleTextMarkupType(), String(type));
    for (const type of [-1, 188, 1000]) assert.throws(() => hooks.type(type), RangeError);
    // 제한 그림의 SHP 헤더 +1a signed 높이(논리 프레임 범위만)다.
    const headerHeight = (name, frame) => { const bytes = raw.database.squidHeader(game.find(name).block, frame); return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getInt16(0x1a, true); };
    assert.equal(hooks.frameHeight(92, 0), 146); assert.equal(hooks.frameHeight(92, 0), headerHeight('fortGump', 0)); assert.equal(hooks.frameHeight(70, 64), headerHeight('dude', 64));
    assert.equal(hooks.frameHeight(70, 65), null); assert.equal(hooks.frameHeight(92, 81), null); assert.equal(hooks.frameHeight(92, -1), null); assert.equal(hooks.frameHeight(69, 0), null); assert.equal(hooks.frameHeight(186, 0), null); assert.equal(hooks.frameHeight(0, 0), null);
});

test('자산 연결 경계: 정의 없는 타입·SHP 헤더 높이 상위 바이트와 부호·프레임 수 signed 상한', () => {
    const raw = realCaptureAssets(); const header = new Uint8Array(36); const shapes = { squidHeader: () => header };
    // 정의를 읽지 못한 타입은 프레임이 없는 것으로 보고 헤더를 읽지 않는다.
    const empty = { typeTable: raw.table, types: () => [{ definition: null, block: 0 }], shapes: { squidHeader: () => { throw new Error('읽으면 안 됨'); } } }; const hooks = {}; bindStyleTextMarkupAssets(hooks, empty); assert.equal(hooks.frameHeight(70, 0), null);
    // 높이 WORD 는 하위/상위 바이트와 부호를 모두 읽는다.
    const one = { typeTable: raw.table, types: () => [{ definition: { clusters: [{}] }, block: 0 }], shapes }; const bound = {}; bindStyleTextMarkupAssets(bound, one);
    header[0x1a] = 0x34; header[0x1b] = 0x12; assert.equal(bound.frameHeight(70, 0), 0x1234); header[0x1a] = 0x80; header[0x1b] = 0xff; assert.equal(bound.frameHeight(70, 0), -128); header[0x1a] = 0xff; header[0x1b] = 0x7f; assert.equal(bound.frameHeight(70, 0), 32767);
    // 프레임 수가 signed 32비트 상한을 넘으면 거부하고 상한 값은 허용한다.
    for (const [count, ok] of [[0x7fffffff, true], [0x80000000, false]]) { const fake = { typeTable: { find: () => -1, view: () => ({ frameCount: count, defaultFrame: 0, gumpFrame: 0, letterKinds: 0, first: [] }) }, types: () => [], shapes }; const h = {}; bindStyleTextMarkupAssets(h, fake); if (ok) assert.equal(h.type(70).frameCount, count); else assert.throws(() => h.type(70), /프레임 수 범위/); }
});

test('실제 자산 전수: 116개 타입×프레임 식이 독립 구현(picture-resolver)과 같은 프레임·크기·기준점을 만든다', () => {
    const raw = realCaptureAssets(); const game = new GameAssets(raw.table, raw.database); const assets = new MarkupAssets(game);
    const fragments = new StyleTextState(); const pictureState = new MarkupState(); const pictureHooks = {}; bindMarkupAssets(pictureState, pictureHooks, assets); const pictures = new Markup(fragments, pictureState, pictureHooks);
    const hooks = {}; bindStyleTextMarkupAssets(hooks, game); const markup = new StyleTextMarkup(fragments, pictures, new StyleTextMarkupState(), hooks);
    const resolve = createPictureResolver({ shapes: raw.database, getTable: () => raw.table });
    const expressions = ['*', 'A', 'B1', 'C2', 'L6', '0', '1', '5', '40', '113', '9999', '-1', 'a1', 'Z3']; let compared = 0; let letterHits = 0;
    // 모든 자산 타입과 대표 프레임 식을 같은 입력으로 두 구현에 넣어 비교한다.
    for (const asset of game.types()) {
        for (const expression of expressions) {
            const reference = resolve(asset.assetName, expression); if (!reference) continue;
            const context = words(); const head = owner(); markup.apply(`I${asset.assetName}.${expression}`, context, head, { value: 0 });
            const tokens = chain(fragments, head); assert.equal(tokens.length, 1, `${asset.assetName}.${expression}`);
            const frame = pictures.lookup(tokens[0]).frame; assert.deepEqual([frame.width, frame.height, frame.hotspotX, frame.hotspotY], [reference.width, reference.height, reference.hotspotX, reference.hotspotY], `${asset.assetName}.${expression}`);
            // 프레임 번호도 독립 계산식(특수 프레임/글자 구간/CRT 정수/클램프)과 같다.
            const v = raw.table.view(asset.block + 70); let expected; let suffix = expression;
            if (expression[0] === '*') expected = v.gumpFrame === -1 ? v.defaultFrame : v.gumpFrame;
            else { expected = 0; const letter = expression.charCodeAt(0); if (letter >= 65 && letter < 128 && letter < 65 + v.letterKinds) { expected = v.first[letter - 65]; suffix = expression.slice(1); letterHits += 1; } expected = (expected + parseCrtInteger(suffix)) | 0; }
            if (expected < 0) expected = 0; else if (expected >= v.frameCount) expected = v.frameCount;
            assert.equal(pictureState.pictures.get(tokens[0]).frame, expected, `${asset.assetName}.${expression}`); compared += 1;
        }
    }
    assert.ok(compared > 1000, `비교 ${compared}건`); assert.ok(letterHits > 50, `글자 구간 ${letterHits}건`);
});

test('실제 StyleText→대괄호→Markup→글꼴 연결: 본문 폭/높이에 그림 폭/높이·`#` 맞춤 배율이 더해지고 반납 순서가 유지된다', () => {
    const raw = realCaptureAssets(); const game = new GameAssets(raw.table, raw.database); const assets = new MarkupAssets(game);
    const fragments = new StyleTextState(); const pictureState = new MarkupState(); const base = {}; bindMarkupAssets(pictureState, base, assets); const pictures = new Markup(fragments, pictureState, base);
    const state = new StyleTextMarkupState(); const markupHooks = {}; bindStyleTextMarkupAssets(markupHooks, game); const markup = new StyleTextMarkup(fragments, pictures, state, markupHooks);
    const fonts = stoneFonts(true); const hooks = {}; bindStyleTextFonts(hooks, new GumpText(fonts)); bindMarkup(hooks, pictures); const styled = markup.hooks(hooks); const text = new StyleText(fragments, styled);
    const plain = owner(); text.parse(plain, 'ab', words([0, 0, 17])); const textSize = text.measure(plain);
    const head = owner(); text.parse(head, 'ab~[I fortGump.0]', words([0, 0, 17])); const size = text.measure(head);
    // fortGump 의 논리 프레임 0 은 200×146 이다. 같은 줄에서 폭은 합이고 높이는 기준선을 맞춘 쪽이다.
    const frame0 = assets.frame(92, 0, 0); assert.deepEqual(frame0, { width: 200, height: 146, hotspotX: 0, hotspotY: 0, extraX: 0, extraY: 0 }); assert.equal(size.width, textSize.width + 200);
    // 그림의 기준점 y 가 0 이라 본문 기준선(상승)만큼 아래로 내려 앉아 높이가 146 + 상승이 된다.
    const rise = text.baseline(chain(fragments, plain)[0])[1]; assert.ok(rise > 0); assert.equal(size.height, Math.max(textSize.height, 146 + rise));
    // `*` 는 gumpFrame(20) 프레임이다.
    const star = owner(); text.parse(star, '~[I fortGump.*]', words()); const starFrame = assets.frame(92, 0, 20); assert.equal(pictureState.pictures.get(chain(fragments, star)[0]).frame, 20); assert.equal(text.measure(star).width, starFrame.width); text.clear(star);
    // `#` 는 65×60 상자에 맞춰 float32 배율을 쓰고 `&` 는 색 치환 BYTE 를 켠다.
    const fit = owner(); text.parse(fit, '~[I fortGump.#]~[I fortGump.&]', words()); const [first, second] = chain(fragments, fit);
    // fortGump 의 `#` 프레임(20)은 상자보다 작아 배율이 1 로 남는다(선택 비율이 1 이상이면 갱신하지 않음).
    assert.equal(pictureState.pictures.get(first).scale, 1); assert.equal(pictureState.pictures.get(first).mapped, 0); assert.equal(pictureState.pictures.get(second).mapped, 1);
    assert.deepEqual(pictures.measure(first), [starFrame.width, starFrame.height]);
    // 모든 자산 타입에서 `#` 배율이 독립 float32 공식(세로 비율 먼저 float32, 선택 비율 1 미만만 갱신)과 같다.
    let shrunk = 0; const sweepHead = owner();
    for (const asset of game.types()) {
        const before = chain(fragments, sweepHead).length; markup.apply(`I${asset.assetName}.#`, words(), sweepHead, { value: 0 }); const added = chain(fragments, sweepHead); if (added.length === before) continue;
        const token = added.at(-1); const size = pictures.lookup(token).frame; const widthRatio = 65 / size.width; const heightRatio = Math.fround(60 / size.height); const selected = widthRatio < heightRatio ? widthRatio : heightRatio;
        assert.equal(pictureState.pictures.get(token).scale, selected < 1 ? Math.fround(selected) : 1, asset.assetName); if (selected < 1) shrunk += 1;
    }
    assert.ok(shrunk > 10, `축소된 타입 ${shrunk}개`); text.clear(sweepHead);
    // 공통 Clear 는 문자열→그림 순서로 두 저장소를 비운다.
    text.clear(head); text.clear(fit); text.clear(plain); assert.equal(fragments.fragments.size, 0); assert.equal(pictureState.pictures.size, 0);
});

test('팔레트·설정 연결: E 의 검정은 기본 별칭 0번, zone 옵션 네 키는 존재할 때만 갱신한다', () => {
    const hooks = {}; bindStyleTextMarkupPalette(hooks, { basic: [33, 44] }); assert.equal(hooks.black(), 33);
    const values = new Map([['allowzonesounds', '0'], ['zoneoffset', ' 7x']]); const read = (key, current) => GameData.prototype.readInt.call({ configuration: { get: (name) => values.get(name) } }, key, current);
    const state = new StyleTextMarkupState(); assert.deepEqual([state.allowSounds, state.allowBackgroundColor, state.allowForegroundColor, state.allowOffset, state.fitWidth, state.fitHeight], [1, 1, 1, 1, 65, 60]);
    loadStyleTextMarkupOptions(state, { readInt: read });
    // 키가 있는 값만 바뀌고(설정 값은 CRT 정수 규칙) 없는 키는 기본값을 유지한다. 맞춤 상자는 설정 대상이 아니다.
    assert.deepEqual([state.allowSounds, state.allowBackgroundColor, state.allowForegroundColor, state.allowOffset, state.fitWidth, state.fitHeight], [0, 1, 1, 7, 65, 60]);
    values.set('zonebgcolor', '0'); values.set('zonefgcolor', '2'); loadStyleTextMarkupOptions(state, { readInt: read }); assert.deepEqual([state.allowBackgroundColor, state.allowForegroundColor], [0, 2]);
    // 키 이름은 정확히 원본 설정 키만 읽는다.
    const keys = []; loadStyleTextMarkupOptions(new StyleTextMarkupState(), { readInt: (key, current) => { keys.push(key); return { changed: false, value: current }; } }); assert.deepEqual(keys, ['allowzonesounds', 'zonebgcolor', 'zonefgcolor', 'zoneoffset']);
});
