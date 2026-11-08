// 신전 가상 소유자 raw-vortex-owner.js(RawVortexOwner)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: vortex-owner-x86.tsv 10.78(originals) 864행 — 입력 6열(옛 소유자·새 소유자·슬롯 상태·extra·초기 신전 DWORD·외부 효과 변이 종류) → 외부 효과 호출 순서(I 섬 소유자 / P 지역 소유 좌표 비트 / R 생산 갱신;
//    원본 내부 공통 Owner 호출 표식 B 는 외부 효과가 아니므로 제외), 슬롯 50 raw, 아홉 Player 의 신전 네 DWORD, 공통 Owner 의 아홉 소유자 목록(활성 밖 stale 포함), revision. CD 10.72 행(864)은 웹 범위(10.78) 밖이다.
//  - 계약: VortexOwner_RejectsForeignTableAndPlayerSize(미복원 가상 표/잘못된 9기록은 최초 raw/Player 변경 전에 거부) + 추가 목록 범위 오류.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E79: 신전 소유자 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState, SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawVortexOwner } from '../www/core/raw-vortex-owner.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E79)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vortex-owner-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 SID, 판본당 독립 행 수, 신전 가상 표, 10.78 Player 한 칸 폭과 초기 신전 DWORD 위치이다. (2026-11-08 WB04-E79)
const SOURCE = 50;
const ROWS = 864;
const VORTEX_TABLE = 0x519860;
const STRIDE = 0xb4;
const TEMPLE_AT = 0x7c;

// 예외가 나는지 본다. (2026-11-08 WB04-E79)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E79)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 현재 풀/9 Player/소유자별 목록을 원본과 같은 초기 입력으로 준비하는 장면이다. (2026-11-08 WB04-E79)
class VortexScene {
    constructor(row) {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.book = new SquidPostPopState();
        this.mode = new SquidOwnerMode();
        this.players = new Uint8Array(9 * STRIDE).fill(0xa5);
        this.outposts = new SquidPostPopList();
        this.revision = { value: 37 };
        this.paints = 0;
        this.events = [];
        // 기대값 대신 입력 필드만 읽는다.
        this.input = row[1].split(',').map(Number);
        assert.equal(this.input.length, 6);
        // 이전 raw 관찰과 같은 명시 번호를 확보한다.
        for (let sid = 5; sid < 128; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.types[82].flags2 = 0x200;
        this.mode.battle = true;
        // 기존 공통 Owner 가 쓰는 아홉 실제 목록을 같은 입력으로 준비한다.
        for (const list of this.book.ownerFactories) {
            list.entries = [1, 2, 3, 4, 5, 6];
            list.count = 0;
        }
        this.owner = new SquidOwner(this.pool, this.types, this.book, this.mode);
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, VORTEX_TABLE);
        raw[10] = 82;
        raw[11] = this.input[2];
        raw[34] = this.input[0];
        raw[40] = this.input[3];
        put(raw, 14, floatToBits(40.5));
        put(raw, 18, floatToBits(50.25));
        // 신전 네 DWORD 만 원본 입력으로 채우고 다른 Player BYTE 는 유지한다.
        for (let player = 0; player < 9; player += 1) {
            put(this.players, this.temple(player), this.input[4]);
            put(this.players, this.temple(player) + 4, 0x11223344);
            put(this.players, this.temple(player) + 8, 0x55667788);
            put(this.players, this.temple(player) + 12, 0x99aabbcc);
        }
        this.outposts.entries = [60, 61, 62];
        this.outposts.count = 2;
        // 원본 추가 목록의 현재 owner·좌표 입력이다. free/dead 여부로 후보를 거르지 않는다.
        for (let index = 0; index < 3; index += 1) {
            const other = this.pool.allocated(60 + index);
            other[34] = [this.input[0], this.input[1], 8][index];
            put(other, 14, floatToBits(70.5 + index));
            put(other, 18, floatToBits(80.25 + index));
        }
    }

    // 신전 DWORD 의 Player 저장 위치이다.
    temple(player) { return player * STRIDE + TEMPLE_AT; }

    // 원본과 같은 명시 외부 효과 변경을 공급하며 신전 몸체 결과는 계산하지 않는다.
    hooks() {
        return {
            islandNumber: () => 3,
            islandExists: () => true,
            setIslandOwner: (island, player) => {
                this.events.push(`I:${island}:${player}`);
                if (this.input[5] === 1) {
                    const raw = this.pool.allocated(SOURCE);
                    raw[34] = 8;
                    put(raw, 14, floatToBits(42.75));
                    put(raw, 18, floatToBits(60.5));
                }
            },
            regionOwnership: (x, y) => {
                this.events.push(`P:${floatToBits(x)}:${floatToBits(y)}`);
                if (this.paints === 0) {
                    if (this.input[5] === 1 || this.input[5] === 3) this.outposts.count = 3;
                    if (this.input[5] === 2) { this.outposts.count = 0; this.pool.allocated(SOURCE)[40] = 1; }
                    if (this.input[5] === 3) this.pool.allocated(SOURCE)[34] = 1;
                }
                this.paints += 1;
            },
            refreshProduction: () => {
                this.events.push('R');
                if (this.input[5] === 3) this.revision.value = 0xffffffff;
            },
            missingIsland: () => { throw new Error('이번 정상 PE 입력은 missingIsland를 요청하지 않습니다'); }
        };
    }

    // 실제 Player 필드·공통 목록·raw 슬롯·revision 을 원본 출력 형식으로 수집한다.
    output() {
        const temples = [];
        // 아홉 Player 의 네 신전 DWORD 모두를 읽는다.
        for (let player = 0; player < 9; player += 1) for (let field = 0; field < 4; field += 1) temples.push(get(this.players, this.temple(player) + field * 4));
        // 기존 실제 공통 Owner 의 capacity 전체 stale 데이터도 대조한다.
        const lists = this.book.ownerFactories.map((list) => `${list.count}:${list.entries.join(',')}`);
        return [this.events.join(';'), hex(this.pool.slot(SOURCE)), temples.join(','), lists.join('|'), String(this.revision.value)];
    }
}

// 원본 내부 Base 호출표식은 독립 공통 Owner 결과로 확인하고 외부 콜백 순서와 구별한다. (2026-11-08 WB04-E79)
const externalEvents = (value) => value.split(';').filter((event) => !event.startsWith('B:')).join(';');

test('VortexOwner: 원본 10.78 864입력의 현재 Player·가변 목록·효과 뒤 좌표/소유자·공통 Owner·revision 이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 구현 계산으로 기대값을 만들지 않고 같은 입력의 실제 PE 결과를 전수 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const scene = new VortexScene(row);
        const vortex = new RawVortexOwner(scene.pool, scene.owner, scene.players, scene.outposts, scene.revision, scene.hooks());
        assert.ok(vortex.handles(SOURCE));
        vortex.set(SOURCE, scene.input[1]);
        const expected = row.slice(2);
        expected[0] = externalEvents(expected[0]);
        assert.deepEqual(scene.output(), expected, `행 ${count}: 입력 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('VortexOwner: 미복원 가상 표/잘못된 9기록·다른 풀은 최초 raw/Player 변경 전에 거부하고 추가 목록 범위 오류를 거부한다', () => {
    const row = rows ? rows[0] : ['originals', '0,0,0,0,0,0', '', '', '', '', ''];
    const scene = new VortexScene(row);
    const before = Uint8Array.from(scene.players);
    assert.ok(throws(() => new RawVortexOwner(scene.pool, scene.owner, scene.players.subarray(0, 10), scene.outposts, scene.revision, scene.hooks())));
    assert.ok(throws(() => new RawVortexOwner(scene.pool, scene.owner, scene.players, scene.outposts, scene.revision, { ...scene.hooks(), refreshProduction: undefined })));
    assert.ok(throws(() => new RawVortexOwner(new SidPool(24000, { server: false }), scene.owner, scene.players, scene.outposts, scene.revision, scene.hooks())));
    const vortex = new RawVortexOwner(scene.pool, scene.owner, scene.players, scene.outposts, scene.revision, scene.hooks());
    put(scene.pool.allocated(SOURCE), 0, 0x11111111);
    assert.ok(throws(() => vortex.set(SOURCE, 1)));
    assert.ok(scene.events.length === 0 && Buffer.from(scene.players).equals(Buffer.from(before)));
    // 추가 목록의 항목이 풀 밖이면 거부한다(섬 소유자 변경과 첫 지역 소유 요청 뒤).
    put(scene.pool.allocated(SOURCE), 0, VORTEX_TABLE);
    scene.pool.allocated(SOURCE)[11] = 0;
    scene.pool.allocated(SOURCE)[40] = 0;
    scene.outposts.entries = [30000, 61, 62];
    scene.outposts.count = 1;
    assert.ok(throws(() => vortex.set(SOURCE, 1)));
    assert.ok(scene.events.length >= 2);
});
