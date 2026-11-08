// 운반물(Carrier)의 운반물·주문 메뉴 부모 몸체(004288f0)를 같은 contained 탐색과 controlled 메뉴 경계로 옮긴 모듈이다(cpppj `client/RawCarrierMenu.cpp/.h`, 10.78 전용).
// 내려놓기/주문 실행 자체가 아니라 메뉴 작성만 한다. 새 운반물 부모를 열 때마다 TYPE flags1 의 0x1000000(메뉴 생략) 비트가 없으면 아래를 수행하고, 어느 경우든 마지막에 기본 객체 메뉴(damageableMenu)를 부른다.
//  - open(carrier): 부모의 이동 불가(extra&0x20 또는 지상 고정 groundImmobile; 단락 평가)를 계산하고 안내 "~wNothing" 서식을 한 번 만든 뒤, 실제 contained finder 로 내용물을 현재 체인 순서로 순회한다.
//    각 내용물마다 ① 설명(description[내용물 종류 WORD +18]) ② TYPE flags2 에 0x20000(비행 가능)이 있으면 이동 종류(movementType)→경로(findPath)→경로 활성(pathActive)으로 "비행 중"을 판단한다 ③ 내려놓을 위치(dropPoint)를
//    두 축 모두 0 초과·현재 지도 한계 미만이면 활성으로 보고 ④ 내용물 flags WORD(+20)를 읽는다. 이동 불가면 "(Not while paralyzed)", 비행 중이면 "(Not while flying)" 안내를 만들고 비활성이다.
//    flags 의 bit0 이 있으면(주문) 현재 정수 비용(patchCost)과 현재 분산 SP(money(로컬 소유자))를 비교해 편집 모드이거나 비용이 SP 이하일 때 활성이며 "for %s%d~[IfortGump.3]" 안내를 만든다(SP 조회 뒤에 편집 전역을 읽는다).
//    bit0 이 없으면(운반물) "Carrying: ~i%s~.\n" 헤더를 쓰고, 부모 소유자(+34)가 로컬 소유자일 때 "%sDrop~. ~i%s~. %s" 항목(Drop 명령, 활성은 이동 불가 아님 && 활성)을 쓴다.
//    bit0 이 있고 부모 소유자가 로컬 소유자이면 Cast 항목(차단 이벤트 0x493·사제 부모의 건설 이벤트 0x25d 가 있으면 비활성)과 "About %s"(도움말 명령, 내용물 종류, 항상 활성)를 쓴다.
//    소유자·이동 종류·내용물 flags 는 외부 콜백이 바꿀 수 있으므로 쓰는 시점마다 현재 슬롯에서 다시 읽는다. 서식 결과는 NUL 뒤를 버리고 256바이트 이상이면, 안내는 128바이트 이상이면 거부한다.
// state = { localOwner, editor, mapSize, helpCommand, priestType(기본 158), dropCommand(기본 0x03000076), castCommand(기본 0x0300011a), localOwnerRead?(), editorRead?() } 이며 호출마다 현재 값을 읽는다(getter 가 있으면 우선).
// contained 는 ContainedFinderState, hooks = { groundImmobile(sid)→bool, movementType(type)→n, findPath(parent, type)→sid, pathActive(sid)→bool, dropPoint(sid, type)→[x, y], patchCost(type)→int32,
// money(owner)→float, hasEvent(sid, event)→bool, description(type)→문자열, format(key, args)→문자열, header(text), item(text, command, parameter, enabled), damageableMenu(sid) } 이고 도달한 훅이 없으면 오류이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E96: 운반물 메뉴 이식으로 추가. 실제 controlled 메뉴/기본 객체 메뉴/월드 연결은 해당 모듈 이식 뒤 bind* 로 잇는다)
import { RawContainedFinder } from './raw-contained-finder.js';

// 슬롯의 TYPE(+10)·소유자(+34)·extra(+40) 위치, 내용물 종류/flags WORD 위치, TYPE 표 크기이다. (2026-11-08 WB04-E96)
const TYPE_AT = 10;
const OWNER_AT = 34;
const EXTRA_AT = 40;
const KIND_AT = 18;
const FLAGS_AT = 20;
const TYPE_COUNT = 188;
// TYPE flags1 의 메뉴 생략 비트, flags2 의 비행 가능 비트, extra 의 이동 불가 비트, 내용물 flags 의 주문 비트이다. (2026-11-08 WB04-E96)
const NO_MENU = 0x1000000;
const FLYER = 0x20000;
const PARALYZED = 0x20;
const SPELL = 1;
// 이벤트 번호: 차단 0x493, 사제 부모의 건설 0x25d 이다. (2026-11-08 WB04-E96)
const BLOCKED_EVENT = 0x493;
const BUILDING_EVENT = 0x25d;
// 원본 서식 버퍼 한도(256)와 안내 문자열 한도(128), 비활성 색 접두이다. (2026-11-08 WB04-E96)
const FORMAT_LIMIT = 256;
const SUFFIX_LIMIT = 128;
const DISABLED = '~r~E';
const encoder = new TextEncoder();

// 비정렬 little endian WORD 이다. (2026-11-08 WB04-E96)
const word = (raw, offset) => raw[offset] | (raw[offset + 1] << 8);

// 도달한 실제 하위를 빈 성공/금액/위치로 대체하지 않는다. (2026-11-08 WB04-E96)
function required(hook) {
    if (typeof hook !== 'function') throw new Error('실제 Carrier 메뉴 하위 미연결');
    return hook;
}

// 문자열의 UTF-8 바이트 수이다(원본 버퍼 한도 비교용). (2026-11-08 WB04-E96)
const byteLength = (text) => encoder.encode(text).length;

// 운반물 메뉴 작성기이다. (2026-11-08 WB04-E96)
export class RawCarrierMenu {
    // 모든 부모/내용물이 쓰는 같은 실제 SID 풀이다. (2026-11-08 WB04-E96)
    #pool;
    // 현재 TYPE 표(188개)이다. (2026-11-08 WB04-E96)
    #types;
    // 실제 contained finder 가 읽는 현재 전역이다. (2026-11-08 WB04-E96)
    #contained;
    // 다른 메뉴와 공유하는 현재 전역이다. (2026-11-08 WB04-E96)
    #state;
    // 실제 의존 몸체와 메뉴 효과 경계이다. (2026-11-08 WB04-E96)
    #hooks;

    // 현재 풀/TYPE/contained 상태와 전역/하위 몸체는 이 처리기보다 오래 살아야 한다. PATCH(10.78) 전용이다. (2026-11-08 WB04-E96)
    constructor(pool, types, contained, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT) throw new Error('Carrier 메뉴 TYPE 표 크기 오류');
        this.#pool = pool;
        this.#types = types;
        this.#contained = contained;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 메뉴가 읽는 물리 SID 범위만 확인하고 현재 슬롯 사본을 돌려준다. (2026-11-08 WB04-E96)
    #raw(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('Carrier 메뉴 SID 범위 오류');
        return this.#pool.slot(sid);
    }

    // 현재 TYPE 참조이다. (2026-11-08 WB04-E96)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('Carrier 메뉴 TYPE 범위 오류');
        return this.#types[type];
    }

    // 현재 소유자 BYTE 이다. (2026-11-08 WB04-E96)
    #owner(sid) { return this.#raw(sid)[OWNER_AT]; }

    // 현재 로컬 Player 번호이다(getter 가 있으면 매번 getter). (2026-11-08 WB04-E96)
    #localOwner() { return (this.#state.localOwnerRead ? this.#state.localOwnerRead() : this.#state.localOwner) >>> 0; }

    // 현재 편집 모드 여부이다(getter 가 있으면 매번 getter). (2026-11-08 WB04-E96)
    #editor() { return Boolean(this.#state.editorRead ? this.#state.editorRead() : this.#state.editor); }

    // 원본 고정 서식을 만든다. 결과의 NUL 뒤는 버리고 256바이트 이상이면 거부한다. (2026-11-08 WB04-E96)
    #format(key, args = []) {
        let text = required(this.#hooks.format)(key, args);
        const nul = text.indexOf('\0');
        if (nul >= 0) text = text.slice(0, nul);
        if (byteLength(text) >= FORMAT_LIMIT) throw new RangeError('Carrier 메뉴 원본256바이트 서식 초과');
        return text;
    }

    // 후보 위치의 두 축이 모두 0 초과이며 현재 지도 한계(signed32) 미만인지 본다(NaN 은 거부). (2026-11-08 WB04-E96)
    #validPoint(point) {
        const bound = this.#state.mapSize | 0;
        return 0 < point[0] && point[0] < bound && 0 < point[1] && point[1] < bound;
    }

    // 현재 전체 내용물 순서로 Carrying/Drop/Cast/About 을 작성하고 기본 메뉴를 마지막에 호출한다. (2026-11-08 WB04-E96)
    open(carrier) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (!(this.#type(this.#raw(carrier)[TYPE_AT]).flags1 & NO_MENU)) {
            const immobile = (this.#raw(carrier)[EXTRA_AT] & PARALYZED) !== 0 || required(hooks.groundImmobile)(carrier);
            const finder = new RawContainedFinder(this.#pool, this.#contained);
            let content = finder.begin(carrier);
            this.#format('~wNothing');
            // 기존 finder 는 다음 SID 를 미리 저장하므로 현재 내용물의 next 변경에 영향을 받지 않는다.
            for (; content; content = finder.next()) {
                const type = word(this.#raw(content), KIND_AT);
                const description = required(hooks.description)(type);
                let flying = false;
                if (this.#type(this.#raw(carrier)[TYPE_AT]).flags2 & FLYER) {
                    const process = required(hooks.movementType)(this.#raw(carrier)[TYPE_AT]);
                    const path = required(hooks.findPath)(carrier, process);
                    flying = Boolean(path) && Boolean(required(hooks.pathActive)(path));
                }
                let enabled = this.#validPoint(required(hooks.dropPoint)(carrier, type));
                const flags = word(this.#raw(content), FLAGS_AT);
                let suffix = '';
                if (immobile || flying) {
                    suffix = this.#format(immobile ? '~r~E(Not while paralyzed)' : '~r~E(Not while flying)');
                    enabled = false;
                } else if (flags & SPELL) {
                    const amount = required(hooks.patchCost)(type) | 0;
                    const sp = required(hooks.money)(this.#localOwner());
                    enabled = this.#editor() || amount <= sp;
                    suffix = this.#format('for %s%d~[IfortGump.3]', [enabled ? '' : DISABLED, amount]);
                }
                if (byteLength(suffix) >= SUFFIX_LIMIT) throw new RangeError('Carrier 메뉴 원본128바이트 안내 초과');
                if (!(flags & SPELL)) {
                    const label = this.#format('Carrying: ~i%s~.\n', [description]);
                    required(hooks.header)(label);
                    if (this.#owner(carrier) === this.#localOwner()) {
                        const text = this.#format('%sDrop~. ~i%s~. %s', [enabled ? '' : DISABLED, description, suffix]);
                        required(hooks.item)(text, state.dropCommand ?? 0x03000076, carrier, !immobile && enabled);
                    }
                } else if (this.#owner(carrier) === this.#localOwner()) {
                    const blocked = Boolean(required(hooks.hasEvent)(carrier, BLOCKED_EVENT));
                    let building = false;
                    if (this.#raw(carrier)[TYPE_AT] === (state.priestType ?? 158)) building = Boolean(required(hooks.hasEvent)(carrier, BUILDING_EVENT));
                    const text = this.#format('%sCast~. ~i%s~.~[I%d.*] %s', [enabled ? '' : DISABLED, description, type | 0, suffix]);
                    required(hooks.item)(text, state.castCommand ?? 0x0300011a, carrier, !immobile && enabled && !blocked && !building);
                    const about = this.#format('About %s', [description]);
                    required(hooks.item)(about, state.helpCommand, type, true);
                }
            }
        }
        required(hooks.damageableMenu)(carrier);
    }
}

// 기존 전체 Carrier 메뉴가 운반물 내려놓기 위치 계산기(RawCarrierDropPoint)를 부르도록 연결한다. 다른 세션의 풀은 거부한다. (2026-11-08 WB04-E96)
export function bindCarrierMenuDropPoint(hooks, pool, point) {
    if (pool !== point.pool) throw new Error('운반물 메뉴 위치 Pool 연결 오류');
    hooks.dropPoint = (carrier, type) => point.find(carrier, type);
}

// 사제 메뉴의 원본 기본 객체 호출을 실제 Carrier 몸체에 연결한다. (2026-11-08 WB04-E96)
export function bindPriestMenuCarrier(priestHooks, carrier) {
    priestHooks.baseMenu = (sid) => carrier.open(sid);
}
