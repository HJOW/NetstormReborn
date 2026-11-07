// 원본 10.78 공통 Pop 부모(004b02d0)를 옮긴 모듈이다. 같은 SID 풀 슬롯·4단계 공간 해시·spot 점유 지도에 객체를 등록하는 순서를 원문 그대로 따른다.
// 순서: 무효/비 void/contained 면 Unchanged → 유한·범위 안 좌표 검사(아니면 (10,10) 보정, metamap 이 꺼져 있으면 로그) → x/y float 와 화면 좌표(×16+0.5, ×11+0.5) 저장 →
//       섬/다리(genus&6)이고 매몰이 아니면 미니맵 통지 → 건물군 genus 또는 표면(flags1 0x800)이고 매몰/종속이 아니면 발자국(y 바깥·x 안쪽)의 spot 비트를 effective genus 로
//       OR(겹치면 Overlap 으로 조기 반환하되 앞선 칸·좌표는 되돌리지 않음), 건물 옆면(양 옆 모서리 제외)·표면 grid 교체 진단/로그 → 해시 단계 선택(섬/다리 0, 그 밖은 SHP 프레임 크기)
//       → 단계 버킷 머리에 삽입(자기 머리 진단 0x123) → 단계 0 이면 섬 번호 갱신(genus 진단 0x4b9, 다리는 기존 WORD 유지) → 현재 표시 갱신 → 첫 Pop 이면 firstPop 비트
//       → 현재 가상 Activate → 마지막 contained 진단(0x71a) → Registered.
// 진단은 정상 복귀하면 그 위치 다음에서 계속하며, 진단 뒤에는 현재 TYPE 표·좌표·checking 을 다시 읽는다. 선택된 하위 장치가 없으면 그 위치에서 오류이고 앞선 쓰기는 남는다.
// 근거: cpppj `o/RawSquidPopParent.cpp/.h`(기준선 1f00a25a). 10.78 전용이다. 로그 서식의 %5.1f 는 C 의 정확한 반올림(동률은 짝수)을 따른다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E1: 공통 Pop 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { SquidHash } from './squid-hash.js';
import { INTERIOR_GENUS_MASK, effectiveGenus } from './squid-space.js';

// 공개 결과이다: 변경 없음, 등록, spot 겹침으로 조기 반환(좌표·앞선 칸은 남음). (2026-11-08 WB04-E1)
export const RAW_POP_RESULT = Object.freeze({ unchanged: 0, registered: 1, overlap: 2 });

// 원문 진단의 소스 경로와 헤더 경로이다. (2026-11-08 WB04-E1)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const SQUID_H = 'c:\\ns\\o\\squid.h';
// 슬롯 필드 위치: next·섬 번호·TYPE·상태·x·y·화면 x/y·레벨·extra. (2026-11-08 WB04-E1)
const NEXT = 4, ISLAND = 8, TYPE = 10, STATE = 11, X = 14, Y = 18, SCREEN_X = 22, SCREEN_Y = 24, LEVEL = 33, EXTRA = 40;
// 무효 섬 번호(PE 0050c038 의 invalidIslandId)이다. 타입 번호와 다르다. (2026-11-08 WB04-E1)
const INVALID_ISLAND = 127;
// 건물군 genus(PE 005424b8)와 spot 을 쓰는 genus 마스크, 표면 flags1 비트이다. (2026-11-08 WB04-E1)
const BUILDING_MASK = INTERIOR_GENUS_MASK, SPOT_MASK = (INTERIOR_GENUS_MASK | 0x400ff) >>> 0, SURFACE_FLAG = 0x800;

// 선택된 하위 장치를 꺼낸다. 없으면 그 위치에서 오류이다. (2026-11-08 WB04-E1)
function need(hooks, name) {
    const device = hooks[name];
    if (typeof device !== 'function') throw new Error(`공통 Pop의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 슬롯의 float32 를 읽는다. (2026-11-08 WB04-E1)
function readFloat(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// 슬롯의 float32 를 쓴다(정확한 비트). (2026-11-08 WB04-E1)
function writeFloat(raw, offset, value) {
    new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setFloat32(offset, value, true);
}

// 정확한 원문 저장 폭(1·2바이트)만 쓰며 이웃 payload 를 유지한다. (2026-11-08 WB04-E1)
function put(raw, offset, value, width) {
    // 낮은 바이트부터 저장한다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 원문 유한 입력의 ftol 이다. 범위 밖 실제 메모리 접근은 호스트 오류로 남긴다. (2026-11-08 WB04-E1)
function toInteger(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Pop 원문 정수 좌표 범위');
    return Math.trunc(value);
}

// imul/sub/inc 의 하위 DWORD 감김을 보존한다(음수/0 발자국을 임의 양수로 보정하지 않음). (2026-11-08 WB04-E1)
function startOf(end, footprint) {
    return (end - footprint + 1) | 0;
}

// TYPE 이름의 첫 NUL 까지만 읽는다(번역된 설명을 쓰지 않음). (2026-11-08 WB04-E1)
function nameOf(record) {
    const text = String(record.name ?? '');
    const nul = text.indexOf('\0');
    return nul < 0 ? text : text.slice(0, nul);
}

// C 의 %<width>.1f 서식이다: 정확한 이진값의 십진 전개에서 첫째 소수 자리로 반올림(동률은 짝수)하고 폭만큼 앞을 공백으로 채운다. NaN/무한대는 소문자 nan/inf 이다. (2026-11-08 WB04-E1)
function formatFixed1(value, width) {
    let text;
    if (Number.isNaN(value)) text = 'nan';
    else if (!Number.isFinite(value)) text = value < 0 ? '-inf' : 'inf';
    else {
        const negative = value < 0 || Object.is(value, -0);
        const digits = Math.abs(value).toFixed(100);
        const dot = digits.indexOf('.');
        const whole = digits.slice(0, dot);
        const rest = digits.slice(dot + 1);
        let tenths = BigInt(whole + rest[0]);
        const tail = rest.slice(1);
        const half = tail[0] > '5' || (tail[0] === '5' && /[1-9]/.test(tail.slice(1)));
        const tie = tail[0] === '5' && !/[1-9]/.test(tail.slice(1));
        if (half || (tie && tenths % 2n === 1n)) tenths += 1n;
        const magnitude = tenths.toString().padStart(2, '0');
        text = `${negative ? '-' : ''}${magnitude.slice(0, -1)}.${magnitude.slice(-1)}`;
    }
    return text.padStart(width, ' ');
}

// 좌표가 현재 월드 안인지 본다: x>0, x<한 변, y>0, y<한 변(NaN 은 실패). 한 변은 현재 getter(없으면 256)이다. (2026-11-08 WB04-E1)
export function isCurrentPopPointValid(x, y, worldCells) {
    const fx = Math.fround(x);
    const fy = Math.fround(y);
    if (!(fx > 0)) return false;
    const limit = Math.fround(typeof worldCells === 'function' ? worldCells() : WORLD_CELLS);
    return fx < limit && fy > 0 && fy < limit;
}

// 004b02d0 전체 부모를 실행한다. pool 은 SidPool, hash 는 SquidHash, spots 는 65536칸 Uint8Array, checking() 은 원문 진단 DWORD(005e4794)의 현재 값이다.
// hooks: { type(number) → 현재 TYPE 레코드, frameSize(sid) → [폭, 높이], worldCells() → 한 변, metamap() → boolean, trace(level, text),
//          assertion(expression, file, line), miniMap(sid, genus), side(cx, cy, buildingX, buildingY), update(sid, flags), firstPop(sid) → flags, activate(sid, flags) }.
// 반환: RAW_POP_RESULT. 사용: runSquidPopParent(pool, hash, spots, sid, 20.5, 31.25, 0, () => 0, hooks). (2026-11-08 WB04-E1)
export function runSquidPopParent(pool, hash, spots, sid, x, y, flags, checking, hooks) {
    if (pool?.edition !== 'patch1078' || !(hash instanceof SquidHash) || !(spots instanceof Uint8Array) || spots.length !== WORLD_CELLS * WORLD_CELLS || typeof checking !== 'function') {
        throw new TypeError('현재 Pop 부모의 PATCH/전체 spot 필요');
    }
    let currentFlags = flags >>> 0;
    let px = Math.fround(x);
    let py = Math.fround(y);
    if (!sid || sid >= pool.capacity) return RAW_POP_RESULT.unchanged;
    const raw = pool.physical(sid);
    if (!(raw[STATE] & 4) || (raw[STATE] & 8)) return RAW_POP_RESULT.unchanged;
    const assertion = (expression, line, file = SQUID_CPP) => need(hooks, 'assertion')(expression, file, line);
    if (!isCurrentPopPointValid(px, py, hooks.worldCells)) {
        if (!need(hooks, 'metamap')()) {
            const name = nameOf(need(hooks, 'type')(raw[TYPE]));
            need(hooks, 'trace')(29, `Error ${name} popped to( ${formatFixed1(px, 5)},${formatFixed1(py, 5)}). Set to( 10,10)\n`);
        }
        px = 10;
        py = 10;
    }
    writeFloat(raw, X, px);
    writeFloat(raw, Y, py);
    put(raw, SCREEN_X, Math.trunc(px * 16 + 0.5), 2);
    put(raw, SCREEN_Y, Math.trunc(py * 11 + 0.5), 2);
    // 좌표 저장 뒤 TYPE 레코드와 그 genus 만 보관한다.
    const initial = need(hooks, 'type')(raw[TYPE]);
    const genus = initial.flags2 >>> 0;
    if ((genus & 6) && !(raw[EXTRA] & 8)) need(hooks, 'miniMap')(sid, genus);
    if (((genus & SPOT_MASK) || (need(hooks, 'type')(raw[TYPE]).flags1 & SURFACE_FLAG)) && !(raw[EXTRA] & 8) && !(raw[STATE] & 8)) {
        // 외부 미니맵이 raw 좌표를 바꿔도 원문 인자 좌표 범위는 유지한다.
        const right = toInteger(px);
        const bottom = toInteger(py);
        const left = startOf(right, initial.footX);
        const top = startOf(bottom, initial.footY);
        if (!isCurrentPopPointValid(left, top, hooks.worldCells)) assertion('start.isValid()', 0x6b7);
        // 진단 복귀 뒤에도 처음 저장한 행 범위를 유지한다.
        for (let cy = top; cy <= bottom; cy += 1) {
            // 매 칸의 effective genus 와 표면 여부는 현재 TYPE/좌표를 다시 읽는다.
            for (let cx = left; cx <= right; cx += 1) {
                if (cx < 0 || cy < 0 || cx >= WORLD_CELLS || cy >= WORLD_CELLS) throw new RangeError('Pop spot의 물리 지도 범위');
                const current = need(hooks, 'type')(raw[TYPE]);
                const effective = effectiveGenus(current.flags2, readFloat(raw, X), readFloat(raw, Y), current.footX, current.footY, cx, cy) & 0xff;
                if (effective) {
                    const saved = spots[cy * WORLD_CELLS + cx];
                    if (saved & effective) return RAW_POP_RESULT.overlap;
                    spots[cy * WORLD_CELLS + cx] = saved | effective;
                }
                if ((genus & BUILDING_MASK) && (cx === left || cx === right) && cy !== top && cy !== bottom) {
                    const surface = hash.cell(0, cx, cy);
                    if (surface && surface < pool.capacity) need(hooks, 'side')(cx, cy, readFloat(raw, X), readFloat(raw, Y));
                }
                if (need(hooks, 'type')(raw[TYPE]).flags1 & SURFACE_FLAG) {
                    if (hash.cell(0, cx, cy)) {
                        // 두 grid 조회 뒤 저장한 후보는 진단이 지도를 바꿔도 유지한다.
                        const replaced = hash.cell(0, cx, cy);
                        if (checking()) assertion('false', 0x6e8);
                        const name = nameOf(need(hooks, 'type')(raw[TYPE]));
                        const oldName = nameOf(need(hooks, 'type')(pool.physical(replaced)[TYPE]));
                        need(hooks, 'trace')(29, `ERROR: gridSid ${sid}( ${name}) replacing ${replaced}( ${oldName})\n`);
                    }
                }
            }
        }
    }
    const levelGenus = need(hooks, 'type')(raw[TYPE]).flags2 >>> 0;
    let level = 0;
    if (!(levelGenus & 6)) {
        const size = need(hooks, 'frameSize')(sid);
        level = SquidHash.objectLevel(levelGenus, size[0], size[1]);
    }
    raw[LEVEL] = level;
    // 선택한 버킷 좌표와 저장한 next 를 진단 뒤에도 사용한다.
    const bucketX = readFloat(raw, X);
    const bucketY = readFloat(raw, Y);
    const next = hash.bucket(level, bucketX, bucketY);
    if (next === sid) assertion('n != getSid()', 0x123, SQUID_H);
    put(raw, NEXT, next, 2);
    hash.setBucket(level, bucketX, bucketY, sid);
    if (!raw[LEVEL]) {
        // 004acd20 은 현재 genus 를 한 번 보관한다.
        const islandGenus = need(hooks, 'type')(raw[TYPE]).flags2 >>> 0;
        if (!(islandGenus & 0x1000006)) assertion('genus & (gISLAND|gBRIDGE|gISLAND3x3)', 0x4b9);
        else if (!(islandGenus & 4)) put(raw, ISLAND, INVALID_ISLAND, 2);
    }
    need(hooks, 'update')(sid, currentFlags);
    if (!(raw[EXTRA] & 0x80)) {
        currentFlags = (currentFlags | need(hooks, 'firstPop')(sid)) >>> 0;
        raw[EXTRA] |= 0x80;
    }
    need(hooks, 'activate')(sid, currentFlags);
    if (raw[STATE] & 8) assertion('!isContained()', 0x71a);
    return RAW_POP_RESULT.registered;
}
