// 거주 구역(Residence) 정기 처리 0049a4e0의 현재 주민/장식/지면 테마 본문과 Fort 전용 Dude 생성 부모 00450a90을 옮긴 모듈이다(cpppj `o/RawResidenceRegular.cpp/.h`, 10.78 전용).
//  - runResidenceRegular: 사건 0 = 주민 nibble 토글 → 표면 통지 → 짝수 프레임 + nibble 지정 → 15~35초 난수 예약, 사건 1 = 두 정수 난수(X −48..+32, Y −32..+32)로 고른 점 둘레의 장식을 finder 로 조사해 처음 바뀔 장식 하나를 한 칸 바꾸고(바꾼 뒤에도 Next 한 번) 최대 조사 수를 기록,
//    사건 2 = Fort 전용 Dude 생성 부모 호출, 사건 3 = 지면 소유자 지정 + 일치 신전 원소별 테마 프레임(+6 간격) 지정. 그 밖의 사건은 payload 를 그대로 돌려준다.
//  - runResidenceDudeSpawn: Fort 에서 현재 Dude 수가 공급원 수보다 적을 때 거주 구역 입구에서 Dude 를 만들고 작업(0x5102)을 예약한다.
// 선택되지 않은 사건의 장치(난수·설정·pool 슬롯)는 읽지 않고, 선택된 위치에서 장치가 없으면 `Error` 로 거부한다. 전역(난수·설정·장식 TYPE)은 소비 위치마다 현재 값을 다시 조회한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E255: 자산 정기 처리 분배 RawAssetRegular 이식 중 cpppj 기준선 뒤 7fa2b600/ae4448e1 이 분리한 새 파일을 같은 단위에서 옮겨 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 선택된 분기가 요구하는 장치가 없으면 그 위치에서 거부하고 연결된 함수를 그대로 돌려준다. (2026-11-10 WB04-E255)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('거주 구역의 선택된 실제 본문 미연결');
    return callback;
}

// 슬롯의 위치 상수이다: 주민 nibble 이 든 WORD(+12), x/y float(+14/+18), 소유자 BYTE(+34), 프레임 DWORD(+36), 기본 프레임 DWORD(+42). (2026-11-10 WB04-E255)
const RESIDENT_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const BASE_FRAME_OFFSET = 42;

// 사건 번호와 반환 값이다: 0 주민 토글, 1 장식 조사, 2 Fort Dude 생성, 3 지면 테마. 사건 2 의 Dude 작업 코드는 0x5102 이다. (2026-11-10 WB04-E255)
const EVENT_RESIDENT = 0;
const EVENT_DECORATION = 1;
const EVENT_DUDE = 2;
const EVENT_THEME = 3;
const DUDE_TASK_CODE = 0x5102;

// 비정렬 리틀엔디언 DWORD 를 읽는다(부호 없음). (2026-11-10 WB04-E255)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 단정도 float 비트를 수로 바꾼다. 신호 NaN 페이로드는 이 모듈이 값으로 계산하지 않는다. (2026-11-10 WB04-E255)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 현재 프레임 DWORD 를 부호 있는 수로 다시 읽는다(콜백 뒤 값). (2026-11-10 WB04-E255)
function frameOf(pool, sid) {
    return readDword(pool.slot(sid), FRAME_OFFSET) | 0;
}

// signed 덧셈의 32비트 감김이다. (2026-11-10 WB04-E255)
const add = (value, delta) => (value + delta) | 0;

// x87 ftol 의 64비트 절삭 뒤 하위 DWORD(int32)이다. 비유한/±2^63 밖 입력은 CRT 범위 오류로 거부한다. (2026-11-10 WB04-E255)
function truncateLow32(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) throw new RangeError('거주 구역 좌표 CRT 물리 범위');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 현재 프레임의 짝수 바닥(0 쪽 나눗셈 뒤 ×2)에 현재 주민 nibble 을 더해 지정한다. 음수 프레임도 0 쪽 나눗셈이다. (2026-11-10 WB04-E255)
function setEven(pool, sid, hooks) {
    const frame = frameOf(pool, sid);
    need(hooks.frame)(sid, add(Math.trunc(frame / 2) * 2, pool.slot(sid)[RESIDENT_OFFSET] & 15), 0);
}

// 한 거주 구역의 정기 사건을 실행한다. pool 은 10.78 SidPool, hash 는 장식 finder 의 SquidHash, types 는 finder 발자국 표, frames 는 TYPE 별 FrameCodeTable 표,
// temples 는 현재 신전 목록(SquidPostPopList)이다. hooks = { random() → GameRandom, notify(sid), dude(sid), frame(sid, value, flags), surface(x, y) → sid, owner(sid, owner),
// element(templeSid) → int, themeMode() → 0/1, decorationType() → TYPE 번호, recordScan(count) } 이다. 반환은 다음 예약 값(float32)이다. (2026-11-10 WB04-E255)
export function runResidenceRegular(pool, hash, types, frames, temples, sid, event, count, payload, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('거주 구역49a4e0은 PATCH 본문입니다');
    if (event === EVENT_RESIDENT) {
        // 주민 nibble 이 0 이면 1, 아니면 0 으로 바꾼다(상위 nibble 보존). 이 쓰기는 slot 상태와 무관한 물리 바이트에 한다.
        const low = pool.slot(sid)[RESIDENT_OFFSET];
        pool.physical(sid)[RESIDENT_OFFSET] = (low & 0xf0) | ((low & 15) ? 0 : 1);
        need(hooks.notify)(sid);
        setEven(pool, sid, hooks);
        return Math.fround(need(hooks.random)().next(65536) * (20.0 / 65536.0) + 15.0);
    }
    if (event === EVENT_DECORATION) {
        const raw = pool.slot(sid);
        const x = floatOf(readDword(raw, X_OFFSET));
        const y = floatOf(readDword(raw, Y_OFFSET));
        // 두 원문 정수 난수 범위를 같은 시드에서 차례로 소비한다(범위 크기는 unsigned 뺄셈).
        const range = (low, high) => add(low, need(hooks.random)().next(((high >>> 0) - (low >>> 0)) >>> 0) | 0);
        let searchX = range(truncateLow32(x - 48.0), truncateLow32(x + 32.0));
        let searchY = range(truncateLow32(y - 32.0), truncateLow32(y + 32.0));
        searchX = Math.min(Math.max(searchX, 10), 246);
        searchY = Math.min(Math.max(searchY, 10), 246);
        // 아래쪽 끝은 float 로 저장한 y+32 와 246 중 작은 값이다.
        const bottom = truncateLow32(Math.min(Math.fround(y + 32.0), 246.0));
        const finder = new RawSquidFinder(pool, hash, types);
        let candidate = finder.begin({ left: searchX - 1, top: searchY - 3, right: searchX + 1, bottom });
        let changed = false;
        let checked = 0;
        // 첫 변경 뒤에도 실제 Next 를 한 번 수행하고 끝낸다.
        while (candidate !== 0 && !changed) {
            checked += 1;
            if (pool.slot(candidate)[10] === need(hooks.decorationType)()) {
                // 원문 4ad060 의 현재 TYPE/프레임을 매번 다시 조회해 플래그를 읽는다.
                const flags = () => {
                    const current = pool.slot(candidate);
                    const frame = frameOf(pool, candidate);
                    if (current[10] >= frames.length) throw new RangeError('거주 구역 현재 장식 TYPE 범위');
                    const codes = frames[current[10]].codes;
                    if (frame < 0 || frame >= codes.length) throw new RangeError('거주 구역 현재 장식 frame 범위');
                    return codes[frame].flags;
                };
                if (flags() & 8) {
                    need(hooks.frame)(candidate, add(frameOf(pool, candidate), 1), 0);
                    changed = true;
                } else if (flags() & 16) {
                    need(hooks.frame)(candidate, add(frameOf(pool, candidate), -1), 0);
                    changed = true;
                }
            }
            candidate = finder.next();
        }
        need(hooks.recordScan)(checked);
        return 1.0;
    }
    if (event === EVENT_DUDE) need(hooks.dude)(sid);
    if (event === EVENT_THEME) {
        const raw = pool.slot(sid);
        const support = need(hooks.surface)(floatOf(readDword(raw, X_OFFSET)), floatOf(readDword(raw, Y_OFFSET)));
        if (support) need(hooks.owner)(sid, pool.slot(support)[OWNER_OFFSET]);
        let theme = 0;
        const owner = pool.slot(sid)[OWNER_OFFSET];
        // 원문이 캡처한 목록/개수에서 소유자가 같은 첫 신전만 선택한다.
        for (const value of temples.items()) {
            const temple = value & 0xffff;
            if (pool.slot(temple)[OWNER_OFFSET] !== owner) continue;
            const element = need(hooks.element)(temple);
            const enabled = need(hooks.themeMode)();
            if (enabled) theme = element === 2 ? 1 : element === 0 ? 2 : element === 1 ? 3 : 0;
            break;
        }
        need(hooks.frame)(sid, add(readDword(pool.slot(sid), BASE_FRAME_OFFSET) | 0, theme * 6), 0);
        setEven(pool, sid, hooks);
        return Math.fround(0.1);
    }
    return payload;
}

// Fort 전용 Dude 생성 부모(00450a90)이다. pool 은 10.78 SidPool, fort 는 현재 Fort 모드(값 또는 getter), population/providers 는 현재 Dude 목록/공급원 목록(SquidPostPopList), source 는 거주 구역 SID,
// hooks = { entrance(source) → [x, y], spawn(x, y, owner) → sid, task(sid, code, delay), assertion() } 이다. Fort 가 아니거나 Dude 수가 공급원 수 이상이면 아무 장치도 읽지 않는다. (2026-11-10 WB04-E255)
export function runResidenceDudeSpawn(pool, fort, population, providers, source, hooks) {
    if (pool?.edition !== 'patch1078') throw new TypeError('거주 구역450a90은 PATCH 본문입니다');
    if (!(typeof fort === 'function' ? fort() : fort)) return;
    // 원문처럼 Dude 수를 먼저 읽고 공급원 수를 다음에 읽는다(둘 다 signed 비교).
    const dudeCount = population.readCount() | 0;
    const providerCount = providers.readCount() | 0;
    if (dudeCount >= providerCount) return;
    // srcSid 진단은 정상 복귀를 허용한다.
    if (!source) need(hooks.assertion)();
    if (pool.slot(source)[11] & 1) return;
    const point = need(hooks.entrance)(source);
    const born = need(hooks.spawn)(point[0], point[1], pool.slot(source)[OWNER_OFFSET]);
    if (born && !(pool.slot(born)[11] & 3)) need(hooks.task)(born, DUDE_TASK_CODE, 0);
}
