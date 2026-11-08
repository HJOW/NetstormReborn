// 건설 배치 통지(메시지 0x3f, 004441b0)의 정리·배치·사제 처리 순서를 옮긴 모듈이다(cpppj `o/RawConstructionNotice.cpp/.h`, 10.78 004441b0).
// 서버가 확정해 보낸(또는 직접 처리하는) 건설 배치를 받아, 놓을 칸의 로컬 예측 조각을 지우고 조각을 놓은 뒤 건설 사제를 움직이거나 건설을 시작한다.
//  - handle(notice): 값형 통지(type·x·y·argument·direction·player·flags·quality·timeLow·timeHigh·sids[1..19])를 처리한다. 타입 번호가 표 밖이거나 SID 가 비었거나 19개를 넘거나
//    0·풀 밖이면 정리나 이력 변경 전에 거부한다. 순서: clear → (타입 flags1 의 HP 보유 비트 또는 Dais 타입이고 notice.flags 비트0 이 없으면 abstract)·(flags1 의 건설 사제 필요 비트 0x8000 이고
//    flags 비트0 이 없으면 needsBuilder) → group 이 10 이 아니고 notice.player 가 로컬 플레이어이면 최근 건설 위치 이력을 뒤에서 밀어 첫 칸에 새 좌표를 넣고 카운터를 0 으로 → place(abstract 0/1) →
//    abstract 이면: needsBuilder 는 로컬 플레이어가 아니거나 moveLocalBuilder 일 때만 moveBuilder(player, 첫 SID, 시각 하위, 시각 상위), 아니고 권한(authority)이 있으면
//    첫 객체 genus 가 사제 이동 대상(0x404200)이면 [소유 사제 조회 → contained 타입 조회 → 두 번째 소유 사제 조회 → 그 사제의 해당 contained 삭제 → 사제 상태 low2=0·genus 사제·이동 가능이면
//    접근점(플래그 7, 실패하면 3)으로 이동 시도, 못 움직이면 첫 객체의 현재 TYPE·소유자로 환불하고 그 객체를 삭제] 아니면 start(첫 SID, 0) → 마지막에 refresh 를 한 번 부른다.
//  - actualHandle(packet): 4441b0 의 실제 BYTE 경로이다. 전체 typed 변환/19개 선검사 없이 원문 소비 위치마다 현재 패킷(ConstructionPacketView)을 읽는다. 진입 때 TYPE·방향·인자를 고정하고
//    그 TYPE 의 레코드를 Clear 뒤에도 같이 읽으며, 분기 선택은 전역(공유 getter)을 사용 위치마다 다시 읽는다. 배치는 actualPlace(요청, 선언 개수, 현재 SID 읽기 함수)로 넘긴다.
//    Builder 이동은 시각/첫 SID/현재 소유자를 선택된 분기에서만 읽고, 사제 분기의 실패 환불은 PATCH 소유자 위치를 쓴다.
//  - makeConstructionNoticeHooks(pool, clear, place, hooks, priest): 정리·배치(typed/실제 wire)·환불·이동 불가 조회를 같은 풀의 실제 모듈로 잇는다. makeConstructionNoticeConfirmHooks(pool, notice, hooks):
//    서버 확정의 직접 처리 경계를 같은 풀의 통지 처리기로 잇는다. 풀이 다르면 거부한다.
// 10.72(CD) 경로(소유자 +32, contained 없음)와 10.37 은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E134: 건설 배치 통지 이식으로 추가)
import { ConstructionPacketView } from './raw-construction-packet.js';

// raw 타입(+10)·상태(+11)·좌표(+14/+18)·PATCH 소유자(+34) 위치이다. (2026-11-08 WB04-E134)
const TYPE = 10;
const STATE = 11;
const X_AT = 14;
const Y_AT = 18;
const PATCH_OWNER = 34;
// HP 보유(0x10)·건설 사제 필요(0x8000) 플래그, 소유 사제 이동 대상(0x404200)·사제(0x200000) genus 비트이다. (2026-11-08 WB04-E134)
const HAS_HP = 0x10;
const NEEDS_BUILDER = 0x8000;
const PRIEST_TARGETS = 0x404200;
const PRIEST = 0x200000;
// 이력에서 제외하는 group 번호, 통지 SID 상한, 이력 칸 수, 타입 표 크기이다. (2026-11-08 WB04-E134)
const NO_HISTORY_GROUP = 10;
const MAX_PIECES = 19;
const TYPE_COUNT = 188;
// 접근점의 첫 시도와 재시도 플래그이다. (2026-11-08 WB04-E134)
const FIRST_APPROACH = 7;
const RETRY_APPROACH = 3;
// 좌표 거의 올림에서 더하는 단정도 값이다(0041d7a0, 0.9999899864196777f). (2026-11-08 WB04-E134)
const ROUND_UP_BIAS = Math.fround(0.9999899864196777);
// 통지 처리기가 쓰는 필수 하위 효과 이름이다(생성 때 모두 확인한다). actualPlace 는 실제 wire 경로에서만 필요하다. (2026-11-08 WB04-E134)
const HOOK_NAMES = Object.freeze(['clear', 'place', 'moveBuilder', 'findPriest', 'immobile', 'approach', 'tryMove', 'refund', 'destroy', 'start', 'refresh',
    'containedType', 'removeContained']);

// 통지 처리기가 읽는 전역이다. 주소는 PATCH 기준이다. 값형 필드(독립 입력)와 공유 getter(실제 Session 이 같은 전역을 빌려줌)를 함께 두고,
// 실제 wire 경로는 공유 getter 를 쓰는 접근자를 소비 위치마다 부른다. 값형 typed 경로는 값형 필드를 그대로 읽는다.
// 사용: 월드마다 하나를 두고 RawConstructionNotice 에 넘긴다. 처리기보다 오래 살아야 한다. (2026-11-08 WB04-E134)
export class ConstructionNoticeState {
    // 00540c70: 통지의 플레이어 바이트와 DWORD 로 비교하는 로컬 플레이어이다.
    localPlayer = 0;
    // 00540bc4: 권한 쪽에서만 자동 건설 시작 또는 소유 사제 이동을 처리한다.
    authority = true;
    // 00544d10: 켜져 있으면 로컬 플레이어의 건설 사제도 다시 이동시킨다.
    moveLocalBuilder = false;
    // 005412ec: HP 플래그가 없어도 abstract 로 놓는 Dais 타입이다.
    daisType = 165;
    // 005412d0: 사제의 contained 타입 조회에 넘기는 사제 타입이다.
    priestType = 158;
    // 00558ea4: 가장 최근 배치부터 다섯 좌표(독립 값형 입력)이다.
    history = Array.from({ length: 5 }, () => ({ x: 0, y: 0 }));
    // 00558e78: 로컬 이력이 갱신되면 0 으로 쓰는 순번(독립 값형 입력)이다.
    historyCounter = 0;
    // 실제 배치/정리와 같은 Client boss 를 빌리는 함수(없으면 null)이다.
    sharedAuthority = null;
    // 실제 Session 이 Client 수명의 같은 이력 {points, index} 를 빌려준다(없으면 null).
    sharedHistory = null;
    // 실제 wire 경로가 직접 읽는 로컬 플레이어/Dais 타입/사제 타입을 빌리는 함수(없으면 null)이다.
    sharedLocalPlayer = null;
    sharedDaisType = null;
    sharedPriestType = null;

    // 이력 다섯 좌표의 현재 장부(공유가 있으면 공유, 아니면 독립 값형)이다. (2026-11-08 WB04-E134)
    historyPoints() { return this.sharedHistory ? this.sharedHistory.points : this.history; }
    // 이력 순번의 현재 값이다. (2026-11-08 WB04-E134)
    historyCounterNow() { return this.sharedHistory ? this.sharedHistory.index : this.historyCounter; }
    // 이력 순번의 현재 장부에 쓴다(Tab 과 통지가 같은 순번을 쓴다). (2026-11-08 WB04-E134)
    setHistoryCounter(value) {
        if (this.sharedHistory) this.sharedHistory.index = value >>> 0;
        else this.historyCounter = value >>> 0;
    }
    // 배치 효과 뒤 자동 건설/사제 이동 관문에서 현재 권한을 읽는다. (2026-11-08 WB04-E134)
    authorityNow() { return this.sharedAuthority ? Boolean(this.sharedAuthority()) : this.authority; }
    // 실제 wire 경로의 현재 로컬 플레이어이다. 독립 입력은 값형 필드를 유지한다. (2026-11-08 WB04-E134)
    localPlayerNow() { return this.sharedLocalPlayer ? this.sharedLocalPlayer() >>> 0 : this.localPlayer; }
    // 실제 wire 경로의 현재 Dais 타입 번호이다. (2026-11-08 WB04-E134)
    daisTypeNow() { return this.sharedDaisType ? this.sharedDaisType() >>> 0 : this.daisType; }
    // 첫 사제 선택 뒤 contained 타입 조회에 넘기는 현재 사제 타입 번호이다. (2026-11-08 WB04-E134)
    priestTypeNow() { return this.sharedPriestType ? this.sharedPriestType() >>> 0 : this.priestType; }
}

// 비정렬 단정도 float 를 읽는다. (2026-11-08 WB04-E134)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 0041d7a0: 편향을 더해 __ftol 의 하위 signed DWORD 로 절삭하고 단정도로 저장한다. 비유한/64비트 범위 밖은 0 이다. (2026-11-08 WB04-E134)
function snap(value) {
    const sum = Math.fround(value) + ROUND_UP_BIAS;
    if (!Number.isFinite(sum) || Math.abs(sum) >= 9223372036854775808) return 0;
    return Math.fround(Number(BigInt.asIntN(32, BigInt(Math.trunc(sum)))));
}

// 건설 배치 통지 처리기이다. pool(10.78 SidPool)·types(현재 타입 표 188칸)·state·hooks 는 이 객체보다 오래 살아야 하며 복사하지 않는다. 필수 효과가 하나라도 없으면 첫 효과 전에 거부한다.
// 사용: const notice = new RawConstructionNotice(pool, types, state, hooks); notice.handle(값형 통지) 또는 notice.actualHandle(패킷 바이트). (2026-11-08 WB04-E134)
export class RawConstructionNotice {
    #pool;
    #types;
    #state;
    #hooks;

    // 판본·타입 표 크기·필수 하위 효과를 확인한다. (2026-11-08 WB04-E134)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 배치 통지는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !state) throw new TypeError('건설 배치 통지의 효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 배치 통지의 효과 연결 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 풀(연결할 실제 모듈의 풀 일치 확인용)이다. (2026-11-08 WB04-E134)
    get pool() { return this.#pool; }

    // 배치 후 슬롯을 다시 읽어 객체 타입의 genus(flags2)를 구한다. 통지 타입과 실제 조각 타입은 같다고 가정하지 않는다. (2026-11-08 WB04-E134)
    #genus(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('건설 배치 통지 객체 타입 오류');
        return this.#types[type].flags2 >>> 0;
    }

    // 현재 사제 좌표로 접근점을 구해 거의 올림한 다음 이동을 시도한다. 재시도에서도 사제 좌표를 새로 읽는다(첫 이동 경계가 좌표를 바꿀 수 있음). (2026-11-08 WB04-E134)
    #approach(priest, object, flags) {
        const raw = this.#pool.slot(priest);
        const point = this.#hooks.approach(object, coordinate(raw, X_AT), coordinate(raw, Y_AT), flags);
        return this.#hooks.tryMove(priest, snap(point.x), snap(point.y), object);
    }

    // 최근 건설 위치 이력을 뒤에서부터 밀어 가장 오래된 좌표를 버리고 첫 칸에 새 좌표를 넣은 뒤 순번을 0 으로 쓴다. 사본이 아니라 현재 장부를 갱신한다. (2026-11-08 WB04-E134)
    #recordHistory(x, y) {
        const history = this.#state.historyPoints();
        // 뒤에서 앞으로 한 칸씩 복사한다.
        for (let index = history.length - 1; index > 0; index -= 1) history[index] = { x: history[index - 1].x, y: history[index - 1].y };
        history[0] = { x, y };
        this.#state.setHistoryCounter(0);
    }

    // 사제 이동 대상 첫 객체의 사제 분기이다: 이동하면 참, 못 움직이면 환불·삭제하고 거짓을 돌려준다. (2026-11-08 WB04-E134)
    #priestMove(priest, first) {
        return (this.#pool.slot(priest)[STATE] & 3) === 0 && (this.#genus(priest) & PRIEST) !== 0 && !this.#hooks.immobile(priest)
            && (this.#approach(priest, first, FIRST_APPROACH) || this.#approach(priest, first, RETRY_APPROACH));
    }

    // 로컬 예측 정리는 항상 배치보다 먼저이다. abstract 가 아니어도 이력·배치·최종 갱신은 수행한다. (2026-11-08 WB04-E134)
    handle(notice) {
        if (notice.type >= this.#types.length) throw new RangeError('건설 배치 통지 타입 번호 오류');
        if (notice.sids.length === 0 || notice.sids.length > MAX_PIECES) throw new RangeError('건설 배치 통지 SID 개수 오류');
        // 잘못된 목록은 정리나 이력 변경 전에 거부한다.
        for (const sid of notice.sids) {
            if (!sid) throw new RangeError('건설 배치 통지 SID 0 오류');
            this.#pool.slot(sid);
        }
        const hooks = this.#hooks;
        const state = this.#state;
        hooks.clear({ type: notice.type, x: notice.x, y: notice.y, argument: notice.argument, direction: notice.direction, player: notice.player });
        const type = this.#types[notice.type];
        const abstract = ((type.flags1 & HAS_HP) !== 0 || notice.type === state.daisType) && (notice.flags & 1) === 0;
        const needsBuilder = (type.flags1 & NEEDS_BUILDER) !== 0 && (notice.flags & 1) === 0;
        if ((type.group | 0) !== NO_HISTORY_GROUP && notice.player === state.localPlayer) this.#recordHistory(notice.x, notice.y);
        hooks.place({ type: notice.type, x: notice.x, y: notice.y, argument: notice.argument, direction: notice.direction, player: notice.player,
            sids: notice.sids, abstract: abstract ? 1 : 0, quality: notice.quality });
        const first = notice.sids[0];
        if (abstract) {
            if (needsBuilder) {
                if (notice.player !== state.localPlayer || state.moveLocalBuilder) hooks.moveBuilder(notice.player, first, notice.timeLow, notice.timeHigh);
            } else if (state.authorityNow()) {
                if ((this.#genus(first) & PRIEST_TARGETS) !== 0) {
                    const priest = hooks.findPriest(notice.player);
                    const contained = hooks.containedType(state.priestType);
                    const current = hooks.findPriest(notice.player);
                    hooks.removeContained(current, contained);
                    if (!this.#priestMove(priest, first)) {
                        const raw = this.#pool.slot(first);
                        hooks.refund(raw[TYPE], raw[PATCH_OWNER]);
                        hooks.destroy(first, 0);
                    }
                } else hooks.start(first, 0);
            }
        }
        hooks.refresh();
    }

    // 4441b0 의 실제 BYTE 경로이다. 같은 packet 주소를 Clear/Place 뒤에도 읽으며 typed handle 의 전체 선검사 계약과 구별한다. (2026-11-08 WB04-E134)
    actualHandle(packet) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (typeof hooks.actualPlace !== 'function') throw new Error('실제 건설 wire 통지의 actualPlace 장치 없음');
        const wire = new ConstructionPacketView(packet);
        const firstType = wire.type();
        const direction = wire.direction();
        const argument = wire.argument();
        if (firstType >= this.#types.length) throw new RangeError('실제 건설 wire 통지의 TYPE 물리 범위');
        // Clear 가 packet TYPE 을 바꾸어도 진입 때 선택한 같은 레코드를 읽는다.
        const selectedType = this.#types[firstType];
        // 4441cd..e2 의 현재 인자를 Clear 호출 직전에 고정한다.
        const clearPlayer = wire.player();
        const clearY = wire.y();
        const clearType = wire.type();
        const clearX = wire.x();
        hooks.clear({ type: clearType, x: clearX, y: clearY, argument, direction, player: clearPlayer });
        // Clear 뒤 같은 TYPE 의 현재 flags DWORD 한 번으로 두 분기를 선택한다.
        const flags = selectedType.flags1;
        const abstract = ((flags & HAS_HP) !== 0 || wire.type() === state.daisTypeNow()) && (wire.flags() & 1) === 0;
        const needsBuilder = (flags & NEEDS_BUILDER) !== 0 && (wire.flags() & 1) === 0;
        if ((selectedType.group | 0) !== NO_HISTORY_GROUP && wire.player() === state.localPlayerNow()) this.#recordHistory(wire.x(), wire.y());
        // 444301 의 품질은 Place 전에 보관하고 이후 재조회하지 않는다.
        const quality = wire.quality();
        // 40a840 은 각 decoder 칸의 현재 홀수 주소 WORD 를 읽는 pointer 이다.
        const sidReader = (index) => wire.sidAt(index);
        // 444310..29 의 scalar 인자는 Place 안에서 packet 변경으로 바뀌지 않는다.
        const count = wire.sidCount();
        const y = wire.y();
        const player = wire.player();
        const x = wire.x();
        const type = wire.type();
        hooks.actualPlace({ type, x, y, argument, direction, player, sids: [], abstract: abstract ? 1 : 0, quality }, count, sidReader);
        if (abstract) {
            if (needsBuilder) {
                if (wire.player() !== state.localPlayerNow() || state.moveLocalBuilder) {
                    // 선택된 이동 분기만 원문 순서의 시각/첫 SID/현재 owner 를 읽는다.
                    const high = wire.timeHigh();
                    const low = wire.timeLow();
                    const first = wire.sidAt(0);
                    const owner = wire.player();
                    hooks.moveBuilder(owner, first, low, high);
                    hooks.refresh();
                    return;
                }
            } else if (state.authorityNow()) {
                // Place 뒤 현재 boss 일 때만 실제 첫 대상 주소를 보관한다.
                const first = wire.sidAt(0);
                if ((this.#genus(first) & PRIEST_TARGETS) !== 0) {
                    // 첫 사제 주소는 보관하고 현재 Priest TYPE 의 contained 를 찾는다.
                    const priest = hooks.findPriest(wire.player());
                    const contained = hooks.containedType(state.priestTypeNow());
                    // 두 번째 사제 선택은 현재 packet owner 를 다시 읽는다.
                    const current = hooks.findPriest(wire.player());
                    hooks.removeContained(current, contained);
                    if (!this.#priestMove(priest, first)) {
                        // 처음 선택한 대상의 현재 TYPE/owner 로 환불 후 같은 대상을 지운다.
                        const raw = this.#pool.slot(first);
                        hooks.refund(raw[TYPE], raw[PATCH_OWNER]);
                        hooks.destroy(first, 0);
                        hooks.refresh();
                        return;
                    }
                // 자동 시작은 원문처럼 packet 의 첫 SID 를 다시 읽는다.
                } else hooks.start(wire.sidAt(0), 0);
            }
        }
        // abstract 가 아니거나 선택되지 않은 Builder 분기도 첫 SID 없이 이 꼬리만 실행한다.
        hooks.refresh();
    }
}

// 외부 효과 가운데 이미 복원된 몸체만 실제 모듈로 교체한다: 정리(clear.clear)·배치(place.place/actualPlace)·환불(place.refund)과 priest 가 있으면 이동 불가 조회(priest.immobile).
// 하나라도 풀이 다르면 거부한다. 사용: makeConstructionNoticeHooks(pool, clear, place, 나머지 경계, priest). (2026-11-08 WB04-E134)
export function makeConstructionNoticeHooks(pool, clear, place, hooks, priest = null) {
    if (pool !== clear?.pool || pool !== place?.pool || (priest && pool !== priest.pool)) throw new TypeError('건설 배치 통지 어댑터의 풀 연결 오류');
    const connected = {
        ...hooks,
        clear: (request) => { clear.clear(request); },
        place: (request) => { place.place(request); },
        // 실제 wire 의 lazy SID 경계를 같은 풀 배치에 결합한다.
        actualPlace: (request, count, reader) => { place.actualPlace(request, count, reader); },
        refund: (type, player) => { place.refund(type, player); },
    };
    if (priest) connected.immobile = (sid) => priest.immobile(sid);
    return connected;
}

// 서버가 전송한 것과 같은 통지를 직접 처리하는 확정 경계를 같은 풀의 실제 통지 처리기로 연결한다. 풀이 다르면 거부한다. (2026-11-08 WB04-E134)
export function makeConstructionNoticeConfirmHooks(pool, notice, hooks) {
    if (pool !== notice?.pool) throw new TypeError('건설 확정 통지 어댑터의 풀 연결 오류');
    return { ...hooks, handle: (value) => { notice.handle(value); } };
}
