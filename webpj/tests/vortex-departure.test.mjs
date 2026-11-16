// RawVortexDeparture의 기준선 계약을 실제 Pool/Hash/ChunkMap/IslandRuntime/Ascend로 대조한다(WB04-E587).
// 직접 cpppj 전용 시험/원본 관측은 없다. 소스의 독립 입력/효과 순서·공유 바이트·오류 접두를 검사하며 게임을 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ChunkMap } from '../www/assets/territory.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawIslandRuntime, createRawIslandStorage, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { RawVortexAscend } from '../www/core/raw-vortex-ascend.js';
import { RawVortexDeparture } from '../www/core/raw-vortex-departure.js';
import { GameRandom } from '../www/core/random.js';
import { put, get, floatToBits } from './raw-support.mjs';

// 선택하지 않은 큰 효과를 성공으로 채우지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 같은 실제3204BYTE 섬 목록과16×16청크/65536칸 표면을 구성한다. 입력 TYPE은 각 계약에 맞춰 명시한다.
function scene(extra = {}) {
    const s = { pool: new SidPool(24000, { server: false }), hash: new SquidHash(), chunks: new ChunkMap(), types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, hotspotX: 0, hotspotY: 0, group: 0 })), records: createRawIslandStorage(), local: { value: 1 }, server: { value: false }, revision: { value: 0 }, events: [], body: null };
    s.islands = new RawIslandRuntime(s.pool, s.hash, s.records, createFortFinishOwnerState(), new SquidPostPopList(), { value: 0 }, { value: 0 }, { setResidenceOwner: forbidden, assertion: forbidden }); s.islands.reset(); s.islands.add(1, 7);
    s.chunks.at(1, 1).island = 0; s.chunks.at(1, 1).record.set([2, 9, 0x11, 4, 5, 6]);
    s.ascend = new RawVortexAscend(s.pool, s.hash, new Uint8Array(65536), s.types, s.islands, new GameRandom(1), { noIslandType: 157 }, { islandNumber: () => 0, reserveCandidateBuffer: forbidden, releaseCandidateBuffer: forbidden, destroy: forbidden });
    s.hooks = {
        assertion: (...args) => s.events.push(['assert', ...args]), state: (sid, word) => { s.events.push(['state', sid, word]); put(s.pool.physical(sid), 12, word, 2); },
        readTerritory: forbidden, writeTerritory: forbidden, refreshProduction: () => s.events.push(['refresh']), snapshot: () => s.events.push(['snapshot']),
        owner: (sid, owner) => { s.events.push(['owner', sid, owner]); s.pool.physical(sid)[34] = owner; }, destroy: (sid, flags) => s.events.push(['destroy', sid, flags]),
        islandNumber: () => 0, ownedIslands: (owner) => { s.events.push(['owned', owner]); return 0; }, requestEdit: () => s.events.push(['edit']),
        log: (text) => s.events.push(['log', text]), sendExcept: (sender, packet) => s.events.push(['relay', sender, packet]), regular: (sid) => s.events.push(['regular', sid]), warnAscend: (owner) => s.events.push(['warn', owner]), sound: (name) => s.events.push(['sound', name]), ...extra,
    };
    s.make = (hooks = s.hooks) => new RawVortexDeparture(s.pool, s.hash, s.types, s.chunks, s.islands, s.ascend, s.local, s.server, s.revision, hooks);
    s.body = s.make(); return s;
}
// 실제 SID 하나에 지정한 TYPE/좌표/owner만 쓴다. 게임의 저장 객체 설치를 가장하지 않는다.
function actor(s, type = 70, owner = 2, x = 20, y = 20) { const sid = s.pool.allocate(2); const raw = s.pool.physical(sid); raw[10] = type; raw[11] = 0; raw[34] = owner; put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(y)); return sid; }
// 원문은 헤더 자체를 검증하지 않고8BYTE 중 지정 위치만 읽는다.
function packet(sid, island = 0, complete = 0) { const p = Uint8Array.of(1, 0x51, 8, 0, island, 0, 0, complete); put(p, 5, sid, 2); return p; }

test('VortexDeparture: 같은 PATCH 풀 계약과 실제 몸체/전역 참조를 유지한다', () => {
    const a = scene(); const b = scene(); assert.equal(a.body.pool, a.pool);
    assert.throws(() => new RawVortexDeparture(a.pool, a.hash, a.types, a.chunks, b.islands, a.ascend, a.local, a.server, a.revision), /같은 PATCH/);
    assert.throws(() => new RawVortexDeparture(a.pool, a.hash, a.types, a.chunks, a.islands, b.ascend, a.local, a.server, a.revision), /같은 PATCH/);
});

test('VortexDeparture 지역: 표면 WORD/지역 signed BYTE와 CRT 변환·현재 noIsland 진단 순서를 지킨다', () => {
    const s = scene(); const surface = actor(s, 71); s.types[71].flags2 = 2; put(s.pool.physical(surface), 8, 0, 2); s.hash.setCell(0, 20, 20, surface); s.records[16] = 0x85;
    const fort = { value: true }; const noIsland = { value: 99 };
    assert.equal(s.body.fortTerritoryAt(20.99, 20.5, fort, noIsland), -123); assert.deepEqual(s.events, []);
    assert.equal(s.body.fortTerritoryAt(4294967296, 20, fort, { value: 0 }), -123);
    fort.value = false; assert.throws(() => s.body.fortTerritoryAt(NaN, 20, fort, noIsland), /CRT/); assert.deepEqual(s.events, []);
    s.types[71].flags2 = 0; const calls = []; s.body = s.make({ ...s.hooks, assertion: (expression, file, line) => { calls.push([expression, line]); if (line === 0x4cf) noIsland.value = 0; } });
    assert.equal(s.body.fortTerritoryAt(20, 20, fort, noIsland), -123); assert.deepEqual(calls, [['inFortMode', 0x344], ['genus & (gISLAND|gBRIDGE|gISLAND3x3)', 0x4cf]]);
    noIsland.value = -1; assert.throws(() => s.body.fortTerritoryAt(256, 20, fort, noIsland), /섬100개/); assert.equal(calls.at(-1)[1], 0x348);
});

test('VortexDeparture 기록: 첫 진단 전6BYTE 사본과 y/x 쓰기 순서·동일 record 주소를 보존한다', () => {
    const s = scene(); s.chunks = new ChunkMap(2, 2); const before = [];
    // 네 청크를 한 섬에 두고 원문 행 우선 쓰기를 관찰한다.
    for (let y = 0; y < 2; ++y) for (let x = 0; x < 2; ++x) { const e = s.chunks.at(x, y); e.island = 0; before.push(e.record); }
    const record = Uint8Array.of(1, 2, 3, 4, 5, 6); put(s.records, 8, 0); const trace = []; const at = s.chunks.at.bind(s.chunks); s.chunks.at = (x, y) => { trace.push([x, y]); return at(x, y); };
    s.body = s.make({ ...s.hooks, assertion: () => { trace.push('assert'); record.fill(0xee); }, refreshProduction: () => trace.push('refresh') }); s.body.updateIslandRecord(0, record);
    assert.deepEqual(trace, ['assert', [0, 0], [1, 0], [0, 1], [1, 1], 'refresh']);
    // 기록 주소와 원래 인수 여섯 바이트를 각각 확인한다.
    for (const bytes of before) assert.deepEqual([...bytes], [1, 2, 3, 4, 5, 6]); assert.equal(before[0], at(0, 0).record); assert.throws(() => s.body.updateIslandRecord(0, new Uint8Array(5)), /6BYTE/);
});

test('VortexDeparture 시작: relay가 바꾼 패킷 SID와 최초 신전의 현재 owner를 구별한다', () => {
    const s = scene(); const first = actor(s, 70, 2); const next = actor(s, 70, 3); const p = packet(first); s.server.value = true;
    s.body = s.make({ ...s.hooks, sendExcept: (sender, bytes) => { assert.equal(bytes, p); s.events.push(['relay', sender]); put(p, 5, next, 2); s.pool.physical(first)[34] = 4; s.local.value = 3; } });
    s.body.receive(-2, p); assert.deepEqual(s.events, [['relay', -2], ['regular', next], ['warn', 4], ['sound', 'enemyIslandPreparing.wav'], ['sound', 'islandRise.wav']]);
});

test('VortexDeparture 시작: 내 신전·client는 relay/경고 없이 같은 일반 예약과 소리만 호출한다', () => {
    const s = scene(); const temple = actor(s, 70, 1); const hooks = { ...s.hooks, sendExcept: null, warnAscend: null }; s.body = s.make(hooks); hooks.sound = forbidden; s.body.receive(9, packet(temple));
    assert.deepEqual(s.events, [['regular', temple], ['sound', 'preparingAscend.wav']]);
});

test('VortexDeparture 누락/실패: 짧은 패킷은 읽기 전 거부하고 free 진단→없는 섬 로그의 순서를 지킨다', () => {
    const s = scene(); assert.throws(() => s.body.receive(9, new Uint8Array(7)), /8BYTE/); assert.deepEqual(s.events, []);
    const temple = actor(s); s.pool.physical(temple)[11] = 1; s.body.receive(9, packet(temple, 99)); assert.deepEqual(s.events, [['assert', '!vortex.isFree()', '\\Ns\\O\\Vortex.cpp', 0x14c], ['log', "Can't ascend island 99\n"]]);
    s.pool.physical(temple)[11] = 0; s.body = s.make({ ...s.hooks, owner: null }); assert.throws(() => s.body.remove(temple, 0), /실제 장치 미연결/); assert.equal(get(s.pool.physical(temple), 12, 2), 3); assert.equal(s.islands.exists(0), 1);
});

test('VortexDeparture 완료: 로그가 바꾼 현재 패킷의 섬/SID를 다시 읽고 기존 신전은 바꾸지 않는다', () => {
    const s = scene(); s.islands.add(2, 8); s.chunks.at(2, 2).island = 1; const first = actor(s); const next = actor(s); const p = packet(first, 0, 1);
    s.body = s.make({ ...s.hooks, log: (text) => { s.events.push(['log', text]); p[4] = 1; put(p, 5, next, 2); } }); s.body.receive(9, p);
    assert.equal(s.islands.exists(0), 1); assert.equal(s.islands.exists(1), 0); assert.equal(get(s.pool.physical(first), 12, 2), 0); assert.equal(get(s.pool.physical(next), 12, 2), 3); assert.equal(s.revision.value, 1); assert.deepEqual(s.events, [['log', 'Removing isle 0\n'], ['state', next, 3], ['owner', next, 0]]);
});

test('VortexDeparture 자기 저장: 지역 사본/첫 빈 슬롯·owner 값 인수와 마지막 현재 전역·revision 감김을 보존한다', () => {
    const s = scene(); const temple = actor(s, 70, 1); let saved; s.revision.value = 0xffffffff;
    s.body = s.make({ ...s.hooks, readTerritory: (owner, records) => { assert.equal(owner, 1); assert.ok(records.every((v) => v === 0)); records[1] = 1; s.pool.physical(temple)[34] = 2; s.local.value = 2; s.chunks.at(1, 1).record.fill(0xee); }, writeTerritory: (owner, records) => { assert.equal(owner, 1); saved = records.slice(); s.events.push(['write']); } }); s.body.remove(temple, 0);
    assert.deepEqual([...saved.subarray(6, 12)], [2, 3, 0x11, 4, 5, 6]); assert.equal(saved[1], 1); assert.deepEqual([...s.chunks.at(1, 1).record], [2, 3, 0x11, 4, 5, 6]); assert.equal(s.islands.exists(0), 0); assert.equal(s.revision.value, 0);
    assert.deepEqual(s.events, [['state', temple, 3], ['write'], ['refresh'], ['snapshot'], ['owner', temple, 0], ['owned', 2], ['edit']]);
});

test('VortexDeparture home 저장: 기존20행의 home 비트만 지우고 다른 바이트·record를 유지한다', () => {
    const s = scene(); const temple = actor(s, 70, 1); s.chunks.at(1, 1).record[1] = 5; let saved;
    s.body = s.make({ ...s.hooks, readTerritory: (_owner, records) => records.fill(0xff), writeTerritory: (_owner, records) => { saved = records.slice(); } }); s.body.remove(temple, 0);
    // 각 원문6BYTE 기록의 flags만 0xfb이고 나머지 바이트는0xff다.
    for (let n = 0; n < 120; ++n) assert.equal(saved[n], n % 6 === 1 ? 0xfb : 0xff); assert.equal(s.chunks.at(1, 1).record[1], 1);
});

test('VortexDeparture 표면: 반환 뒤 x+1 커서의 네 이웃을 현재 조회하고 받침 삭제를 뒤에 실행한다', () => {
    const s = scene(); const temple = actor(s); s.types[71].flags2 = 2; s.types[72].flags2 = 4;
    const island = actor(s, 71, 2, 20, 20); const right = actor(s, 72, 2, 22, 20); const up = actor(s, 72, 2, 21, 19); const down = actor(s, 72, 2, 21, 21); const left = actor(s, 72, 2, 19, 20);
    const points = new Map([[island, [20, 20]], [right, [22, 20]], [up, [21, 19]], [down, [21, 21]], [left, [19, 20]]]);
    // 같은 실제 표면 지도에 서로 다른 다리/받침을 넣는다.
    for (const [sid, [x, y]] of points) s.hash.setCell(0, x, y, sid);
    s.body = s.make({ ...s.hooks, destroy: (sid, flags) => { s.events.push(['destroy', sid, flags]); const [x, y] = points.get(sid); s.hash.setCell(0, x, y, 0); } }); s.body.remove(temple, 0);
    assert.deepEqual(s.events.filter((e) => e[0] === 'destroy'), [['destroy', right, 0x100048], ['destroy', up, 0x100048], ['destroy', down, 0x100048], ['destroy', island, 0x848]]); assert.equal(s.hash.cell(0, 19, 20), left);
});

test('VortexDeparture 추가 후보: 첫 genus를 보존하고 섬 조회 뒤 직접 청크 판정을 실행한다', () => {
    const s = scene(); const temple = actor(s); const candidate = actor(s, 75); s.types[75].flags2 = 0x80000; const raw = s.pool.physical(candidate); put(raw, 4, s.hash.bucket(1, 20, 20), 2); s.hash.setBucket(1, 20, 20, candidate);
    s.body = s.make({ ...s.hooks, islandNumber: () => { s.events.push(['island']); raw[10] = 76; return 99; } }); s.body.remove(temple, 0);
    assert.ok(s.events.some((e) => e[0] === 'island')); assert.ok(s.events.some((e) => e[0] === 'destroy' && e[1] === candidate && e[2] === 0x848)); assert.equal(raw[10], 76);
});

test('VortexDeparture 추가 후보: y-4 청크 분기와 보호 genus의 최종 거부를 구분한다', () => {
    // 동일 실제 월드 구조에 두 독립 genus 입력을 적용한다. 유닛/전투 행동을 생성하지 않는다.
    for (const protectedType of [false, true]) {
        const s = scene(); const temple = actor(s); s.chunks.at(1, 0).island = 0; s.chunks.at(1, 1).island = protectedType ? 0 : 1;
        const candidate = actor(s, 75, 2, 20, 17); s.types[75].flags2 = protectedType ? 0x1080000 : 0x1000; put(s.pool.physical(candidate), 4, s.hash.bucket(1, 20, 17), 2); s.hash.setBucket(1, 20, 17, candidate);
        s.body.remove(temple, 0); const destroyed = s.events.some((e) => e[0] === 'destroy' && e[1] === candidate); assert.equal(destroyed, !protectedType);
    }
});

test('VortexDeparture 실패 접두: snapshot 예외는 이미 저장한6BYTE를 남기고 owner/섬/revision 정리는 실행하지 않는다', () => {
    const s = scene(); const temple = actor(s, 70, 1); let writes = 0;
    s.body = s.make({ ...s.hooks, readTerritory: () => {}, writeTerritory: () => { ++writes; }, snapshot: () => { throw new Error('snapshot 중단'); } });
    assert.throws(() => s.body.remove(temple, 0), /snapshot 중단/); assert.equal(writes, 1); assert.equal(s.chunks.at(1, 1).record[1], 3); assert.equal(s.pool.physical(temple)[34], 1); assert.equal(get(s.pool.physical(temple), 12, 2), 3); assert.equal(s.islands.exists(0), 1); assert.equal(s.revision.value, 0); assert.deepEqual(s.events, [['state', temple, 3], ['refresh']]);
});
