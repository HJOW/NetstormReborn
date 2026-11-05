// 원본 10.78 공통 공간 해제 부모(004afe50, Squid::Unpop)를 옮긴 모듈이다. 같은 SID 풀 슬롯·4단계 공간 해시·spot 점유 지도를 원문 순서로 바꾼다.
// 순서: Form/contained/유효성 진단 → 이미 해제(free|void)면 끝 → void 비트 → 섬/다리 미니맵 통지 → 발자국 y/x 순서로 spot 비트 해제·건물 옆면·표면 진단
//       → contained 면 종속 해제로 끝 → 단계 0~3 의 현재 버킷 체인에서 자신을 빼기(SetNext 진단) → 미발견 진단 → 현재 표시 갱신.
// 진단(assertion)은 원문 식·파일·줄을 넘기고 정상 복귀하면 그 뒤 원문 순서로 계속한다. 진단 뒤 바뀐 현재 TYPE·좌표·checking 은 그 시점에 다시 읽는다.
// 선택된 하위 장치가 연결되지 않았으면 그때 오류이며, 이미 실행한 부분 변경(void·spot·체인)은 되돌리지 않는다.
// 근거: cpppj `o/RawSquidUnpopParent.cpp/.h`(ea848459·6ff267a1), 원본 관측 `cpppj/tests/fixtures/squid-unpop-parent-hardware-x86.tsv` 1300행. CD 10.72 경로는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-D1: 공통 공간 해제 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { SQUID_HASH_LEVELS, SQUID_HASH_SCALES, SquidHash } from './squid-hash.js';
import { INTERIOR_GENUS_MASK, effectiveGenus } from './squid-space.js';

// 슬롯 필드 위치(10.78)이다: next WORD, 타입 BYTE, 상태 BYTE, x/y float, extra BYTE. (2026-11-05 WB04-D1)
const NEXT = 4, TYPE = 10, STATE = 11, X = 14, Y = 18, EXTRA = 40;

// 원본 진단 파일 이름이다(10.78). (2026-11-05 WB04-D1)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const SQUID_H = 'c:\\ns\\o\\squid.h';

// 공통 해제의 선택 장치를 꺼낸다. 연결되지 않았으면 오류이며 앞선 부분 변경은 그대로 남는다. (2026-11-05 WB04-D1)
function need(hooks, name) {
    const hook = hooks[name];
    if (typeof hook !== 'function') throw new Error(`공통 Unpop 의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return hook;
}

// 슬롯의 float 를 읽는다(정렬되지 않은 리틀엔디언). (2026-11-05 WB04-D1)
function readFloat(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// 슬롯의 WORD 를 읽는다. (2026-11-05 WB04-D1)
function readWord(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 슬롯의 WORD 를 쓴다(이웃 바이트 보존). (2026-11-05 WB04-D1)
function writeWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 지도 좌표를 0 방향으로 절삭한다. 소비 시점에 보드 밖·NaN 을 거부한다. (2026-11-05 WB04-D1)
function cell(value) {
    if (!Number.isFinite(value) || value < 0 || value >= WORLD_CELLS) throw new RangeError('Unpop 지도 좌표 범위 오류');
    return Math.trunc(value);
}

// 원본 004afe50(10.78)을 실행한다. pool 은 SidPool, hash 는 SquidHash, spots 는 65536칸 Uint8Array(같은 점유 지도), sid 는 해제할 번호, flags 는 표시 갱신 플래그,
// checking() 은 원본 진단 DWORD(005e4794)의 현재 값을 돌려주는 함수이다. hooks:
//  - type(number) → { flags1, flags2, footX, footY } (현재 TYPE 표 재조회)
//  - miniMap(sid, genus) 섬/다리 미니맵 통지, side(x, y) 건물 옆면(RawBuildingSpace.unpopSide), contained(sid) 종속 해제 종단,
//  - update(sid, flags) 현재 표시 갱신(가상 88/8c), assertion(expression, file, line) 원문 진단(정상 복귀하면 계속).
// 사용: runSquidUnpopParent(pool, hash, spots, sid, 0, () => checking, { type, miniMap, side, contained, update, assertion }). (2026-11-05 WB04-D1)
export function runSquidUnpopParent(pool, hash, spots, sid, flags, checking, hooks) {
    if (pool?.edition !== 'patch1078' || !(hash instanceof SquidHash) || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS || typeof checking !== 'function') {
        throw new TypeError('공통 Unpop 의 10.78 풀/해시/점유 지도/진단 연결 오류');
    }
    const assert = (expression, line) => need(hooks, 'assertion')(expression, SQUID_CPP, line);
    const raw = pool.physical(sid);
    if (raw[TYPE] < 70) assert('!isForm()', 0x5d8);
    if (raw[STATE] & 8) assert('!isContained()', 0x5d9);
    if (sid === 0 || sid >= pool.capacity) assert('isValid()', 0x5e0);
    if (raw[STATE] & 5) return;
    raw[STATE] |= 4;
    const type = need(hooks, 'type');
    const initial = type(raw[TYPE]);
    const genus = initial.flags2 >>> 0;
    if ((genus & 6) && !(raw[EXTRA] & 8)) need(hooks, 'miniMap')(sid, genus);
    // 건물·자산 genus 이거나 현재 TYPE 이 표면(flags1 0x800)이고, 매몰/종속이 아니면 발자국의 spot 을 지운다.
    if (((genus & ((INTERIOR_GENUS_MASK | 0x400ff) >>> 0)) || (type(raw[TYPE]).flags1 & 0x800)) && !(raw[EXTRA] & 8) && !(raw[STATE] & 8)) {
        const right = cell(readFloat(raw, X));
        const bottom = cell(readFloat(raw, Y));
        const left = right - initial.footX + 1;
        const top = bottom - initial.footY + 1;
        if (left < 0 || top < 0 || initial.footX < 0 || initial.footY < 0 || initial.footX > WORLD_CELLS || initial.footY > WORLD_CELLS) throw new RangeError('Unpop 물리 발자국 범위 오류');
        // 원문 y 바깥·x 안쪽 순서이며, 저장한 범위를 진단 뒤 다시 만들지 않는다.
        for (let y = top; y <= bottom; y += 1) {
            // 각 칸에서 현재 TYPE·위치를 다시 읽는다.
            for (let x = left; x <= right; x += 1) {
                const current = type(raw[TYPE]);
                const effective = effectiveGenus(current.flags2, readFloat(raw, X), readFloat(raw, Y), current.footX, current.footY, x, y) & 0xff;
                if (effective) {
                    const saved = spots[y * WORLD_CELLS + x];
                    if (checking() !== 0 && (saved & effective) !== effective) assert('(flags & effectiveGenus) == ( unsigned char)effectiveGenus', 0x617);
                    spots[y * WORLD_CELLS + x] = saved & ~effective & 0xff;
                }
                if ((genus & INTERIOR_GENUS_MASK) && (x === left || x === right) && y !== top && y !== bottom) {
                    const surface = hash.cell(0, x, y);
                    if (surface !== 0 && surface < pool.capacity) need(hooks, 'side')(x, y);
                }
                const surfaceFlags = type(raw[TYPE]).flags1;
                if ((surfaceFlags & 0x800) && checking() !== 0 && hash.cell(0, x, y) !== sid) assert('getGridSid( (float)fx, (float)fy) == getSid()', 0x629);
            }
        }
    }
    if (raw[STATE] & 8) {
        need(hooks, 'contained')(sid);
        return;
    }
    let found = false;
    // 현재 좌표로 각 단계의 실제 버킷을 고르고 체인에서 자신을 뺀다.
    for (let level = 0; level < SQUID_HASH_LEVELS && !found; level += 1) {
        const index = SquidHash.bucketIndex(level, readFloat(raw, X), readFloat(raw, Y));
        const side = WORLD_CELLS / SQUID_HASH_SCALES[level];
        const bucketX = index % side;
        const bucketY = Math.floor(index / side);
        let previous = 0;
        let current = hash.cell(level, bucketX, bucketY);
        const seen = new Set();
        // 물리 범위·무한 순환만 소비 시점에 거부하며 앞선 쓰기는 남긴다.
        while (current !== 0) {
            if (seen.has(current)) throw new Error('Unpop 체인이 순환합니다.');
            seen.add(current);
            const next = readWord(pool.physical(current), NEXT);
            if (current === sid) {
                if (previous === 0) hash.setCell(level, bucketX, bucketY, next);
                else {
                    // SetNext 진단이 정상 복귀하면 저장한 next 를 그대로 쓴다.
                    if (previous === next) need(hooks, 'assertion')('n != getSid()', SQUID_H, 0x123);
                    writeWord(pool.physical(previous), NEXT, next);
                }
                found = true;
                break;
            }
            previous = current;
            current = next;
        }
    }
    if (!found) assert('found', 0x65d);
    need(hooks, 'update')(sid, flags);
}
