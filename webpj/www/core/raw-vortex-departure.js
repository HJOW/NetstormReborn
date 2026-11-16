// 신전51h 수신·섬 상승 완료와 Fort 지역 조회를 같은 현재 월드에서 실행한다(cpppj o/RawVortexDeparture, 225553df, WB04-E587).
// 사용: Pool/Hash/TYPE/ChunkMap/RawIslandRuntime/RawVortexAscend와 localOwner/server/revision {value} 참조·실제 효과를 공급한다.
// receive/remove/fortTerritoryAt/updateIslandRecord는 호출 위치마다 현재 슬롯/전역을 다시 읽는다. Session 설치는 상위 바인딩의 몫이다.
import { RawSquidFinder } from './raw-squid-finder.js';

// 도달한 효과 직전에만 함수 사본을 선택한다. 누락을 정상 완료로 대신하지 않는다.
function need(fn) { if (typeof fn !== 'function') throw new Error('신전 섬 상승의 실제 장치 미연결'); return fn; }
// 지정 폭의 현재 물리 바이트만 읽는다. 짧은 패킷의 소비를 영으로 채우지 않는다.
function read(bytes, at, width = 4) { if (at < 0 || at > bytes.length || width > bytes.length - at) throw new RangeError('신전 상승의 실제 필드 범위'); let value = 0; for (let n = 0; n < width; ++n) value |= bytes[at + n] << (n * 8); return value >>> 0; }
// float 인수의 CRT _ftol 하위 DWORD를 signed로 읽는다. NaN/64비트 호스트 범위 밖은 변환 전에 거부한다.
function cell(value) { const wide = Math.fround(value); if (!(wide >= -9223372036854775808 && wide < 9223372036854775808)) throw new RangeError('신전 상승 좌표의 CRT 변환 범위'); return Number(BigInt.asIntN(32, BigInt(Math.trunc(wide)))); }
// 청크/지역의 원문 signed BYTE 해석이다.
function signedByte(value) { return (value << 24) >> 24; }

// 별도 섬/후보 목록 없이 기존 월드의 현재 물리를 참조하는 PATCH 부모다.
export class RawVortexDeparture {
    // 같은 실제 슬롯/표면·일반 Hash/현재 TYPE·청크·섬·기존 후보 몸체다.
    #pool; #hash; #types; #chunks; #islands; #ascend;
    // 효과 뒤 다시 읽는 실제 전역 참조와 값으로 소유하는 함수 묶음이다.
    #localOwner; #server; #revision; #hooks;
    // 같은 Pool의 세 몸체만 허용한다. 전역은 값 사본이 아닌 {value} 참조로 전달한다.
    constructor(pool, hash, types, chunks, islands, ascend, localOwner, server, revision, hooks = {}) {
        if (pool?.edition !== 'patch1078' || islands?.pool !== pool || ascend?.pool !== pool) throw new TypeError('신전 섬 상승의 같은 PATCH World 필요');
        this.#pool = pool; this.#hash = hash; this.#types = types; this.#chunks = chunks; this.#islands = islands; this.#ascend = ascend;
        this.#localOwner = localOwner; this.#server = server; this.#revision = revision; this.#hooks = { ...hooks };
    }
    // 후속 Session/Storage가 같은 실제 풀을 확인한다.
    get pool() { return this.#pool; }
    // 현재 TYPE를 소비 위치에서 읽는다. 첫 genus를 후속 효과의 조회로 대신하지 않는다.
    #genus(sid) { const type = this.#pool.slot(sid)[10]; if (type >= this.#types.length) throw new RangeError('신전 상승의 현재 TYPE 범위'); return this.#types[type].flags2 >>> 0; }
    // 현재 비정렬 float 좌표 둘을 값으로 읽는다.
    #position(sid) { const raw = this.#pool.slot(sid); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); return [view.getFloat32(14, true), view.getFloat32(18, true)]; }
    // _ftol 뒤 signed 나눗셈의 0방향 절삭으로 현재 청크의 signed 섬 BYTE를 읽는다.
    #islandAt(x, y) { return signedByte(this.#chunks.at(Math.trunc(cell(x) / 16), Math.trunc(cell(y) / 16)).island); }

    // float→칸 변환을 Fort 진단보다 먼저 수행한다. fort/noIsland도 같은 {value} 참조다.
    fortTerritoryAt(x, y, fort, noIsland) {
        const cx = cell(x); const cy = cell(y); const h = this.#hooks;
        if (!fort.value) need(h.assertion)('inFortMode', '\\Ns\\O\\Islandlist.cpp', 0x344);
        const surface = cx >= 0 && cx <= 255 && cy >= 0 && cy <= 255 ? this.#hash.view(0)[cy * 256 + cx] : 0;
        let island;
        if (!surface) island = noIsland.value | 0;
        else {
            const genus = this.#genus(surface);
            if (!(genus & 0x1000006)) { need(h.assertion)('genus & (gISLAND|gBRIDGE|gISLAND3x3)', '\\Ns\\O\\Squid.cpp', 0x4cf); island = noIsland.value | 0; }
            else island = genus & 4 ? noIsland.value | 0 : read(this.#pool.slot(surface), 8, 2);
        }
        if (island < 0 || island >= (this.#islands.count | 0) || !this.#islands.exists(island)) need(h.assertion)('exists(islandId)', '\\Ns\\O\\Islandlist.cpp', 0x348);
        if (island < 0 || island >= 100) throw new RangeError('Descend 지역BYTE의 실제 섬100개 물리 범위');
        return signedByte(this.#islands.bytes[4 + island * 32 + 12]);
    }

    // 첫 진단 전에 6BYTE 인수를 고정하고, y 행 우선으로 같은 섬의 기록만 덮은 뒤 생산을 갱신한다.
    updateIslandRecord(island, record) {
        if (record.length < 6) throw new RangeError('신전 상승의 실제6BYTE 기록 부족');
        const value = Uint8Array.from(record.subarray(0, 6));
        if (!this.#islands.exists(island)) need(this.#hooks.assertion)('islandList.exists(islandId)', '\\Ns\\O\\Chunkmap.cpp', 0x139);
        // 현재 활성 크기와 항목은 매번 다시 읽는다. 항목의 record 주소도 보존한다.
        for (let y = 0; y < this.#chunks.yLength; ++y) for (let x = 0; x < this.#chunks.xLength; ++x) { const entry = this.#chunks.at(x, y); if (signedByte(entry.island) === island) entry.record.set(value); }
        need(this.#hooks.refreshProduction)();
    }
    // 추가 후보는 첫 genus를 보존하되 좌표 조회 뒤 현재 섬 조회/청크의 원문 순서를 지킨다.
    #additionalCandidate(sid, island) {
        const genus = this.#genus(sid);
        if (genus & 0x1000) { const [x, y] = this.#position(sid); return this.#islandAt(x, Math.fround(y - 4)) === island; }
        if (!(genus & 0x28a300)) return false;
        const [x, y] = this.#position(sid); let found = need(this.#hooks.islandNumber)(x, y) === island;
        if (genus & 0x80000) found = this.#islandAt(x, y) === island;
        return !(genus & 0x1000000) && found;
    }
    // 표면 순회의 보관 커서는 반환 자산의 x보다1 큰 값이다. 네 이웃도 그 현재 커서로 조회한다.
    #surfaces(island, neighbors) {
        let x = 0; let y = 0; const surface = this.#hash.view(0);
        // 범위 밖 이웃은 SID0이다. 그 SID의 현재 TYPE 조회는 생략하지 않는다.
        const at = (px, py) => px < 0 || px > 255 || py < 0 || py > 255 ? 0 : surface[py * 256 + px];
        // 고정 목록 사본 없이 다음 호출의 현재 청크/표면을 다시 읽는다.
        const next = () => {
            let current = 0;
            do { if (signedByte(this.#chunks.at(Math.trunc(x / 16), Math.trunc(y / 16)).island) === island) { current = at(x, y); ++x; } else x += 16; if (x > 255) { x = 0; ++y; } } while (!current && y < 256);
            return current;
        };
        // 삭제가 다음 청크/표면을 바꾸면 다음 조회에 반영한다.
        for (let current = next(); current; current = next()) {
            const genus = this.#genus(current);
            if (neighbors) {
                if (!(genus & 2)) continue;
                // 각 삭제 후 다음 이웃의 현재 genus를 다시 읽는다.
                for (const [dx, dy] of [[-1, 0], [1, 0], [0, -1], [0, 1]]) { const candidate = at(x + dx, y + dy); if (this.#genus(candidate) & 4) need(this.#hooks.destroy)(candidate, 0x100048); }
            } else if ((genus & 0x1000002) === 2) need(this.#hooks.destroy)(current, 0x848);
        }
    }
    // 고정 신전 SID와 조건부 자기 Territory 저장을 구분해 현재 공간/자산/섬을 정리한다.
    remove(temple, island) {
        const h = this.#hooks;
        if (this.#pool.slot(temple)[11] & 1) need(h.assertion)('!vortex.isFree()', '\\Ns\\O\\Vortex.cpp', 0xea);
        need(h.state)(temple, 3); let mine = false;
        if (this.#pool.slot(temple)[34] === (this.#localOwner.value >>> 0)) {
            mine = true; const [x, y] = this.#position(temple); const record = this.#chunks.at(Math.trunc(cell(x) / 16), Math.trunc(cell(y) / 16)).record.slice();
            if (!(record[1] & 1)) need(h.assertion)('newTerr.exists', '\\Ns\\O\\Vortex.cpp', 0xf7);
            const records = new Uint8Array(120); const owner = this.#pool.slot(temple)[34]; need(h.readTerritory)(owner, records);
            if (!(record[1] & 4)) {
                record[1] = (record[1] & 0xf7) | 2;
                // 첫 빈 기록만 채우며20개 모두 찼으면 그대로 저장한다.
                for (let n = 0; n < 20; ++n) if (!(records[n * 6 + 1] & 1)) { records.set(record, n * 6); break; }
            } else {
                record[1] &= 0xfb;
                // 기존20개 기록의 home 비트만 지운다.
                for (let n = 0; n < 20; ++n) if (records[n * 6 + 1] & 4) records[n * 6 + 1] &= 0xfb;
            }
            need(h.writeTerritory)(owner, records); this.updateIslandRecord(island, record); need(h.snapshot)();
        }
        need(h.owner)(temple, 0); this.#surfaces(island, true);
        if (!this.#islands.exists(island)) need(h.assertion)('islandList.exists(islandId)', '\\Ns\\O\\Chunkmap.cpp', 0xc2);
        const bounds = this.#chunks.bounds(island); const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let current = finder.begin({ left: bounds.left * 16, top: bounds.top * 16, right: bounds.right * 16, bottom: bounds.bottom * 16 + 4 });
        // 후보 효과 전에 다음 SID를 확보해 효과가 바꾼 현재 상태를 원문 위치에서 읽는다.
        while (current) { const sid = current; current = finder.next(); if (this.#ascend.candidate(sid, island, 9999) || this.#additionalCandidate(sid, island)) need(h.destroy)(sid, 0x848); }
        this.#surfaces(island, false); this.#islands.delete(island); this.#revision.value = (this.#revision.value + 1) >>> 0;
        if (mine && !need(h.ownedIslands)(this.#localOwner.value >>> 0)) need(h.requestEdit)();
    }
    // 51h/8BYTE를 해석한다. 처음 선택한 신전 SID는 마지막 owner 조회까지 유지한다.
    receive(sender, packet) {
        const h = this.#hooks;
        if (packet.length < 8) throw new RangeError('신전51h 수신8BYTE 부족');
        const original = read(packet, 5, 2);
        if (this.#pool.slot(original)[11] & 1) need(h.assertion)('!vortex.isFree()', '\\Ns\\O\\Vortex.cpp', 0x14c);
        if (!this.#islands.exists(packet[4])) { need(h.log)(`Can't ascend island ${packet[4]}\n`); return; }
        if (packet[7]) { need(h.log)(`Removing isle ${packet[4]}\n`); this.remove(read(packet, 5, 2), packet[4]); return; }
        if (this.#server.value) need(h.sendExcept)(sender | 0, packet);
        need(h.regular)(read(packet, 5, 2)); const owner = this.#pool.slot(original)[34];
        if (owner !== (this.#localOwner.value >>> 0)) { need(h.warnAscend)(owner); need(h.sound)('enemyIslandPreparing.wav'); need(h.sound)('islandRise.wav'); return; }
        need(h.sound)('preparingAscend.wav');
    }
}
