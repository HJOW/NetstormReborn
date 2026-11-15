// ClientGameplayGlobals의 장기 상태·대여·실행 수명을 cpppj의 실제 공개 경계로 대조한다.
// cpppj CampaignSessionTests의 실제 Session 부모는 D/E에서 재대조하며 여기서는 같은 RawSurfaceWorld를 가진 최소 Session/전체 owner 계약을 사용한다. (WB04-E570)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { ClientGameplayGlobals, CLIENT_GAMEPLAY_FIELDS, withdrawGameplayForState } from '../www/core/client-gameplay-globals.js';
import { ClientSubscriberGlobals } from '../www/core/client-subscriber-globals.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { CampaignCursorScene } from './campaign-cursor-support.mjs';
import { RawContainedFinder } from '../www/core/raw-contained-finder.js';
import { RawMissionStart, createMissionStartGlobals, createMissionStartState } from '../www/core/raw-mission-start.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { REPO_ROOT } from './helpers.mjs';

// PE 대조는 기존 manifest와 같이 선택 자료이며 존재하는 경우 정확한10.78 판본만 허용한다.
const ORIGINAL_PATH = path.join(REPO_ROOT, 'originals/Netstorm.exe');
// 같은 읽기 전용 원본을 반복해서 읽지 않는다.
let originalBytes = null;

// 보호 원본의 읽기 전용 PE 보기를 원문의 주소 식별자에 대응시킨다.
function originalAt(address, length = 4) {
    if (originalBytes === null) { originalBytes = fs.readFileSync(ORIGINAL_PATH); assert.equal(createHash('sha256').update(originalBytes).digest('hex'), 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236'); }
    const bytes = originalBytes; const pe = bytes.readUInt32LE(0x3c); const imageBase = bytes.readUInt32LE(pe + 24 + 28); const relative = address - imageBase; const table = pe + 24 + bytes.readUInt16LE(pe + 20);
    // 원문 가상 영역의 BSS 부분은 파일 바이트가 아니라 최초0이다.
    for (let i = 0; i < bytes.readUInt16LE(pe + 6); i += 1) { const at = table + i * 40; const start = bytes.readUInt32LE(at + 12); const raw = bytes.readUInt32LE(at + 16); const size = Math.max(bytes.readUInt32LE(at + 8), raw); if (relative >= start && relative + length <= start + size) return relative - start + length <= raw ? bytes.subarray(bytes.readUInt32LE(at + 20) + relative - start, bytes.readUInt32LE(at + 20) + relative - start + length) : Buffer.alloc(length); }
    throw new RangeError('보호 PE 식별자 범위');
}

// 전체 Session을 흉내 내지 않고 같은 실제 World와 pin 설치 입구만 공개한다.
function runtime() {
    const actual = new CampaignCursorScene(); const session = { world: actual.world, pin: null, bindRuntimePin(callback) { this.pin = callback; } }; const owner = { session, world: actual.world, pixels: new Uint8Array(32) }; return { actual, session, owner };
}

test('Gameplay 최초값: 보호 PE22칸·반경4float·기한9double·별도 캐시/장기 상태를 직접 대조한다', (t) => {
    const g = new ClientGameplayGlobals(); assert.equal(CLIENT_GAMEPLAY_FIELDS.length, 22);
    // 각 필드의 현재 초기값을 cpppj 명시 기본값과 비교한다.
    for (const field of CLIENT_GAMEPLAY_FIELDS) assert.equal(g[field.name]().value, field.initial, field.name);
    assert.deepEqual([...g.manaRanges()], [14, 22, 30, 38]); assert.equal(g.energy.radiusSquared, 484); assert.equal(g.energy.allied, 1); assert.equal(g.cursor.lockDepth, 1); assert.equal(g.player.battleOwner, 1); assert.equal(g.player.templateOwner, 1);
    assert.equal(g.world.mySubscriber, 0); assert.equal(g.world.loginSubscriber, 0); assert.deepEqual([...g.battleOptionsOwner()], new Array(87).fill(0)); assert.equal(g.terrainBitmaps().initialized, 0); assert.equal(g.terrainStamp().variants, null);
    assert.deepEqual(g.sidRobustDiagnostics(), { lastRobust: 0, type: 0 });
    if (!fs.existsSync(ORIGINAL_PATH)) { t.diagnostic('보호 원본이 없는 환경이라 PE 바이트 대조만 생략'); return; }
    // 존재하는 실제 PE의 비트는 기본값 표와 독립적으로 읽는다. 현재 작업 환경은 모든 칸을 직접 대조한다.
    for (const field of CLIENT_GAMEPLAY_FIELDS) { const raw = originalAt(field.address); const expected = field.kind === 'f32' ? raw.readFloatLE() : field.kind === 'i32' ? raw.readInt32LE() : raw.readUInt32LE(); assert.equal(g[field.name]().value, expected, field.name); }
    assert.deepEqual(Buffer.from(g.manaRanges().buffer), originalAt(0x5425f4, 16)); assert.deepEqual(Buffer.from(g.priestTakenDeadlines().buffer), originalAt(0x595450, 72)); assert.equal(originalAt(0x52f48c).readFloatLE(), g.energy.radiusSquared); assert.throws(() => originalAt(0), RangeError);
});

test('Gameplay 같은 소유자: Subscriber·권한·GameAux 별칭과 모든 현재 참조의 폭/독립성', () => {
    const subscribers = new ClientSubscriberGlobals(); const authority = new RawAuthorityState(); const aux = new GameAuxState(); const g = new ClientGameplayGlobals(subscribers, authority, aux); const other = new ClientGameplayGlobals();
    subscribers.mySubscriber().value = 17; subscribers.loginSubscriber().value = 23; assert.equal(g.subscribers(), subscribers); assert.equal(g.authority(), authority); assert.equal(g.gameAux(), aux); assert.equal(g.world.mySubscriber, 17); g.world.loginSubscriber = 31; assert.equal(subscribers.loginSubscriber().value, 31);
    g.server().value = false; assert.equal(authority.server, false); authority.boss = false; assert.equal(g.boss().value, false); assert.equal(other.server().value, true);
    // 별칭 쓰기는 같은 현재 GameAux의 TYPE만 바꾸고 별도 복사본을 만들지 않는다.
    for (const name of ['geyserType', 'firstTimerFactoryType', 'secondTimerFactoryType', 'outpostType']) { const ref = g[name](); assert.equal(g[name](), ref); ref.value = aux[name] + 1; assert.equal(aux[name], ref.value); aux[name] += 1; assert.equal(ref.value, aux[name]); }
    g.fullDeck().value = 0x100000001; g.denySalvage().value = 0xffffffff; g.manaRadius().value = 1 / 3; assert.equal(g.fullDeck().value, 1); assert.equal(g.denySalvage().value, -1); assert.equal(g.manaRadius().value, Math.fround(1 / 3)); assert.equal(other.fullDeck().value, 0);
    g.energy.radiusSquared = 1 / 3; assert.equal(g.energy.radiusSquared, Math.fround(1 / 3)); assert.notEqual(g.terrainBitmaps(), other.terrainBitmaps()); assert.notEqual(g.camera(), other.camera()); assert.notEqual(g.cursorGlobals(), other.cursorGlobals());
});

test('Gameplay 일반 대여: 같은 객체를 fresh로 만들고 특별/치트·기록·선택 누적과 장기 캐시는 보존한다', () => {
    const g = new ClientGameplayGlobals(); const refs = [g.player, g.cursor, g.finder, g.energy, g.selection, g.cursorGlobals().lifecycle().previews];
    Object.assign(g.player, { special: 0xabcdef01, playerKnown: 9, battleOwner: 7 }); Object.assign(g.cursor, { preview: 0xfedcba98, lockDepth: 9, recording: 7, recorder: 0x76543210 }); Object.assign(g.energy, { radiusSquared: 99, userCheat: 0x845fed, ignore: 9, allied: 0 });
    g.selection.priestSelections = 19; g.selection.appendMultiple(25); g.selection.selected = 42; g.fullDeck().value = 9; g.manaRadius().value = 38; g.world.editor = 8; g.world.missionPost = false; g.authority().server = false; g.authority().boss = false;
    g.terrainBitmaps().initialized = 1; g.terrainBitmaps().classified[120] = 43; g.terrainStamp().variants = [8, 3]; g.camera().snapshot().bytes[3] = 0x99; g.battleOptionsOwner()[2] = 2; g.manaRanges()[1] = 31; g.priestTakenDeadlines()[1] = 9.5;
    g.aiDebug().checking().value = -7; g.finder.assertion = () => {}; g.finder.containedType = 99; const lease = g.acquire();
    assert.equal(g.borrowed(), true); assert.deepEqual([g.player, g.cursor, g.finder, g.energy, g.selection, g.cursorGlobals().lifecycle().previews], refs);
    assert.deepEqual([g.player.special, g.player.playerKnown, g.player.battleOwner, g.cursor.lockDepth, g.cursor.preview, g.cursor.recording, g.cursor.recorder], [0xabcdef01, 0, 1, 1, 0, 7, 0x76543210]);
    assert.deepEqual([g.energy.radiusSquared, g.energy.userCheat, g.energy.ignore, g.energy.allied, g.fullDeck().value, g.manaRadius().value], [484, 0x845fed, 0, 1, 0, 22]);
    assert.equal(g.finder.containedType, 6); assert.equal(g.finder.assertion, null); assert.equal(g.finder.currentChecking, 0xfffffff9); assert.equal(g.finder.currentBoss, true); assert.equal(g.world.editor, 0); assert.equal(g.world.missionPost, true); assert.equal(g.server().value, true);
    assert.equal(g.selection.multipleCount, 0); assert.equal(g.selection.priestSelections, 19); assert.equal(g.camera().snapshot().bytes[3], 0x99); assert.equal(g.terrainBitmaps().classified[120], 43); assert.deepEqual(g.terrainStamp().variants, [8, 3]); assert.equal(g.battleOptionsOwner()[2], 2); assert.equal(g.manaRanges()[1], 31); assert.equal(g.priestTakenDeadlines()[1], 9.5); lease.release();
});

test('Gameplay 준비 대여: 초기화/반납 쓰기 없이 현재 Player·커서·에너지·선택·권한을 보존한다', () => {
    const g = new ClientGameplayGlobals(); g.player.playerKnown = 7; g.cursor.preview = 0x12345678; g.cursor.lockDepth = 9; g.cursorGlobals().lifecycle().previews[29] = 0xfedcba98; g.world.editor = 0x80000000; g.world.missionPost = false; g.energy.userCheat = 17; g.energy.radiusSquared = 13; g.manaRadius().value = 31; g.authority().boss = false; g.selection.selected = 54;
    const use = g.acquirePrepared(); assert.equal(g.borrowed(), true); use.release(); assert.equal(g.borrowed(), false);
    assert.deepEqual([g.player.playerKnown, g.cursor.preview, g.cursor.lockDepth, g.cursorGlobals().lifecycle().previews[29], g.world.editor, g.world.missionPost, g.energy.userCheat, g.energy.radiusSquared, g.manaRadius().value, g.boss().value, g.selection.selected], [7, 0x12345678, 9, 0xfedcba98, 0x80000000, false, 17, 13, 31, false, 54]);
});

test('Gameplay 일반 반납/부분 생성: 현재 공개·PlayerKnown·커서·editor만 무효화하고 낡은 대여는 다음 세대를 못 바꾼다', () => {
    const g = new ClientGameplayGlobals(); const use = g.acquire(); const session = {}; use.publish(session); g.player.playerKnown = 9; g.player.battleOwner = 3; g.cursor.preview = 42; g.cursor.recording = 17; g.cursor.recorder = 23; g.world.editor = 9; g.energy.ignore = 7; g.fullDeck().value = 0x80000000; g.selection.selected = 59; use.release();
    assert.throws(() => g.requireSession(), /미연결/); assert.deepEqual([g.player.playerKnown, g.player.battleOwner, g.cursor.preview, g.cursor.lockDepth, g.cursor.recording, g.cursor.recorder, g.world.editor, g.energy.ignore, g.fullDeck().value, g.selection.selected], [0, 3, 0, 1, 17, 23, 0, 7, 0x80000000, 59]);
    const next = g.acquirePrepared(); g.cursor.preview = 123; use.release(); assert.equal(g.borrowed(), true); assert.equal(g.cursor.preview, 123); assert.throws(() => use.publish({}), /이미 반납/); assert.throws(() => use.withdraw(session), /이미 반납/); next.release();
});

test('Gameplay 대여 관문: 중복 일반/준비 대여는 어떤 fresh 쓰기보다 먼저 거부한다', () => {
    const g = new ClientGameplayGlobals(); const first = g.acquirePrepared(); g.cursor.preview = 123; g.player.playerKnown = 7; g.fullDeck().value = 9;
    assert.throws(() => g.acquire(), /중복 대여/); assert.throws(() => g.acquirePrepared(), /중복 대여/); assert.deepEqual([g.cursor.preview, g.player.playerKnown, g.fullDeck().value], [123, 7, 9]); first.release(); const second = g.acquire(); assert.equal(g.cursor.preview, 0); second.release();
});

test('Gameplay 공개/반납: 같은 Session만 닫고 중복·미공개·다른 대상을 거부한다', () => {
    const g = new ClientGameplayGlobals(); const use = g.acquire(); const first = {}; const second = {};
    assert.throws(() => use.publish(null), TypeError); assert.throws(() => g.requireSession(), /미연결/); use.publish(first); assert.equal(g.requireSession(), first); assert.throws(() => use.publish(second), /중복 연결/); assert.throws(() => use.withdraw(second), /반납 불일치/); assert.equal(g.requireSession(), first);
    use.withdraw(first); assert.equal(g.borrowed(), true); assert.throws(() => g.requireSession(), /미연결/); use.publish(second); assert.equal(g.requireSession(), second); use.release();
});

test('Gameplay 빈 Session 경계: 미공개 null끼리 같아도 실제 참조/State 반납으로 인정하지 않는다', () => {
    const g = new ClientGameplayGlobals(); const use = g.acquire(); g.player.playerKnown = 7;
    assert.throws(() => use.withdraw(null), /반납 불일치/); assert.throws(() => withdrawGameplayForState(use, null), /같은 현재/); assert.throws(() => g.bindRuntimeOwner(null, {}), TypeError); assert.throws(() => g.pinCurrentRuntime(null), /미공개/); assert.equal(g.runtimeCallActive(null), false);
    assert.equal(g.player.playerKnown, 7); use.release(); assert.equal(g.player.playerKnown, 0);
});

test('Gameplay Runtime pin: 실제 World를 가진 전체 owner를 중첩 보유하고 배송 중 재공개/반납을 거부한다', () => {
    const g = new ClientGameplayGlobals(); const { actual, session, owner } = runtime(); const use = g.acquirePrepared(); use.publish(session);
    assert.throws(() => g.pinCurrentRuntime(session), /미공개/); assert.throws(() => g.bindRuntimeOwner({}, owner), TypeError); assert.throws(() => g.bindRuntimeOwner(session, null), TypeError);
    g.bindRuntimeOwner(session, owner); assert.equal(g.runtimeCallActive(session), false); const first = session.pin(); const second = g.pinCurrentRuntime(session); assert.equal(first.owner, owner); assert.equal(first.owner.world.storage.pool, actual.core.pool); assert.equal(g.runtimeCallActive(session), true); assert.equal(g.runtimeCallActive({}), false);
    assert.throws(() => g.bindRuntimeOwner(session, owner), /배송 중/); assert.throws(() => use.withdraw(session), /배송 종료 전/); assert.throws(() => withdrawGameplayForState(use, session), /배송 종료 전/);
    first.release(); first.release(); assert.equal(first.owner, null); assert.equal(g.runtimeCallActive(session), true); second.release(); assert.equal(g.runtimeCallActive(session), false); use.withdraw(session); use.release(); actual.world.clearProcesses();
});

test('Gameplay Runtime pin 꼬리: 설치 실패 prefix·현재 Session 교체·예외 finally 반납을 보존한다', () => {
    const g = new ClientGameplayGlobals(); const use = g.acquirePrepared(); const noEntry = {}; use.publish(noEntry); const owner = { session: noEntry };
    assert.throws(() => g.bindRuntimeOwner(noEntry, owner), /pin 입구 미연결/); const prefix = g.pinCurrentRuntime(noEntry); assert.equal(prefix.owner, owner); prefix.release(); use.withdraw(noEntry);
    const { actual, session, owner: realOwner } = runtime(); use.publish(session); g.bindRuntimeOwner(session, realOwner); const old = session.pin;
    assert.throws(() => { const pin = old(); try { throw new Error('선택한 하위 예외'); } finally { pin.release(); } }, /선택한 하위 예외/); assert.equal(g.runtimeCallActive(session), false);
    use.withdraw(session); assert.throws(() => old(), /미연결/); use.publish({}); assert.throws(() => old(), /현재Session교체/); assert.throws(() => g.pinCurrentRuntime(session), /미공개/); use.release(); actual.world.clearProcesses();
});

test('Gameplay State friend: 같은 현재 대여/Session의 반납 정책만 바꾸고 자식 정리 증빙은 만들지 않는다', () => {
    const g = new ClientGameplayGlobals(); const use = g.acquire(); const session = {}; use.publish(session); g.player.playerKnown = 7; g.cursor.preview = 123; g.world.editor = 9;
    assert.throws(() => withdrawGameplayForState({}, session), TypeError); assert.throws(() => withdrawGameplayForState(use, {}), /같은 현재/); withdrawGameplayForState(use, session); assert.throws(() => g.requireSession(), /미연결/); use.release(); assert.deepEqual([g.player.playerKnown, g.cursor.preview, g.world.editor], [7, 123, 9]); assert.throws(() => withdrawGameplayForState(use, session), TypeError);
});

test('Gameplay checking 실제 대여: E400 이월과 finder가 같은 현재 진단/권한을 재대여 뒤에도 읽는다', () => {
    const g = new ClientGameplayGlobals(); const state = g.cursorGlobals().lifecycle(); const checking = g.aiDebug().checking(); checking.value = 0x80000000; assert.equal(state.checkingValue(), 0x80000000);
    const use = g.acquire(); assert.equal(state.checkingValue(), 0x80000000); checking.value = 0; assert.equal(state.checkingValue(), 0); use.release(); checking.value = -1; const prepared = g.acquirePrepared(); prepared.release(); assert.equal(state.checkingValue(), 0xffffffff);
    const { actual } = runtime(); const raw = actual.core.pool.allocated(actual.parent); raw[11] |= 2; g.boss().value = false; assert.throws(() => new RawContainedFinder(actual.core.pool, g.finder).begin(actual.parent), /dead 부모 종속 조회 assert/); let assertions = 0;
    g.finder.assertion = () => { assertions += 1; checking.value = 0; }; new RawContainedFinder(actual.core.pool, g.finder).begin(actual.parent); assert.equal(assertions, 1); new RawContainedFinder(actual.core.pool, g.finder).begin(actual.parent); assert.equal(assertions, 1); raw[11] &= ~2; actual.world.clearProcesses();
});

test('Gameplay 장기 진단/제한/캐시: 일반·준비·반납이 값을 지우지 않고 Debug 주소는 같은 현재 DWORD만 쓴다', () => {
    const g = new ClientGameplayGlobals(); const fields = ['denySalvage', 'denyAscend', 'patchDiagnostic', 'patchSample', 'drawWater', 'hideBuildings', 'showGameTime', 'battleClockStamp', 'battleClockOrigin', 'mapRevision', 'worldCells', 'gameServerPort', 'fortKeepGenus', 'stateAuxFC4', 'localAddress'];
    // 원문 제한/진단의 전체 DWORD와 signed 두 칸을 대여 전후 같은 주소에서 읽는다.
    for (const prepared of [false, true]) { const refs = fields.map((name, i) => { const ref = g[name](); ref.value = i + 17; return ref; }); const lease = prepared ? g.acquirePrepared() : g.acquire(); lease.release(); fields.forEach((name, i) => { assert.equal(g[name](), refs[i]); assert.equal(refs[i].value, i + 17); }); }
    // 원문 공개된 Debug 식별자들은 bool이나 임의 포인터로 바꾸지 않는다.
    for (const address of [0x54d91c, 0x54d920, 0x54d924, 0x54d92c, 0x595144, 0x55a480, 0x55a484, 0x5e4794, 0x52f3cc]) { g.writeDebugWord(address, 0x80000063); assert.equal(g.readDebugWord(address), 0x80000063); }
    assert.equal(g.aiDebug().checking().value, 0x80000063 | 0); assert.equal(g.specialAuxiliary, 0x80000063); assert.equal(g.player.special, 0x80000063); assert.throws(() => g.readDebugWord(0x12345678), RangeError); assert.throws(() => g.writeDebugWord(0x12345678, 29), RangeError);
    const diagnostic = g.sidRobustDiagnostics(); diagnostic.lastRobust = 0x80000063; diagnostic.type = 177; const lease = g.acquire(); lease.release(); assert.equal(g.sidRobustDiagnostics(), diagnostic); assert.deepEqual(diagnostic, { lastRobust: 0x80000063, type: 177 });
});

test('Gameplay 현재 owner 교체: 이미 보관한 pin 함수도 같은 Session의 최신 전체 owner를 다시 읽는다', () => {
    const g = new ClientGameplayGlobals(); const { actual, session, owner } = runtime(); const use = g.acquirePrepared(); use.publish(session); g.bindRuntimeOwner(session, owner); const captured = session.pin; const replacement = { session, world: actual.world, pixels: new Uint8Array(64) };
    g.bindRuntimeOwner(session, replacement); const pin = captured(); assert.equal(pin.owner, replacement); assert.equal(pin.owner.world, actual.world); pin.release(); use.withdraw(session); use.release(); actual.world.clearProcesses();
});

test('Gameplay 실제 Start 제한: 설정/SP 효과 사이의 장기 두 DWORD store가 owner에서 즉시 보인다', () => {
    const g = new ClientGameplayGlobals(); const globals = createMissionStartGlobals(); globals.shared.denySalvage = g.denySalvage(); globals.shared.denyAscend = g.denyAscend(); const gate = createMissionFrameGateState();
    const body = new RawMissionStart(gate, createMissionStartState(), globals, { get: (key) => { if (key === 'denySalvage') return '-17'; if (key === 'denyAscend') { assert.equal(g.denySalvage().value, -17); return '2147483647'; } if (key === 'allowAnyCapture') assert.equal(g.denyAscend().value, 2147483647); return ''; }, sectionNames: () => '@', set: () => {}, refreshSp: () => { g.denySalvage().value = -29; g.denyAscend().value = 37; }, clearTechnology: () => {}, addTechnology: () => {}, hasPriest: () => false, refreshCommandGump: () => {}, resetCamera: () => {}, gameSeconds: () => 1 });
    body.start(); assert.equal(gate.started, 1); assert.deepEqual([g.denySalvage().value, g.denyAscend().value], [-29, 37]); assert.deepEqual([globals.denySalvage, globals.denyAscend], [0, 0]);
});

test('Gameplay weak owner: 별도 Node GC에서 pin 동안 실제 전체 owner를 유지하고 마지막 반납 뒤 수명 종료를 거부한다', () => {
    const url = new URL('../www/core/client-gameplay-globals.js', import.meta.url).href;
    const program = `import assert from 'node:assert/strict';import {ClientGameplayGlobals} from ${JSON.stringify(url)};
// 독립 Node의 현재 전역·대여·pin 설치 입구와 실제 자식 배열을 가진 전체 owner다.
const g=new ClientGameplayGlobals();const use=g.acquirePrepared();const s={bindRuntimePin(fn){this.pin=fn;}};use.publish(s);let owner={session:s,children:new Uint8Array(1024)};const watch=new WeakRef(owner);g.bindRuntimeOwner(s,owner);const pin=s.pin();owner=null;await new Promise(setImmediate);global.gc();assert.ok(watch.deref());assert.ok(g.runtimeCallActive(s));pin.release();await new Promise(setImmediate);
// job 경계마다 GC를 진행하며 마지막 pin 반납 후 weak 수명 종료를 실제로 관찰한다.
for(let i=0;i<32;i++){global.gc();if(!watch.deref())break;await new Promise(setImmediate);}assert.equal(watch.deref(),undefined);assert.throws(()=>s.pin(),/수명 종료/);assert.equal(g.runtimeCallActive(s),false);use.release();`;
    const child = spawnSync(process.execPath, ['--expose-gc', '--input-type=module', '-e', program], { encoding: 'utf8', timeout: 15000 }); assert.equal(child.status, 0, child.stderr || child.error?.message);
});
