// 골렘(Carrier)의 Cast/Drop 제어 흐름 — 요청(427070)·메뉴 명령(42aa50)·서버 수신(444bc0)을 옮긴 모듈이다(cpppj `o/RawCarrierDrop.cpp/.h`, 10.78 전용). 원본의 현재 재조회와 조기 반환/계속 진행 순서를 보존한다.
//  - request(parent, content = 0): content 가 0 이면 부모의 첫 내용물부터 주문(내용물 kind 의 flags2 & 0x100)을 건너뛴 첫 비주문을 고른다(없으면 반환). 주문이면 부모 extra bit20·지면 정지(immobile)·비용 승인(afford)을 거친 뒤 현재 비용을 다시 읽어
//    음수 비용으로 SP 를 차감(subtract), 카메라 투영 좌표로 비용 표시(popup), 0.1 초 Regular 예약, 부착 체인에서 통지 프로세스 타입에 통지, 선택 해제를 하고, flags2 & 0x8000 이면 내용물 +22 를 1 로 표시한 뒤 9바이트 요청 `[프로토콜&0x7f, 메시지, 길이(2), 부모(2), 내용물(2), +22 여부]` 를 송신한다.
//  - command(parent, code, listParent): 부모 소유자가 로컬이면 선택(select)·시전(Cast: 처음 내용물 SID 를 보관한 채 플래그를 검색한 뒤 **보관한 SID** 로 요청)·내려놓기·포획(pickup) 명령을 처리해 1 을 돌려주고, 그 밖은 공통 메뉴(baseMenu)로 넘긴다.
//  - receive(sender, packet): 9바이트 미만 거부. 서버가 아니면 진단(0x4cf) 뒤 계속. 부모 dead/free, 내용물 free/dead/void 이거나 내용물 TYPE 이 다르면 반환. 내용물 kind 가 0 이거나 TYPE 수 이상이면(checking 이면 진단 0x4e8) 반환. 내려놓기 위치(dropPoint)가 맵 안 (0, 한계)이 아니면 반환, 맞으면 정수 절삭,
//    입력 큐 명령 후 kind 가 주문이면 송신자 불일치 거부(badOwner)·비용(로컬이면 0)보다 돈이 적으면 환불 통지(중단하지 않음)·이동 프로세스 제거·0.1 초 예약·분리/Unpop·주문 생성/소유자/QA/Pop·활성화 6바이트 로컬 수신 뒤 배포이고,
//    아니면 일반 배치(place)가 성공하면 내용물 +22 를 패킷 값으로 표시하고 파괴한 뒤 부모가 비행 genus(0x20000)가 아니고 정지(standing)도 아니면 회피(1.0, −1)한다. 패킷은 효과 뒤 다시 읽는다.
// 큰 월드/프로세스/장치 효과는 훅으로 연결하며 선택된 경계의 누락은 도달 시 거부한다. state = { request/activation: { message, protocol, length }, localOwner, server, checking, numTypes, contentType, notifyProcessType, queueCommand,
// selectCommand, castCommand, dropCommand, pickupCommand, mapLimit } 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E93: 골렘 Cast/Drop 이식으로 추가)
import { RawContainedFinder } from './raw-contained-finder.js';

// 슬롯의 TYPE(+10)·상태(+11)·내용물 단어 WORD(+12)·좌표(+14/+18)·내용물 종류 WORD(+18)·플래그(+20)·내용물 표시(+22)·소유자(+34)·extra(+40) 위치와 genus 비트, 진단 줄이다. (2026-11-08 WB04-E93)
const TYPE = 10;
const STATE = 11;
const WORD = 12;
const X_AT = 14;
const Y_AT = 18;
const KIND = 18;
const FLAGS = 20;
const MARK = 22;
const OWNER = 34;
const EXTRA = 40;
const SPELL = 0x100;
const CARRY_MARK = 0x8000;
const FLYER = 0x20000;
const SERVER_LINE = 0x4cf;
const KIND_LINE = 0x4e8;
const REGULAR_DELAY = Math.fround(0.1);
const fround = Math.fround;

// 선택된 실제 하위가 없으면 그 호출 위치에서 오류를 보고한다. (2026-11-08 WB04-E93)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Cast/Drop 실제 하위 미연결');
    return hook;
}

// 호출자 소유 패킷/슬롯의 비정렬 WORD/DWORD 를 원본 폭으로 읽고 쓴다. (2026-11-08 WB04-E93)
function bytes(data, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += data[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function put(data, offset, value, width) {
    for (let index = 0; index < width; index += 1) data[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 헤더 작성기는 bit7 만 지우고 선언 길이를 4 이상으로 고치지 않는다. (2026-11-08 WB04-E93)
function header(data, value) {
    data[0] = value.protocol & 0x7f;
    data[1] = value.message & 0xff;
    put(data, 2, value.length, 2);
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E93)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawCarrierDrop {
    // 원본 물리 상태를 공유하는 같은 저장소, 현재 TYPE 표, 종속 탐색기의 공통 전역, 효과 사이에 다시 읽는 실제 Session 값, 도달한 큰 하위의 구현이다.
    #pool;
    #types;
    #contained;
    #state;
    #hooks;

    // 10.78 의 188 TYPE/50바이트 슬롯만 사용하며 생성 시 외부 효과를 실행하지 않는다. (2026-11-08 WB04-E93)
    constructor(pool, types, contained, state, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188) throw new TypeError('Cast/Drop 판본/TYPE 오류');
        this.#pool = pool;
        this.#types = types;
        this.#contained = contained;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 서로 다른 Session 의 하위 연결을 검사할 때 쓰는 풀이다. (2026-11-08 WB04-E93)
    get pool() { return this.#pool; }

    // 같은 물리 슬롯을 현재 위치에서 읽는다. (2026-11-08 WB04-E93)
    #read(sid, offset, width = 4) { return bytes(this.#pool.slot(sid), offset, width); }

    // 현재 부모의 genus 이다(범위 밖 TYPE 거부). (2026-11-08 WB04-E93)
    #genus(sid) {
        const type = this.#read(sid, TYPE, 1);
        if (type >= this.#types.length) throw new RangeError('Cast/Drop 자산 TYPE');
        return this.#types[type].flags2;
    }

    // 내용물의 종류는 BYTE 자산 타입과 다르다(+18 WORD). 외부 효과 뒤 종류를 다시 읽어 genus 를 구한다. (2026-11-08 WB04-E93)
    #kind(form) { return this.#read(form, KIND, 2); }
    #kindGenus(form) {
        const type = this.#kind(form);
        if (type >= this.#types.length) throw new RangeError('Cast/Drop 내용물 TYPE');
        return this.#types[type].flags2;
    }

    // 원본처럼 free 여부를 추가 검사하지 않고 내용물 +22 를 쓴다(물리 free 슬롯에도 같은 BYTE 쓰기). (2026-11-08 WB04-E93)
    #mark(form, value) { this.#pool.physical(form)[MARK] = value; }

    // 원본의 주문 전용 관문을 일반 내려놓기에 추가하지 않는다. (2026-11-08 WB04-E93)
    request(parent, content = 0) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (!content) {
            const finder = new RawContainedFinder(this.#pool, this.#contained);
            content = finder.begin(parent);
            // 주문을 건너뛰고 현재 첫 비주문을 선택한다.
            while (content && (this.#kindGenus(content) & SPELL)) content = finder.next();
            if (!content) return;
        }
        if (this.#kindGenus(content) & SPELL) {
            if (this.#read(parent, EXTRA, 1) & 0x20) return;
            if (need(hooks.immobile)(parent)) return;
            if (!need(hooks.afford)(fround(need(hooks.cost)(this.#kind(content))))) return;
            // 승인 효과 뒤의 현재 비용을 다시 읽는다.
            const cost = need(hooks.cost)(this.#kind(content));
            // 원본 neg 의 signed32 감김이다.
            const negative = (0 - cost) | 0;
            need(hooks.subtract)(state.localOwner, fround(negative));
            const point = need(hooks.project)([coordinate(this.#pool.slot(parent), X_AT), coordinate(this.#pool.slot(parent), Y_AT)]);
            need(hooks.popup)(cost, point, 0);
            need(hooks.regular)(parent, REGULAR_DELAY);
            need(hooks.attached)(parent, (process) => { if (this.#read(process, TYPE, 1) === state.notifyProcessType) need(hooks.notify)(process, 3, 0, 0); });
            need(hooks.select)(0, 0, 1);
        }
        if (this.#kindGenus(content) & CARRY_MARK) this.#mark(content, 1);
        const packet = new Uint8Array(9);
        header(packet, state.request);
        put(packet, 4, parent, 2);
        put(packet, 6, content, 2);
        packet[8] = this.#read(content, MARK, 1) !== 0 ? 1 : 0;
        need(hooks.send)(9, packet);
    }

    // 검색 결과와 요청 대상이 다를 수 있는 원본 메뉴의 최초 SID 보관을 유지한다. (2026-11-08 WB04-E93)
    command(parent, code, listParent) {
        const state = this.#state;
        if (this.#read(parent, OWNER, 1) === state.localOwner) {
            const command = (code & 0x03ffffff) >>> 0;
            if (command === state.selectCommand) { need(this.#hooks.select)(parent, 0, 1); return 1; }
            if (command === state.castCommand) {
                const finder = new RawContainedFinder(this.#pool, this.#contained);
                let current = finder.begin(listParent);
                const initial = current;
                // 현재 플래그를 찾더라도 실제 요청에는 처음 보관한 SID 를 전달한다.
                while (current && !(this.#read(current, FLAGS, 1) & 1)) current = finder.next();
                this.request(parent, initial);
                return 1;
            }
            if (command === state.dropCommand) { this.request(parent, 0); return 1; }
            if (command === state.pickupCommand) { need(this.#hooks.pickup)(parent); return 1; }
        }
        return need(this.#hooks.baseMenu)(parent, code, listParent);
    }

    // 비용 부족 통지 뒤에도 생성 분기를 계속하는 원본 동작을 별도 성공 반환으로 바꾸지 않는다. (2026-11-08 WB04-E93)
    receive(sender, packet) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (packet.length < 9) throw new RangeError('Cast/Drop 요청 패킷 길이 부족');
        if (!state.server) need(hooks.assertion)(SERVER_LINE);
        const parent = bytes(packet, 4, 2);
        if (this.#read(parent, STATE, 1) & 3) return;
        const content = bytes(packet, 6, 2);
        if (!content) return;
        if ((this.#read(content, STATE, 1) & 7) || this.#read(content, TYPE, 1) !== state.contentType) return;
        const kind = this.#kind(content);
        if (state.checking && (!kind || kind >= (state.numTypes | 0))) need(hooks.assertion)(KIND_LINE);
        if (!kind || kind >= (state.numTypes | 0)) return;
        let point = need(hooks.dropPoint)(parent, kind);
        const limit = fround(state.mapLimit);
        if (!(0 < point[0] && point[0] < limit && 0 < point[1] && point[1] < limit)) return;
        point = [fround(Math.trunc(point[0])), fround(Math.trunc(point[1]))];
        need(hooks.queue)(state.queueCommand, kind, 0);
        if (kind >= this.#types.length) throw new RangeError('Cast/Drop 현재 TYPE 물리 범위');
        if (this.#types[kind].flags2 & SPELL) {
            const owner = this.#read(parent, OWNER, 1);
            if (sender !== owner) { need(hooks.badOwner)(kind, sender); return; }
            let cost = need(hooks.cost)(kind);
            if (owner === state.localOwner) cost = 0;
            if (need(hooks.money)(owner) < cost) need(hooks.refund)(kind, this.#read(parent, OWNER, 1), [-10000, -10000], 0, 0);
            const movement = need(hooks.movement)(this.#read(parent, TYPE, 1));
            need(hooks.removeMovement)(parent, movement, 0, 1);
            need(hooks.regular)(parent, REGULAR_DELAY);
            need(hooks.detach)(parent, 0);
            need(hooks.unpop)(parent, 0);
            const born = need(hooks.create)(kind, 0);
            if (!born) throw new Error('Cast/Drop 주문 생성 실패');
            need(hooks.owner)(born, this.#read(parent, OWNER, 1));
            need(hooks.argument)(born, bytes(packet, 4, 2));
            need(hooks.pop)(born, point, 0);
            const activation = new Uint8Array(6);
            header(activation, state.activation);
            put(activation, 4, born, 2);
            need(hooks.activate)(9, 0, activation);
            need(hooks.broadcast)(activation);
            return;
        }
        if (need(hooks.place)(kind, point, this.#read(parent, OWNER, 1), this.#read(content, WORD, 2), 0, 1, 0.0, 0)) {
            this.#mark(content, packet[8] !== 0 ? 1 : 0);
            need(hooks.destroy)(content, 0);
            if (!(this.#genus(parent) & FLYER) && !need(hooks.standing)(parent)) need(hooks.escape)(parent, 1.0, -1);
        }
    }
}
