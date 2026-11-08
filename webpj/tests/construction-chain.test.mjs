// 건설 계열 실제 모듈 통합: 로컬 예측 → 서버 확정 → 배치 통지(예측 정리 → 확정 조각 배치)를 실제 Confirm·Notice·Clear·Place 와 실제 SID 생성/소유자/보상/SP 저장소로 한 번에 잇는다
// (cpppj `ConstructionClear_ReplacesLocalPredictionWithConfirmedPieces` 와 `ConstructionConfirm_CreatesServerSidsThatPlacementUses` 에 대응).
// Pop 과 삭제는 탐색에 필요한 해시 등록/해제만 하는 경계이며 실제 통지 처리기가 abstract 를 판정한다.
//  - 1) 클라이언트 영역 SID 아홉 개로 noIsland 예측 배치(abstract)를 한다.
//  - 2) 서버 확정은 서버 영역 SID 아홉 개를 만들어 통지를 보내고 같은 통지를 직접 처리한다: 예측 아홉 개가 칸 순서대로 같은 조각 플래그(0x2000000)로 모두 지워지고(SID 반납), 칸마다 서버 영역의 확정 조각 하나만 남는다.
//  - 3) 확정 조각은 abstract 가 아니고(HP 보유 비트가 없는 타입) 소유자 2·시작 칸 단어 0x5d4·Pop 플래그 0x41 이며 통지의 마지막 갱신은 정확히 한 번이고 최근 건설 위치 이력이 새 좌표로 바뀐다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E139: 건설 확정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';
import { ConstructionPlaceState, RawConstructionPlace, makeConstructionPlaceHooks } from '../www/core/raw-construction-place.js';
import { ConstructionClearState, RawConstructionClear, SAME_PIECE_FLAG } from '../www/core/raw-construction-clear.js';
import { ConstructionNoticeState, RawConstructionNotice, makeConstructionNoticeConfirmHooks, makeConstructionNoticeHooks } from '../www/core/raw-construction-notice.js';
import { ConstructionConfirmState, RawConstructionConfirm, makeConstructionConfirmHooks } from '../www/core/raw-construction-confirm.js';

// noIsland 타입 번호, 풀 크기, 서버 영역 첫 번호, 예측 조각 수이다. (2026-11-09 WB04-E139)
const NO_ISLAND = 157;
const CAPACITY = 24000;
const SERVER_FIRST = 15000;
const PIECES = 9;

// 실행기의 frame_codes(0=기본)와 같은 프레임 코드 표이다. (2026-11-09 WB04-E139)
function inputFrames() {
    const codes = [];
    for (let side = 65; side <= 80; side += 1) {
        for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
        codes.push({ side, variant: 80, number: 11, flags: 0x20 });
        codes.push({ side, variant: 80, number: 12, flags: 0x40 });
    }
    codes.push(codes[0]);
    return codes;
}

test('건설 통합: 로컬 예측 아홉 칸이 서버 확정 조각으로 바뀐다(실제 Confirm·Notice·Clear·Place)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, constructorAddress: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    Object.assign(types[NO_ISLAND], { footX: 1, footY: 1, group: 1 });
    frames[NO_ISLAND] = { frames: inputFrames(), defaultFrame: 3 };
    const factory = new SquidFactory(pool, types);
    const books = new SquidPostPopState();
    const owner = new SquidOwner(pool, types, books, new SquidOwnerMode());
    const rewardState = new SquidRewardState();
    const store = new ScrambledSpStore(new GameRandom(), () => 1);
    const reward = new SquidReward(pool, types, books, rewardState, store);
    const placeState = new ConstructionPlaceState();
    placeState.localPlayer = 2;
    const pops = [];
    const placeHooks = {
        notifySurface: () => {}, sendMoney: () => {}, format: () => '', tellOthers: () => {}, tellPlayer: () => {}, tell: () => {}, sendReject: () => {},
        // Pop 경계: 좌표를 쓰고 void 를 끈 뒤 해시 0단계 체인의 머리에 잇는다.
        pop: (sid, x, y, flags) => {
            const raw = pool.allocated(sid);
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            raw[11] &= ~4;
            put(raw, 4, hash.bucket(0, x, y), 2);
            hash.setBucket(0, x, y, sid);
            pops.push([sid, flags]);
        },
        take: () => 0, setOwner: () => {}, storedSp: () => 0, addLocalSp: () => {}, addPlayerSp: () => {},
    };
    const place = new RawConstructionPlace(pool, types, frames, placeState, reward, makeConstructionPlaceHooks(pool, factory, owner, reward, store, placeState, placeHooks));
    // 1) 로컬 예측: 클라이언트 영역 SID 아홉 개로 abstract 배치를 한다.
    const predicted = [];
    for (let index = 0; index < PIECES; index += 1) predicted.push(factory.create(NO_ISLAND, 2));
    place.place({ type: NO_ISLAND, x: 20, y: 21, argument: 0, direction: 0, player: 2, sids: predicted, abstract: 1, quality: 0 });
    assert.ok(predicted[0] < SERVER_FIRST && (pool.slot(predicted[0])[40] & 1) === 1);
    pops.length = 0;
    // 2) 통지 처리: 예측 조각을 지우는 삭제 경계는 체인에서 빼고 SID 를 반납한다.
    const removed = [];
    const clear = new RawConstructionClear(pool, hash, types, frames, new ConstructionClearState(), {
        destroy: (sid, flags) => {
            assert.equal(flags, SAME_PIECE_FLAG);
            removed.push(sid);
            // 이 장면에서는 칸마다 예측 조각 하나만 등록돼 있으므로 버킷의 머리가 바로 그 조각이다.
            const raw = pool.allocated(sid);
            const x = new DataView(raw.buffer, raw.byteOffset + 14, 4).getFloat32(0, true);
            const y = new DataView(raw.buffer, raw.byteOffset + 18, 4).getFloat32(0, true);
            assert.equal(hash.bucket(0, x, y), sid);
            hash.setBucket(0, x, y, raw[4] | (raw[5] << 8));
            raw[11] |= 4;
            pool.release(sid);
        },
    });
    const noticeState = new ConstructionNoticeState();
    noticeState.localPlayer = 2;
    let refreshed = 0;
    const noticeBase = {
        moveBuilder: () => {}, findPriest: () => 0, immobile: () => true, approach: () => ({ x: 0, y: 0 }), tryMove: () => false, destroy: () => {}, start: () => {},
        refresh: () => { refreshed += 1; }, containedType: () => 0, removeContained: () => {},
    };
    const handler = new RawConstructionNotice(pool, types, noticeState, makeConstructionNoticeHooks(pool, clear, place, noticeBase));
    const confirmState = new ConstructionConfirmState();
    const sent = [];
    const confirmHooks = {
        findBuilder: () => 77, broadcast: (notice) => sent.push(notice), handle: () => {}, recorderActive: () => false, record: () => {}, create: () => 0,
    };
    const confirm = new RawConstructionConfirm(pool, types, frames, confirmState,
        makeConstructionConfirmHooks(pool, factory, makeConstructionNoticeConfirmHooks(pool, handler, confirmHooks)));
    assert.equal(confirm.confirm({ type: NO_ISLAND, x: 20, y: 21, player: 2, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 }), true);
    assert.equal(refreshed, 1);
    assert.equal(sent.length, 1);
    assert.equal(noticeState.history[0].x, 20);
    assert.equal(noticeState.history[0].y, 21);
    // 예측 아홉 개가 칸 순서대로 모두 지워지고, 칸마다 서버 영역 번호의 확정 조각 하나만 남는다.
    assert.deepEqual(removed, predicted);
    const finder = new RawSquidFinder(pool, hash, types);
    let found = 0;
    // 아홉 칸 전체 범위에 남은 객체를 센다.
    for (let current = finder.begin({ left: 20, top: 21, right: 22, bottom: 23 }); current !== 0; current = finder.next()) {
        const raw = pool.slot(current);
        found += 1;
        assert.ok(current >= SERVER_FIRST, `서버 영역 ${current}`);
        assert.equal(raw[40] & 1, 0);
        assert.equal(raw[34], 2);
        assert.equal(raw[10], NO_ISLAND);
    }
    assert.equal(found, PIECES);
    // 확정 조각의 Pop 플래그는 0x41 이다(abstract 아님).
    assert.equal(pops.length, PIECES);
    assert.ok(pops.every(([, flags]) => flags === 0x41));
    // 시작 칸만 단어 0x5d4 를 받는다.
    const words = sent[0].sids.map((sid) => pool.slot(sid)[12] | (pool.slot(sid)[13] << 8));
    assert.equal(words[0], 0x5d4);
    assert.ok(words.slice(1).every((word) => word === 0));
});
