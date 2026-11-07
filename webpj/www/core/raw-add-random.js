// 대본 AddRandom 의 고정 범위 부모(0040dfd0)와 그 시각 seed(004bcab0 → 00460d70), 그리고 섬 검색(0046f380/0046f3f0)을 옮긴 모듈이다(cpppj `o/RawAddRandom.cpp/.h`, `o/RawFortTerritorySave.cpp`의 섬 검색 함수, `o/RawFortFinishOwner.cpp`의 TimeSeed, 10.78 전용).
// addRandomObjects(type, quantity, hooks): 벽시각으로 지역 난수 seed 를 만들고(글로벌 GameRandom 을 바꾸지 않음) 기본 범위 청크 1..15 를 16배한 칸 16..240 사각형을 만든 뒤, 시각 seed 효과 다음에 현재 로컬 소유자를 읽어 그 소유자의 지역 0 섬을 찾고
//   place(난수, 사각형, 섬, 수량, 타입, 0)로 기존 배치 몸체에 넘긴다. 마지막 배치 반환은 쓰지 않는다. 선택된 실제 장치(wallSeconds → local → island → place)의 누락은 그 호출 위치에서 거부하며 앞선 효과는 되돌리지 않는다.
// timeSeed(wallSeconds): double 초에 1013 을 곱해 64비트 절삭의 하위 32비트(음수는 SAR 반올림)를 `(v>>4) + v*33` 로 감은 seed 이다(RawFortFinishOwner 도 같은 함수를 쓴다).
// findFortTerritoryIsland(islands, fort, owner, region): 3204바이트 섬 기록(개수 DWORD + 32바이트 항목)에서 존재하는 항목 중 일반은 원래 owner·현재 owner 가 모두 요청 owner 와 같고, Fort 는 현재 owner 가 0 이거나 요청 owner 인 첫 signed region BYTE 일치를 돌려준다(없으면 127).
//   findAnyFortTerritoryIsland(islands, region)는 owner 조건 없이 존재하는 첫 일치를 돌려준다. Edit 의 저장 부분(SnapshotFortTerritory 등)은 웹 제외 범위이고 이 두 검색만 미션 런타임이 공유한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E30: AddRandom 부모 이식으로 추가)
import { GameRandom } from './random.js';

// 시각 seed 의 원본 double 배율과 DWORD 계수, 기본 청크 범위(00431fc0: 1..15)와 칸 배율, 섬 없음 값이다. (2026-11-08 WB04-E30)
const TIME_SCALE = 1013;
const SEED_FACTOR = 33;
const CHUNK_FIRST = 1;
const CHUNK_LAST = 15;
const CELLS_PER_CHUNK = 16;
export const NO_ISLAND = 127;

// 원본 __ftol 의 64비트 절삭 하위 DWORD(signed)이다. 비유한/범위 밖은 하위 DWORD 0 으로 처리한다. (2026-11-08 WB04-E30)
function truncate(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// SAR 의 음수 반올림과 DWORD 감김을 그대로 유지한다. 글로벌 GameRandom seed 를 변경하지 않는다. (2026-11-08 WB04-E30)
export function timeSeed(wallSeconds) {
    const value = truncate(wallSeconds * TIME_SCALE);
    return ((value >> 4) + Math.imul(value, SEED_FACTOR)) >>> 0;
}

// 선택된 실제 장치가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E30)
function need(device) {
    if (typeof device !== 'function') throw new Error('AddRandom 부모의 실제 장치 미연결');
    return device;
}

// hooks = { wallSeconds(), local(), island(소유자, 지역), place(난수, 사각형{left,top,right,bottom}, 섬, 수량, 타입, force) } 이다. (2026-11-08 WB04-E30)
export function addRandomObjects(type, quantity, hooks) {
    const random = new GameRandom(timeSeed(need(hooks.wallSeconds)()));
    const rectangle = {
        left: CHUNK_FIRST * CELLS_PER_CHUNK, top: CHUNK_FIRST * CELLS_PER_CHUNK,
        right: CHUNK_LAST * CELLS_PER_CHUNK, bottom: CHUNK_LAST * CELLS_PER_CHUNK,
    };
    const owner = need(hooks.local)();
    const region = need(hooks.island)(owner, 0);
    need(hooks.place)(random, rectangle, region, quantity, type, 0);
}

// 비정렬 필드의 지정 폭을 리틀엔디언으로 읽고 물리 범위 밖은 거부한다. (2026-11-08 WB04-E30)
function read(bytes, at, width = 4) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('Territory 현재 필드의 물리 범위');
    let value = 0;
    // 지정 폭의 바이트만 합친다.
    for (let byte = 0; byte < width; byte += 1) value += bytes[at + byte] * 2 ** (8 * byte);
    return value >>> 0;
}

// 원본 MSVC char 를 signed 로 확장한다. (2026-11-08 WB04-E30)
const signedByte = (value) => (value << 24) >> 24;

// count 는 진입 때 보관하며 최초 일치에서 반환한다. 잘못된 큰 count 는 실제 접근할 때만 거부한다. (2026-11-08 WB04-E30)
export function findFortTerritoryIsland(islands, fort, owner, region) {
    const count = read(islands, 0) | 0;
    // 현재 3204바이트의 존재/소유/지역 필드만 읽는다.
    for (let index = 0; index < count; index += 1) {
        const at = 4 + index * 32;
        if (!read(islands, at + 4)) continue;
        const current = read(islands, at + 8);
        if (fort ? (current && current !== (owner >>> 0)) : (signedByte(read(islands, at + 13, 1)) !== owner || current !== (owner >>> 0))) continue;
        if (signedByte(read(islands, at + 12, 1)) === region) return index;
    }
    return NO_ISLAND;
}

// Editor 검색은 원래/현재 owner 를 읽거나 별도의 유효성 조건을 추가하지 않는다: 존재하는 첫 일치만 반환한다. (2026-11-08 WB04-E30)
export function findAnyFortTerritoryIsland(islands, region) {
    const count = read(islands, 0) | 0;
    // 존재하는 항목의 지역 BYTE 만 비교한다.
    for (let index = 0; index < count; index += 1) {
        const at = 4 + index * 32;
        if (read(islands, at + 4) && signedByte(read(islands, at + 12, 1)) === region) return index;
    }
    return NO_ISLAND;
}
