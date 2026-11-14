// WarningMessages의 공통 생성/목록/실제 Text/트리/삭제와 C++ 17자리 보고값을 검사한다. 게임/브라우저/MP 실행이 아니다. (2026-11-14 WB04-E454)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { warningScene } from './warning-messages-support.mjs';
import { GameClock } from '../www/core/game-clock.js';

test('Warnings_ActualStyleAndGumpLifetime: 실제 공통 20칸·10초 경계·삭제 전후 주소와 재사용', () => {
    const r = warningScene({ now: 0.1 }); const m = r.messages; m.add('~B~E~yOur Priest has been Immobilized'); m.add('~B~E~yOur Priest has been Immobilized');
    assert.equal(r.state.sounds, 2); assert.deepEqual(r.rows().map((row) => row.y), [0, 12]); assert.equal(r.create.count, 3);
    // 같은 Warning도 독립 생성하며 21번째는 생성/소리 없이 끝낸다.
    for (let index = 0; index < 19; index += 1) m.add('~E~yWarning');
    assert.equal(r.rows().length, 20); assert.equal(r.state.sounds, 20); assert.equal(r.create.count, 21);
    r.state.now = 10.1 - Number.EPSILON * 8; r.idle(); assert.equal(r.rows().length, 20);
    const token = r.rows()[0].token; r.state.now = 10.1; r.idle(); assert.equal(m.hasMessages(), false); assert.ok(m.node(token)); assert.equal(r.create.count, 21); assert.equal(r.common.life.killCount, 20);
    r.cleanup(); assert.equal(m.node(token), null); assert.equal(r.create.count, 1); assert.equal(r.fragments.fragments.size, 0);
    m.add('~E~yAgain'); assert.equal(r.rows()[0].y, 0); assert.equal(r.state.sounds, 21);
});

test('Warnings_PauseSharesBoardAndPermanentText: 정지 안내는 벽시각 없이 영구이고 같은 표에서 삭제한다', () => {
    let clocks = 0; const r = warningScene({ clock: () => { clocks += 1; return r.state.now; } }); r.messages.pauseStatus(true); assert.equal(clocks, 0); r.messages.add('~E~yWarning');
    assert.deepEqual(r.rows().map((row) => row.y), [0, 12]); assert.equal(r.state.sounds, 1); assert.equal(r.rows()[0].source, '~E~wGame Paused!');
    r.state.now = 100; r.idle(); assert.equal(r.rows().length, 1); assert.equal(clocks, 2); r.messages.pauseStatus(false); assert.equal(r.messages.hasMessages(), false); r.cleanup(); assert.equal(r.create.count, 1);
    r.messages.pauseStatus(true); assert.equal(r.rows().length, 1); assert.equal(r.state.sounds, 1);
});

test('Warnings_SeparatedFrameStagesExpireWhileGameClockIsPaused: 실제 GameClock 정지와 벽시각 만료 분리', () => {
    const clock = new GameClock(0); let milliseconds = 100; let reads = 0; const r = warningScene({ clock: () => { reads += 1; return clock.wallSeconds(milliseconds); } });
    r.messages.pauseStatus(true); r.messages.add('~E~yTimed warning'); const initialReads = reads; clock.pause(milliseconds); milliseconds = 10100;
    r.cleanup(); assert.equal(reads, initialReads); assert.equal(r.rows().length, 2); r.idle(); assert.equal(r.rows().length, 1); assert.equal(clock.gameSeconds(milliseconds), 0.1); assert.equal(reads, initialReads + 1);
    r.cleanup(); assert.equal(r.create.count, 2);
});

test('같은 실제 등록표의 다른 형제는 메시지로 읽지 않으며 Kill 뒤 free는 Cleanup까지 지연된다', () => {
    const r = warningScene(); const other = r.sibling(); r.messages.add('~E~ySame Client'); const row = r.rows()[0]; const id = r.messages.node(row.token).id;
    assert.equal(r.messages.lookup(r.lookup(other).serial), null); assert.equal(r.messages.lookup(row.serial).token, row.token); assert.equal(r.messages.node(row.token).parent, r.create.entries[0]);
    r.lifetime.kill(row.token); assert.equal(r.messages.hasMessages(), false); assert.ok(r.messages.node(row.token)); assert.equal(r.create.entries[id], row.token);
    r.cleanup(); assert.equal(r.messages.node(row.token), null); assert.equal(r.create.entries[id], 0); assert.ok(r.lookup(other)); assert.equal(r.create.count, 2);
});

test('Warnings_CommonPageClearAndStaleBoard: 공통 전체 정리 뒤 오래된 표를 재사용하고 실제 Style 체인을 반납한다', () => {
    const r = warningScene();
    // 원문 표를 가득 채워 stale serial 재사용을 확인한다.
    for (let index = 0; index < 20; index += 1) r.messages.add('~E~yOld page');
    const last = r.rows().at(-1).serial; r.clear(); assert.equal(r.messages.hasMessages(), false); assert.equal(r.create.count, 1); assert.equal(r.fragments.fragments.size, 0);
    r.messages.add('~E~yNew page'); assert.equal(r.rows()[0].y, 0); assert.ok(r.rows()[0].serial > last); assert.equal(r.state.sounds, 21);
});

test('Warnings_ReentrantSoundCreatesSiblingInSameRegistry: 생성 중 콜백의 다른 Gump와 serial·root를 공유한다', () => {
    let r; let other; r = warningScene({ onSound: () => { other = r.sibling(); } }); r.messages.add('~E~yAfter sibling'); const row = r.rows()[0];
    assert.ok(r.lookup(other).serial > row.serial); assert.equal(r.messages.node(row.token).parent, r.create.entries[0]); assert.equal(r.create.count, 3); assert.equal(r.rows().length, 1);
});

test('생성 실패는 실제 Text 소멸/등록 해제/저장소 반납을 실행하고 다음 생성이 가능하다', () => {
    const r = warningScene(); assert.throws(() => r.messages.add('x'.repeat(256)), RangeError); assert.equal(r.state.sounds, 1); assert.equal(r.create.count, 1); assert.equal(r.messages.hasMessages(), false); assert.equal(r.fragments.fragments.size, 0);
    r.messages.add('good'); assert.equal(r.rows().length, 1); assert.equal(r.state.sounds, 2); assert.equal(r.create.count, 2);
});

test('Text 생성 전 실패와 Style 작성 후 Fit 실패를 구별해 등록표·실제 조각을 반납한다', () => {
    const before = warningScene(); const construct = before.single.construct; before.single.construct = () => { throw new Error('before Text'); };
    assert.throws(() => before.messages.add('never styled'), /before Text/); assert.equal(before.create.count, 1); assert.equal(before.messages.node(100), null); assert.equal(before.state.sounds, 0);
    before.single.construct = construct; before.messages.add('next'); assert.equal(before.rows()[0].token, 101);
    const after = warningScene(); const fit = after.single.fit; after.single.fit = () => { throw new Error('after Style'); };
    assert.throws(() => after.messages.add('already styled'), /after Style/); assert.equal(after.create.count, 1); assert.equal(after.messages.node(100), null); assert.equal(after.fragments.fragments.size, 0); assert.ok(after.events.some((event) => event[0] === 'release'));
    after.single.fit = fit; after.messages.add('next'); assert.equal(after.rows().length, 1);
});

test('기준선 뒤 연결한 상태 안내도 같은 실제 공장을 사용하고 미연결 세 종류는 생성 전에 거부한다', () => {
    const r = warningScene();
    // 실제 공장이 연결한 8종의 본문/음향/기한을 공유 부모에 통과시킨다.
    for (const kind of [1, 2, 3, 4, 5, 7, 9, 18]) r.messages.battleStatus(kind, 4, 1);
    assert.equal(r.rows().length, 8); assert.equal(r.state.sounds, 5); assert.equal(r.rows()[0].deadline, 0); assert.equal(r.rows()[1].deadline, 0);
    assert.match(r.rows()[1].source, /^~E~uP4/); assert.match(r.rows()[3].source, /sent you 1 Storm Power/); assert.match(r.rows()[7].source, /Injection of 1 sp/);
    const count = r.create.count; const serial = r.create.serial;
    // 원문의 실제 생성 미연결 경계를 숨기거나 MessageGump의 지원 목록으로 확장하지 않는다.
    for (const kind of [6, 8, 10]) assert.throws(() => r.messages.battleStatus(kind, 4, 1), /미연결/);
    assert.equal(r.create.count, count); assert.equal(r.create.serial, serial);
});

test('억제 관문은 Warning/전투만 막고 Pause/시간은 통과하며 시간 만료는 0.1초다', () => {
    let suppressed = true; const r = warningScene({ suppressed: () => suppressed }); r.messages.add('hidden'); r.messages.battleStatus(9, 0, 0); assert.equal(r.create.count, 1); assert.equal(r.state.sounds, 0);
    r.messages.pauseStatus(true); r.messages.gameTimeStatus(); r.messages.gameTimeStatus(); assert.equal(r.rows().length, 2); assert.equal(r.rows()[1].deadline, 0.1); assert.equal(r.create.count, 3);
    r.state.now = 0.1; r.idle(); assert.equal(r.rows().length, 1); r.cleanup(); suppressed = false; r.messages.battleStatus(9, 0, 0); assert.equal(r.rows().length, 2); assert.equal(r.rows()[1].source, '~E~wGame started!');
});

test('현재 팔레트/크기는 원본 순서로 갱신하고 이미 저장한 문맥은 보존하며 Draw 공급자는 실제 Text를 호출한다', () => {
    const r = warningScene(); r.messages.add('~wVisible'); const row = r.rows()[0]; const head = r.messages.node(row.token).head; const color = r.fragments.fragments.get(head).context[2];
    const palette = { basic: Uint32Array.from({ length: 8 }, (_, index) => index + 50), named: [] }; r.messages.setView(800, 600, palette);
    assert.equal(r.create.screenWidth, 800); assert.equal(r.lookup(r.create.entries[0]).width, 800); assert.strictEqual(r.state.palettes.at(-1), palette); assert.equal(r.fragments.fragments.get(head).context[2], color);
    assert.equal(r.providers.get(2).owns(row.token), true); r.providers.get(2).draw(row.token); assert.ok(r.draws.some((draw) => draw[0] === 'Visible'));
    assert.throws(() => r.messages.setView(0, 10, palette), TypeError); r.clear(); r.messages.destroy(); assert.deepEqual(r.providers.get(2), {});
});

test('직계 링크 순환/미연결 종류는 명시 오류며 NULL/빈 입력은 공통 생성자에서 구별한다', () => {
    const r = warningScene(); r.messages.addNull(); r.messages.add(''); assert.deepEqual(r.rows().map((row) => row.source), ['(null)', '']);
    const count = r.create.count; assert.throws(() => r.messages.battleStatus(6, 1, 0), /미연결/); assert.equal(r.create.count, count);
    const row = r.rows()[0]; const node = r.messages.node(row.token); const oldNext = node.next; node.next = row.token; assert.throws(() => r.messages.lookup(0x7fffffff), /순환/); node.next = oldNext;
});

test('Report: C++ 표준 스트림의 17자리 수치 fixture와 부호0·동률·지수·비정상값을 일치시킨다', () => {
    const r = warningScene(); r.messages.add('report'); const node = r.messages.node(r.rows()[0].token);
    const data = fs.readFileSync(new URL('./fixtures/warning-report-numbers.tsv', import.meta.url), 'utf8').trimEnd().split(/\r?\n/).filter((line) => line && !line.startsWith('#'));
    assert.equal(data.length, 1018);
    // 기대값은 별도 C++ 프로그램이 출력했으며 JS 알고리즘으로 만들지 않았다.
    for (const line of data) { const [bits, expected] = line.split('\t'); const bytes = Buffer.alloc(8); bytes.writeBigUInt64LE(BigInt(`0x${bits}`)); node.deadline = bytes.readDoubleLE(); assert.equal(r.rows()[0].deadlineText, expected, bits); }
});
