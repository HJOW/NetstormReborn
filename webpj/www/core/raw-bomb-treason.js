// 트리즌 주문의 대상 가상 함수 두 개(원문 41ff20 자격·41ff90 소유권/받침/이웃 전파)를 같은 raw 풀·해시에 적용하는 모듈이다(cpppj `o/RawBombTreason.cpp/.h`, 10.78 전용).
// 사용:
//   const body = new RawBombTreason(pool, hash, types, frames, hooks);
//     pool = SidPool(10.78), hash = SquidHash(같은 네 단계 공간 해시), types = 188칸 타입 표(`flags2`·`name`·`footX/footY`), frames = 188칸 프레임 코드 표({ codes: [{ side, variant, number, flags }] }),
//     hooks = { role(역할) → 현재 DWORD(5412cc/5411a8/541278/5411a0/5412c4/541200), owner(sid, 가상표, owner BYTE), refresh(sid, 가상표), fence(sid, 재생성 여부),
//               find(x, y, 타입) → sid, support(x, y, owner BYTE), assertion(식, 파일, 줄), trace(수준, 문구), flare(sid, 1, 3, 플레어 타입, 3) }.
//   body.mayTarget(parent, target) → 소유자가 같으면 거부, 빈 섬 바닥('H' 방향)·고갈 간헐천·genus 0x10170004 가 아니면 거부, 사제 genus(0x200000)는 거부(다른 추가 관문 없음).
//   body.apply(parent, target) → 운반체 대상이면 끝, 수준 4 문구("Set 이름 at (x,y) from 이전 to 새 소유자")를 남기고 펜스는 정리→소유권 전파→재생성, genus 0x10040000 대상은 바로 아래 빈 섬 바닥까지,
//     섬/간헐천/genus 0x10000000 대상은 받침 소유권 전파와 칸 위 후보 갱신, 다리는 북·동·남·서 이웃 칸의 연결 객체까지 전파한 뒤 마지막에 플레어를 낸다.
// 역할 값은 호출 위치마다 현재 DWORD 로 다시 읽으며, 선택한 효과 누락은 앞선 부수 효과 뒤 명시 오류이다. 원문 진단은 정상 복귀하면 같은 위치에서 계속한다.
// 원본 관측: cpppj `bomb-treason-x86.tsv` 157행(자격 E·적용 A: 소유자/종류/방향/QA/바닥 발견/재진입 변경/후보 순회의 효과 순서·소유자 결과)을 tests/raw-bomb-treason.test.mjs 가 대조한다.
// 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E552: 트리즌 주문 대상 본문 RawBombTreason 이식)
import { RawSquidFinder } from './raw-squid-finder.js';
import { pathSide } from './raw-path-animation.js';

// 트리즌이 읽는 여섯 현재 전역 역할이다(번호는 hooks.role 의 인자). (2026-11-15 WB04-E552)
export const BOMB_TREASON_ROLE = Object.freeze({ NoIsland: 0, EmptyGeyser: 1, Fence: 2, Bridge: 3, Connector: 4, Flare: 5 });

// 슬롯의 가상 표(+0)·TYPE(+10)·운반체 WORD(+12)·좌표 float(+14/+18)·소유자(+34)·프레임(+36) 위치이다. (2026-11-15 WB04-E552)
const TABLE_OFFSET = 0;
const TYPE_OFFSET = 10;
const CARRIER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
// 자격/전파가 보는 genus 마스크와 빈 섬 바닥의 방향 글자('H')이다. (2026-11-15 WB04-E552)
const ELIGIBLE_GENUS = 0x10170004;
const PRIEST_GENUS = 0x200000;
const BENEATH_GENUS = 0x10040000;
const SUPPORT_GENUS = 0x10000000;
const FLOOR_SIDE = 0x48;
// 북·동·남·서(0, 2, 4, 6 방향)의 이웃 칸 오프셋이다(원본 ROM 의 짝수 방향). (2026-11-15 WB04-E552)
const NEIGHBOR_CELLS = Object.freeze([[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]]);
// 원문 진단이 가리키는 Bomb.cpp 의 파일 이름과 두 줄 번호이다. (2026-11-15 WB04-E552)
const BOMB_CPP = '\\Ns\\O\\Bomb.cpp';
const BENEATH_ASSERT_LINE = 0x5e9;
const OWN_ASSERT_LINE = 0x5f0;

// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다. (2026-11-15 WB04-E552)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('트리즌 실제 하위 미연결');
    return hook;
}

// 원문 _ftol 의 signed64 절삭 뒤 하위 DWORD(signed)이며 범위 밖/비유한은 0 이다. (2026-11-15 WB04-E552)
function cell(value) {
    const limit = 9223372036854775808;
    if (!(value >= -limit && value < limit)) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 트리즌 대상 본문이다. 모든 참조는 이 본문보다 오래 살아야 하며 독립 세계/소유자/검색 결과 사본을 만들지 않는다. (2026-11-15 WB04-E552)
export class RawBombTreason {
    // 같은 PATCH 풀/해시·현재 188칸 TYPE/프레임 표와 외부 효과 장치를 빌린다.
    constructor(pool, hash, types, frames, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || frames.length !== 188) throw new TypeError('트리즌 같은 PATCH/188TYPE 계약 오류');
        this.pool = pool;
        this.hash = hash;
        this.types = types;
        this.frames = frames;
        this.hooks = hooks;
    }

    // 원문 _ftol 의 signed64 절삭 뒤 하위 DWORD 이다(비유한·범위 밖 0). 직접 값 검사용으로 공개한다. (2026-11-15 WB04-E552)
    static cell(value) { return cell(value); }

    // 현재 물리 슬롯의 지정 폭을 낮은 바이트부터 조합한다(이웃 필드/상태를 추가로 읽지 않음).
    read(sid, offset, width = 4) {
        const raw = this.pool.slot(sid);
        let value = 0;
        for (let byte = 0; byte < width; byte += 1) value += raw[offset + byte] * 2 ** (8 * byte);
        return value >>> 0;
    }

    // 효과마다 다시 읽은 TYPE 의 실제 행이다.
    type(sid) {
        const type = this.read(sid, TYPE_OFFSET, 1);
        if (type >= this.types.length) throw new RangeError('트리즌 현재TYPE 범위 오류');
        return this.types[type];
    }

    // 현재 BYTE 종류와 현재 전체 DWORD 역할을 비교한다(역할 DWORD 를 미리 줄이거나 캐시하지 않음).
    is(sid, role) { return this.read(sid, TYPE_OFFSET, 1) === need(this.hooks.role)(role); }

    // 현재 TYPE/프레임에서 원문 signed 방향 BYTE 를 읽는다(004ad680).
    side(sid) {
        const type = this.read(sid, TYPE_OFFSET, 1);
        if (type >= this.frames.length) throw new RangeError('트리즌 현재 방향TYPE 범위 오류');
        return pathSide(this.frames[type], this.read(sid, FRAME_OFFSET) | 0);
    }

    // 원문 Pop 좌표를 스냅 없이 두 float 로 읽는다.
    position(sid) {
        const raw = this.pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return [view.getFloat32(X_OFFSET, true), view.getFloat32(Y_OFFSET, true)];
    }

    // 41ff20: 소유자가 같으면 거부하고, 그 뒤에만 종류/방향/genus 를 조회한다.
    mayTarget(parent, target) {
        if (this.read(target, OWNER_OFFSET, 1) === this.read(parent, OWNER_OFFSET, 1)) return false;
        if (!(this.is(target, BOMB_TREASON_ROLE.NoIsland) && this.side(target) === FLOOR_SIDE) && !this.is(target, BOMB_TREASON_ROLE.EmptyGeyser) && !(this.type(target).flags2 & ELIGIBLE_GENUS)) return false;
        return !(this.type(target).flags2 & PRIEST_GENUS);
    }

    // 소유자 호출이 바꾼 가상 표를 표시 호출 직전에 다시 읽는다.
    set(parent, target) {
        const table = this.read(target, TABLE_OFFSET);
        const owner = this.read(parent, OWNER_OFFSET, 1) & 0xff;
        need(this.hooks.owner)(target, table, owner);
        need(this.hooks.refresh)(target, this.read(target, TABLE_OFFSET));
    }

    // 일반 지점 finder 는 flags 0 과 보정 없는 절삭을 쓴다: 후보 next 는 외부 효과 전에 저장한다.
    at(parent, x, y, connectors) {
        const cx = cell(x);
        const cy = cell(y);
        const finder = new RawSquidFinder(this.pool, this.hash, this.types);
        // 선저장 next 와 후보의 현재 자료를 그대로 순회한다.
        for (let candidate = finder.begin({ left: cx, top: cy, right: cx, bottom: cy }, 0); candidate; candidate = finder.next()) {
            if (connectors ? this.is(candidate, BOMB_TREASON_ROLE.Connector) : (this.type(candidate).flags2 & BENEATH_GENUS) !== 0) this.set(parent, candidate);
        }
    }

    // 41ff90: 효과 진입은 자격을 반복하거나 현재 권한을 추가 검사하지 않는다.
    apply(parent, target) {
        const h = this.hooks;
        if (target === this.read(parent, CARRIER_OFFSET, 2)) return;
        const position = this.position(target);
        const oldOwner = this.read(target, OWNER_OFFSET, 1);
        const owner = this.read(parent, OWNER_OFFSET, 1);
        need(h.trace)(4, `Set ${this.type(target).name} at (${cell(position[0])},${cell(position[1])}) from ${oldOwner} to ${owner}\n`);
        if (this.is(target, BOMB_TREASON_ROLE.Fence)) need(h.fence)(target, false);
        this.set(parent, target);
        if (this.is(target, BOMB_TREASON_ROLE.Fence)) need(h.fence)(target, true);
        if (this.type(target).flags2 & BENEATH_GENUS) {
            const point = this.position(target);
            const beneath = need(h.find)(point[0], point[1], need(h.role)(BOMB_TREASON_ROLE.NoIsland));
            if (beneath) {
                need(h.trace)(4, 'A - Also setting noIsland under it.');
                if (this.side(beneath) !== FLOOR_SIDE) need(h.assertion)("anIsle.getOrientation() == 'H'", BOMB_CPP, BENEATH_ASSERT_LINE);
                const at = this.position(beneath);
                need(h.support)(at[0], at[1], this.read(parent, OWNER_OFFSET, 1) & 0xff);
            }
        }
        if (this.is(target, BOMB_TREASON_ROLE.NoIsland) || this.is(target, BOMB_TREASON_ROLE.EmptyGeyser) || (this.type(target).flags2 & SUPPORT_GENUS)) {
            if (this.is(target, BOMB_TREASON_ROLE.NoIsland) && this.side(target) !== FLOOR_SIDE) need(h.assertion)("s.getOrientation() == 'H'", BOMB_CPP, OWN_ASSERT_LINE);
            need(h.trace)(4, 'B - Also setting noIsland under it.');
            const point = this.position(target);
            need(h.support)(point[0], point[1], this.read(parent, OWNER_OFFSET, 1) & 0xff);
            const current = this.position(target);
            this.at(parent, current[0], current[1], false);
        }
        if (this.is(target, BOMB_TREASON_ROLE.Bridge)) {
            // 원본 ROM 의 북/동/남/서 순서이며 매 방향마다 현재 대상 좌표를 다시 읽는다.
            for (let direction = 0; direction < 8; direction += 2) {
                const point = this.position(target);
                const [dx, dy] = NEIGHBOR_CELLS[direction];
                this.at(parent, Math.fround(cell(point[0] + dx)), Math.fround(cell(point[1] + dy)), true);
            }
        }
        need(h.flare)(target, 1, 3, need(h.role)(BOMB_TREASON_ROLE.Flare), 3);
    }
}
