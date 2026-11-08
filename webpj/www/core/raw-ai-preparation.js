// AI 의 현재 목표·지역·지갑·초기 월드 등록 준비를 같은 AI 본체에 적용하는 모듈이다(cpppj `o/RawAiPreparation.cpp/.h`, 10.78: 패치 00416eb0 준비 전체·00410300 적 선택·00411a80 부착 간헐천 수).
//  - new RawAiPreparation(memory, pool, players, allies, globals, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), pool 은 같은 PATCH SidPool(사제 HP·좌표), players 는 현재 Player 9기록(9×180 BYTE, +0x2c 참여 DWORD·+0x78 지역),
//    allies 는 방향별 9×9 DWORD(owner×9+target, 대칭화하지 않음), globals = { localOwner()(00540c70), playerKnown()(00595344 로컬 소유자 확정 여부), noRegion()(0050c038 지역 없음 번호, signed), invalidGraph()(00540410 현재 무효 그래프, 도달할 때만 요구), now()(0055b4d0 현재 QWORD 게임 시각) } 는 각 원본 위치에서 현재 값을 다시 읽고,
//    hooks = { missingEnemyAssertion()(00416eff enemyPlayerId 진단, 반환 뒤 계속), priest(owner)→SID(004921f0), temple(region)→SID(0046f260), maxHitPoints(sid)→정수(004adcb0, 현재 HP 는 호출 전에 raw+0x1a 에서 읽음), pointDistance(source, target)→거리(0041cfc0 두 raw 좌표 [x, y] 사이, 발 중심 보정 없음),
//    graph(sid)→DWORD(00462bc0), regionBounds(region)→[left, top, right, bottom](00432210→0040d900 청크 int<<4→float), storedSp(owner)→float(0040ec50 분산 SP), addLocalSp(delta)(0040eec0 로컬 SP·같은 AI 지갑·표시 갱신), demo()→정수(00463b50 현재 미션+68), knowledgeOpen()→bool(00445330 현재 005325c8==0 판정),
//    blockingGumpOpen()→bool(004641d0 루트 직계 차단 창 판정), initialScan(token)(00415720 전체를 원래 +0x25c 조건 위치에서 실행) } 는 도달한 위치에서만 요구한다.
//  - selectEnemy(token): 00410300 — +0x264 를 0 으로 비운 뒤 강제 대상(+0x240)이 있으면 사제/동맹 확인 없이 선택하고, 아니면 Player 1..8 중 현재 사제가 있고 현재 owner 행에서 동맹이 아닌 첫 owner 를 선택한다.
//  - countAttachedGeysers(token): 00411a80 — +0x324 포인터와 +0x338 개수(원본의 +0x32c 불일치를 교정하지 않음)로 각 간헐천의 현재 그래프가 본체 +0x298 과 같은 수를 센다(graph 하위가 개수/포인터/기준 그래프를 바꾸면 각 위치에서 다시 읽음).
//  - update(token): 00416eb0 — 적 재선택(미선택 또는 동맹) → 적 진단 → 데모 지식/차단 창 생각 기한(float rate + QWORD now) → 자기 신전(+0x294) → 능력 0x100 이면 참여 적 사제 중 최대 HP 절반 미만(HP 선저장)인 대상을 같은 동맹의 모든 참여 사제 거리로 비교(먼저 가까운 자신을 발견해도 중단하지 않음)해 현재 owner 가 최근접이면 선택
//    → 적 신전/그래프/지역 경계 중심(+0x2b0/+0x2b4, x87 배정도 중간 합 뒤 float 저장) → 소유/적 사제(+0x29c/+0x2a0) → 로컬 SP(FILD 정수 비교와 float 차액, 두 번째 읽기에서 현재 owner/지갑 재조회) → 최초 한 번 초기 월드 등록(+0x25c) → 부착 간헐천 수로 수집자 충족(+0x2cc)/간헐천 충족(+0x2d0, 12000 초과 지갑이면 1)을 쓴다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E181: AI 준비 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 Player0~8 의 물리 기록 수, 패치판 Player 한 기록의 BYTE 수, 최근접 초기 거리(00417027)와 중심 배율(00500ee4), 간헐천 부착 충족 지갑 기준(00417394)이다. (2026-11-09 WB04-E181)
const PLAYER_SLOTS = 9;
const PLAYER_BYTES = 180;
const NEAREST_DISTANCE = 99999.0;
const HALF = 0.5;
const RICH_ENOUGH = 12000;

// 비정렬 DWORD 를 지정 원문 범위 안에서 읽는다. (2026-11-09 WB04-E181)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 준비 현재 DWORD 물리 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정한 네 BYTE 만 덮어쓰고 다른 미초기화 원문을 유지한다. (2026-11-09 WB04-E181)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 준비 현재 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 선택된 원본 하위가 없으면 그 지점에서 명시 오류를 낸다. (2026-11-09 WB04-E181)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// DWORD 의 signed 분기를 보존한다. (2026-11-09 WB04-E181)
const signed = (value) => value | 0;

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E181)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 현재 사제 raw 좌표를 float 저장값 그대로 읽는다. (2026-11-09 WB04-E181)
const point = (raw) => [bitsFloat(read(raw, 14)), bitsFloat(read(raw, 18))];

// AI 준비 처리기이다. 별도 값형 AI 상태 없이 실제 0xb08 본체와 Player9·방향별 동맹을 공유한다. (2026-11-09 WB04-E181)
export class RawAiPreparation {
    #memory;
    #pool;
    #players;
    #allies;
    #globals;
    #hooks;

    // 본문이 선택하기 전에는 외부 월드 효과나 목록 포인터에 접근하지 않는다. (2026-11-09 WB04-E181)
    constructor(memory, pool, players, allies, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 준비 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !(players instanceof Uint8Array) || players.length !== PLAYER_SLOTS * PLAYER_BYTES || allies?.length !== PLAYER_SLOTS * PLAYER_SLOTS) {
            throw new TypeError('AI 준비 같은 PATCH/Player9/동맹81 계약 누락');
        }
        for (const name of ['localOwner', 'playerKnown', 'noRegion', 'now']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 준비 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#players = players;
        this.#allies = allies;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 조립 시 같은 실제 풀인지 확인하기 위한 풀이다. (2026-11-09 WB04-E181)
    get pool() { return this.#pool; }

    // 실제 소유 블록을 매번 다시 확인하며 값형 사본을 만들지 않는다. (2026-11-09 WB04-E181)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 준비 실제0xb08 본체 누락');
        return raw;
    }

    // 콜백 뒤에도 같은 token 의 현재 DWORD 를 다시 읽고, 해당 DWORD 에만 쓴다. (2026-11-09 WB04-E181)
    #field(token, offset) { return read(this.#body(token), offset); }
    #setField(token, offset, value) { write(this.#body(token), offset, value >>> 0); }

    // 손상된 owner 만 물리 범위 밖에서 거부하고 0번 기록은 허용한다. (2026-11-09 WB04-E181)
    #player(owner, offset) {
        if (owner >= PLAYER_SLOTS || offset > PLAYER_BYTES - 4) throw new RangeError('AI 준비 현재 Player 물리 범위');
        return read(this.#players, owner * PLAYER_BYTES + offset);
    }

    // 대칭화하지 않고 원본 owner 행을 현재 읽는다. (2026-11-09 WB04-E181)
    #allied(owner, target) {
        if (owner >= PLAYER_SLOTS || target >= PLAYER_SLOTS) throw new RangeError('AI 준비 현재 방향별 동맹 범위');
        return this.#allies[owner * PLAYER_SLOTS + target] !== 0;
    }

    // 현재 풀에 실제로 존재하는 물리 번호만 좁힌다(free/dead 필터는 추가하지 않음). (2026-11-09 WB04-E181)
    #physicalSid(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 준비 현재 사제 SID 물리 범위');
        return value;
    }

    // 00410300: 강제 대상은 사제/동맹 확인 없이 선택하고 일반 탐색은 사제 조회 뒤 현재 owner 행을 읽는다. (2026-11-09 WB04-E181)
    selectEnemy(token) {
        this.#setField(token, 0x264, 0);
        const forced = this.#field(token, 0x240);
        if (forced) {
            this.#setField(token, 0x264, forced);
            return;
        }
        // 첫 살아 있는 사제 보유 비동맹 플레이어에서 즉시 끝낸다.
        for (let owner = 1; owner < PLAYER_SLOTS; owner += 1) {
            if (need(this.#hooks.priest, 'AI 준비 현재 사제 조회 미연결')(owner) && !this.#allied(this.#field(token, 0x260), owner)) {
                this.#setField(token, 0x264, owner);
                return;
            }
        }
    }

    // 00411a80: 원본은 +0x324 목록을 +0x338 개수만 읽는다(이 불일치를 +0x32c 로 교정하지 않음). (2026-11-09 WB04-E181)
    countAttachedGeysers(token) {
        let count = 0;
        // graph 하위가 개수/포인터/기준 그래프를 바꿀 수 있으므로 각 원문 위치에서 다시 읽는다.
        for (let index = 0; index < signed(this.#field(token, 0x338)); index += 1) {
            const list = this.#memory.bytes(this.#field(token, 0x324));
            const sid = read(list, index * 4);
            const graph = need(this.#hooks.graph, 'AI 준비 간헐천 그래프 미연결')(sid);
            if (graph === this.#field(token, 0x298)) count += 1;
        }
        return signed(count);
    }

    // 00416eb0: 원본의 enemy 재선택·데모 기한·취약 사제 목표·SP·최초 등록·현재 수량 판정을 순서대로 실행한다. (2026-11-09 WB04-E181)
    update(token) {
        const g = this.#globals;
        const h = this.#hooks;
        if (!this.#field(token, 0x264) || this.#allied(this.#field(token, 0x260), this.#field(token, 0x264))) this.selectEnemy(token);
        if (!this.#field(token, 0x264)) need(h.missingEnemyAssertion, 'AI 준비 적 소유자 진단 미연결')();
        if (need(h.demo, 'AI 준비 현재 demo 조회 미연결')() !== 0 && this.#field(token, 0x260) === (g.localOwner() >>> 0)
            && (need(h.knowledgeOpen, 'AI 준비 현재 지식 창 조회 미연결')() || need(h.blockingGumpOpen, 'AI 준비 현재 차단 창 조회 미연결')())) {
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, bitsFloat(this.#field(token, 0x228)) + g.now(), true);
            this.#setField(token, 0x5a8, view.getUint32(0, true));
            this.#setField(token, 0x5ac, view.getUint32(4, true));
        }
        const ownRegion = signed(this.#player(this.#field(token, 0x260), 0x78));
        this.#setField(token, 0x294, ownRegion === g.noRegion() ? 0 : need(h.temple, 'AI 준비 현재 신전 조회 미연결')(ownRegion));
        if (this.#field(token, 0x248) & 0x100) {
            // 참여 DWORD 가 비제로인 적 사제 중 절반 HP 미만인 대상을 원본 순서로 살핀다.
            for (let enemy = 1; enemy < PLAYER_SLOTS; enemy += 1) {
                if (!this.#player(enemy, 0x2c) || this.#allied(this.#field(token, 0x260), enemy)) continue;
                const priest = need(h.priest, 'AI 준비 취약 적 사제 조회 미연결')(enemy);
                if (!priest) continue;
                const sid = this.#physicalSid(priest);
                const hitPoints = signed(read(this.#pool.slot(sid), 0x1a));
                const maxHitPoints = need(h.maxHitPoints, 'AI 준비 현재 사제 최대 HP 미연결')(sid);
                if (hitPoints >= Math.trunc(maxHitPoints / 2)) continue;
                if (this.#field(token, 0x264) === enemy) break;
                let nearest = NEAREST_DISTANCE;
                let nearestOwner = 0;
                // 같은 방향 동맹의 모든 참여 사제를 방문한다. 먼저 가까운 자신을 발견해도 중단하지 않는다.
                for (let ally = 1; ally < PLAYER_SLOTS; ally += 1) {
                    if (!this.#player(ally, 0x2c) || !this.#allied(this.#field(token, 0x260), ally)) continue;
                    const friendPriest = need(h.priest, 'AI 준비 동맹 사제 조회 미연결')(ally);
                    if (!friendPriest) continue;
                    const source = point(this.#pool.slot(this.#physicalSid(friendPriest)));
                    const target = point(this.#pool.slot(sid));
                    const distance = need(h.pointDistance, 'AI 준비 현재 사제 Point 거리 미연결')(source, target);
                    if (distance < nearest) {
                        nearest = Math.fround(distance);
                        nearestOwner = Math.fround(ally);
                    }
                }
                if (nearestOwner === signed(this.#field(token, 0x260))) {
                    this.#setField(token, 0x264, enemy);
                    break;
                }
            }
        }
        const enemyRegion = signed(this.#player(this.#field(token, 0x264), 0x78));
        this.#setField(token, 0x2a8, enemyRegion === g.noRegion() ? 0 : need(h.temple, 'AI 준비 현재 적 신전 조회 미연결')(enemyRegion));
        const ownTemple = this.#field(token, 0x294);
        const ownGraph = ownTemple ? need(h.graph, 'AI 준비 현재 신전 그래프 미연결')(ownTemple) : need(g.invalidGraph, 'AI 준비 현재 무효 그래프 조회 미연결')();
        this.#setField(token, 0x298, ownGraph);
        this.#setField(token, 0x5f8, ownGraph);
        this.#setField(token, 0x604, this.#field(token, 0x238));
        this.#setField(token, 0x5fc, this.#field(token, 0x260));
        this.#setField(token, 0x600, this.#field(token, 0x220));
        const enemyTemple = this.#field(token, 0x2a8);
        this.#setField(token, 0x2ac, enemyTemple ? need(h.graph, 'AI 준비 현재 적 신전 그래프 미연결')(enemyTemple) : need(g.invalidGraph, 'AI 준비 현재 적 무효 그래프 조회 미연결')());
        const currentEnemyRegion = signed(this.#player(this.#field(token, 0x264), 0x78));
        if (currentEnemyRegion !== g.noRegion()) {
            const bounds = need(h.regionBounds, 'AI 준비 현재 적 청크 경계 미연결')(currentEnemyRegion);
            // 하위가 돌려준 네 float 를 원래 연속 필드에 복사한다.
            for (let index = 0; index < 4; index += 1) this.#setField(token, 0x2b8 + index * 4, floatBits(bounds[index]));
            this.#setField(token, 0x2b0, this.#field(token, 0x2b8));
            this.#setField(token, 0x2b4, this.#field(token, 0x2bc));
            const left = bitsFloat(this.#field(token, 0x2b8));
            const right = bitsFloat(this.#field(token, 0x2c0));
            const x = Math.fround((right - left) * HALF + bitsFloat(this.#field(token, 0x2b0)));
            this.#setField(token, 0x2b0, floatBits(x));
            const top = bitsFloat(this.#field(token, 0x2bc));
            const bottom = bitsFloat(this.#field(token, 0x2c4));
            const y = Math.fround((bottom - top) * HALF + bitsFloat(this.#field(token, 0x2b4)));
            this.#setField(token, 0x2b4, floatBits(y));
        }
        this.#setField(token, 0x29c, need(h.priest, 'AI 준비 현재 소유 사제 조회 미연결')(this.#field(token, 0x260)));
        this.#setField(token, 0x2a0, need(h.priest, 'AI 준비 현재 적 사제 조회 미연결')(this.#field(token, 0x264)));
        if (this.#field(token, 0x260) === (g.localOwner() >>> 0)) {
            const current = g.playerKnown() ? Math.fround(need(h.storedSp, 'AI 준비 현재 SP 조회 미연결')(g.localOwner())) : 0.0;
            if (current !== signed(this.#field(token, 0x2d8))) {
                const reread = g.playerKnown() ? Math.fround(need(h.storedSp, 'AI 준비 두 번째 현재 SP 조회 미연결')(g.localOwner())) : 0.0;
                const delta = Math.fround(signed(this.#field(token, 0x2d8)) - reread);
                need(h.addLocalSp, 'AI 준비 실제 로컬 SP 가산 미연결')(delta);
            }
        }
        if (!this.#field(token, 0x25c)) {
            need(h.initialScan, 'AI 준비 초기 전체 월드 등록 미연결')(token);
            this.#setField(token, 0x25c, 1);
        }
        const attached = this.countAttachedGeysers(token);
        const collectors = attached < 1 ? (this.#field(token, 0x234) !== 0 ? 1 : 0) : this.#field(token, 0x234);
        this.#setField(token, 0x2cc, signed(collectors) <= signed(this.#field(token, 0x2e4)) ? 1 : 0);
        if (signed(this.#field(token, 0x32c)) < 2) this.#setField(token, 0x2d0, 1);
        else {
            const desired = signed(this.#field(token, 0x230));
            const minimum = desired > 0 ? desired : 1;
            this.#setField(token, 0x2d0, minimum <= this.countAttachedGeysers(token) ? 1 : 0);
        }
        if (signed(this.#field(token, 0x2d8)) > RICH_ENOUGH) this.#setField(token, 0x2d0, 1);
    }
}
