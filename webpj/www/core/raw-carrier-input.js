// 운반물(Carrier)의 커서 평가(4277f0)·강조(4284a0)·클릭 명령 분배(42a360)·이동 확인 음향(426290)을 옮긴 모듈이다(cpppj `client/RawCarrierInput.cpp/.h`, 10.78 전용).
// 원본 Carrier.cpp 의 조기 반환과 효과 뒤 현재 상태(TYPE·소유자·전역) 재조회를 유지하고, 큰 월드/포획/전송/희생 UI 와 실제 커서·음향 장치는 훅으로 받는다.
//  - evaluate(parent, point): 부모 TYPE genus(flags2)에 0x30000(이동/운반)이 없으면 진단(0x7b6) 뒤, 또는 extra&0x20·사제 HP/지면 검사(immobile)면 커서 6 이다. 아니면 부모 현재 위치를 snap 한 자기 사각형(폭 0.999)에
//    점이 들어 있는지 본다. 안에 있으면 insidePoint 를 -10000 으로 쓰고, 내용물이 있고 메뉴 생략 TYPE 이 아니면 내려놓기 위치(dropPoint)를 부른 뒤 6, 아니면 포획 후보가 있고 같은 Graph 면 8, 아니면 6 이다.
//    밖이면 후보를 찾고 genus 0x20000(비행) 이 없으면 후보 없음일 때 제단(0x200000 genus이고 제단 허용이면 점을 제단 위치로 바꾸고 0x11)·연결 검사 실패 시 간헐천/귀환 장소(커서 3, 점을 그 위치로)·연결되면 제단 없음 0x12/있음 3(점-3)을 정하고,
//    후보가 있고 (너겟이거나 내용물이 없고) 같은 Graph 면 0xd 로 덮은 뒤 현재 커서를 반환한다. 비행이면 후보 0xd → 간헐천 3 → 귀환 장소 3 → 제단 3(점-3) → 0x12 순이다. 점은 caller 의 객체를 직접 바꾼다.
//  - highlight(number, point, force): 점을 snap 한다. number 가 0 이면 진단(0xce)만, 아니면 진단 이동점 표시(markers)가 켜졌을 때 표시 타입(markerTypes[카운터 % 개수])과 snap 점으로 표시·커서 갱신 후 카운터를 DWORD 감김으로 증가시킨다.
//    force 가 없고 다중 선택이 아니면 evaluate 결과를 커서로 지정(selectCursor), 아니면 0x12 를 지정한다.
//  - commit(number, point): 점을 snap 한다. number 0 이면 highlight(0)만, 부모 extra&1 이면 무시한다. 비행 TYPE 은 점을 최소 (7, 5)로 올린 뒤 evaluate 한다. 커서 3·0xd·0xe·0x12 는 소유자가 로컬이면 확인 음향 후 이동 요청(request, 0),
//    8·9 는 포획(pickup)+선택 해제, 10 은 로컬이면 내려놓기 음향 후 운반물 내려놓기 요청(dropCargo)을 하고 6·0xb·0xc 와 같이 선택 해제, 0x11 은 제단 희생(조기 희생이면 로컬/원격 조기 처리, 아니면 희생 요청 후 로컬이면 확인 음향+제단 보관+희생 표시), 그 밖은 진단(0x179)이다.
//  - acknowledge(parent, index): 같은 TYPE 이 같은 프레임에 이미 소리를 냈으면 무시하고 아니면 장부를 갱신한다. 사제는 index 를 0..1(범위 밖이면 난수(0,2000)/1000), 골렘은 0..4(난수(0,4000)/1000)로 이름 소리, 나머지는 TYPE 이동 음향 핸들이다.
// state = CarrierInputState({ cursor, dais, priestType(158), golemType(133), insidePoint, markers, markerCounter, markerTypes }), audio[type] = { moveSound, dropSound }, stamps 는 TYPE별 프레임 장부(Uint32Array, 188 이상),
// frame 은 현재 프레임의 `{ value }` 또는 getter, hooks 는 위 파일 머리의 CarrierInputHooks 대응 함수들(immobile·content·candidate[{ sid, nugget }]·dropPoint·graph·connected·surface·geyser·returnPlace·daisAt·validGeyser·altarAllowed·
// localOwner·multiple·checking·earlySacrifice·assertion·highlight·selectCursor·select·request·pickup·dropCargo·earlyLocal·refreshCursor·earlyRemote·showSacrifice·marker·random·namedSound·sound)이며 도달한 훅이 없으면 오류이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E97: 운반물 입력 이식으로 추가)
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';

// 부모 TYPE 번호·소유자·extra 위치와 내용물 종류 WORD 위치이다. (2026-11-08 WB04-E97)
const TYPE_AT = 10;
const OWNER_AT = 34;
const EXTRA_AT = 40;
const KIND_AT = 18;
const X_AT = 14;
const Y_AT = 18;
const TYPE_COUNT = 188;
// genus(flags2)의 이동 가능 계열·비행·제단 비트, 메뉴 생략 flags1 비트, extra 의 이동 불가·선택 해제 비트이다. (2026-11-08 WB04-E97)
const MOVER = 0x30000;
const FLYER = 0x20000;
const ALTAR = 0x200000;
const NO_MENU = 0x1000000;
const PARALYZED = 0x20;
const DISABLED = 1;
// 원본 503b90 의 자기 클릭 사각형 폭(단정도)과 자기 사각형에서 기록하는 값이다. (2026-11-08 WB04-E97)
const INSIDE_EXTENT = Math.fround(0.9990000128746033);
const INSIDE_MARK = -10000;
const fround = Math.fround;

// 도달한 미연결 효과만 그 지점에서 거부한다. (2026-11-08 WB04-E97)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('운반체 실제 사용자입력 하위 미연결');
    return hook;
}

// 선택·강조 전역과 다른 Carrier.cpp 고유 전역의 단일 저장소이다. (2026-11-08 WB04-E97)
export class CarrierInputState {
    // 5453d8/5453dc: 현재 커서 코드와 희생 대상이다.
    cursor = 0;
    dais = 0;
    // 5412d0/54126c: 현재 타입 getter 이며 원본 초기 번호이다.
    priestType = 158;
    golemType = 133;
    // 5453ec/f0: 자기 사각형에서만 -10000 을 기록한다.
    insidePoint = [0, 0];
    // 5453e0: 진단용 이동점 표시이다(일반 미션 부모가 0 으로 초기화).
    markers = false;
    // 54541c: signed 나머지와 DWORD 감김 증가를 보존한다.
    markerCounter = 0;
    // 54d928/531858: 실제 표시 타입 배열이다.
    markerTypes = [];
}

// 운반물 사용자 입력 처리기이다. (2026-11-08 WB04-E97)
export class RawCarrierInput {
    // 동일한 실제 객체/내용물 풀이다. (2026-11-08 WB04-E97)
    #pool;
    // 현재 188 TYPE 표이다. (2026-11-08 WB04-E97)
    #types;
    // TYPE 별 음향 핸들 표이다. (2026-11-08 WB04-E97)
    #audio;
    // 실제 SoundProcess 와 공유하는 TYPE+1f0 프레임 장부이다. (2026-11-08 WB04-E97)
    #stamps;
    // 원본 55b4a8 프레임을 읽는 소유자이다. (2026-11-08 WB04-E97)
    #frame;
    // 커서/희생/진단 표시의 현재 전역이다. (2026-11-08 WB04-E97)
    #state;
    // 선택된 큰 실제 하위와 장치 경계이다. (2026-11-08 WB04-E97)
    #hooks;

    // 같은 188TYPE/음향 장부/PATCH 를 검증한다. (2026-11-08 WB04-E97)
    constructor(pool, types, audio, stamps, frame, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !audio || audio.length !== TYPE_COUNT || !stamps || stamps.length < TYPE_COUNT) {
            throw new Error('운반체 입력의 같은 PATCH/TYPE/음향 장부 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#audio = audio;
        this.#stamps = stamps;
        this.#frame = frame;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 프레임이다(getter 또는 { value }). (2026-11-08 WB04-E97)
    #currentFrame() { return (typeof this.#frame === 'function' ? this.#frame() : this.#frame.value) >>> 0; }

    // 원본 물리 SID 범위만 확인한다(free/dead 필터를 추가하지 않는다). (2026-11-08 WB04-E97)
    #number(value) {
        if (!value || value >= this.#pool.capacity) throw new RangeError('운반체 입력 SID 물리 범위');
        return value;
    }

    // 현재 50바이트 슬롯(사본)을 효과 뒤 다시 읽는다. (2026-11-08 WB04-E97)
    #raw(parent) { return this.#pool.slot(this.#number(parent)); }

    // 현재 TYPE 번호를 검증한다. (2026-11-08 WB04-E97)
    #type(parent) {
        const type = this.#raw(parent)[TYPE_AT];
        if (type >= this.#types.length) throw new RangeError('운반체 입력 TYPE 범위');
        return type;
    }

    // 가상 효과 뒤 원본 위치에서 TYPE+ec(flags2, genus)를 재조회한다. (2026-11-08 WB04-E97)
    #genus(parent) { return this.#types[this.#type(parent)].flags2 >>> 0; }

    // 현재 TYPE+e8(flags1)을 genus 와 구별해 읽는다. (2026-11-08 WB04-E97)
    #flags(parent) { return this.#types[this.#type(parent)].flags1 >>> 0; }

    // 현재 소유자 BYTE 이다. (2026-11-08 WB04-E97)
    #owner(parent) { return this.#raw(parent)[OWNER_AT]; }

    // 현재 위치 두 float 를 변환 없이 읽는다. (2026-11-08 WB04-E97)
    #position(parent) {
        const raw = this.#raw(parent);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return { x: view.getFloat32(X_AT, true), y: view.getFloat32(Y_AT, true) };
    }

    // 반환 코드가 전역 cursor 에 기록되는 분기와 직접 반환되는 분기를 구별한다. point 는 caller 의 객체를 직접 바꾼다. (2026-11-08 WB04-E97)
    evaluate(parent, point) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (!(this.#genus(parent) & MOVER)) need(hooks.assertion)(0x7b6);
        if (!(this.#genus(parent) & MOVER) || (this.#raw(parent)[EXTRA_AT] & PARALYZED)) { state.cursor = 6; return 6; }
        if (need(hooks.immobile)(parent)) { state.cursor = 6; return 6; }
        const position = this.#position(parent);
        const snapped = RawCarrierDropPoint.snap([position.x, position.y]);
        const lowX = fround(snapped[0] - INSIDE_EXTENT);
        const lowY = fround(snapped[1] - INSIDE_EXTENT);
        const contained = need(hooks.content)(parent, 0x20);
        need(hooks.content)(parent, 1);
        if (lowX <= point.x && point.x <= snapped[0] && lowY <= point.y && point.y <= snapped[1]) {
            state.insidePoint = [INSIDE_MARK, INSIDE_MARK];
            if (contained && !(this.#flags(parent) & NO_MENU)) {
                need(hooks.dropPoint)(parent, this.#raw(contained)[KIND_AT] | (this.#raw(contained)[KIND_AT + 1] << 8));
                return 6;
            }
            const candidate = need(hooks.candidate)(parent, this.#position(parent));
            if (!(this.#flags(parent) & NO_MENU) && candidate.sid) {
                const graph = need(hooks.graph)(parent);
                if (need(hooks.graph)(candidate.sid) === graph) return 8;
            }
            return 6;
        }
        const candidate = need(hooks.candidate)(parent, point);
        if (!(this.#genus(parent) & FLYER)) {
            if (!candidate.sid) {
                let dais = 0;
                if ((this.#genus(parent) & ALTAR) && (dais = need(hooks.daisAt)(point)) && need(hooks.altarAllowed)(dais, parent)) {
                    const at = this.#position(dais);
                    point.x = at.x;
                    point.y = at.y;
                    state.cursor = 0x11;
                } else if (!need(hooks.connected)(this.#position(parent), point)) {
                    let geyser = 0;
                    let place = 0;
                    if ((need(hooks.surface)(point) & 2) && (geyser = need(hooks.geyser)(point)) && need(hooks.validGeyser)(parent, geyser)) {
                        const at = this.#position(geyser);
                        point.x = at.x;
                        point.y = at.y;
                        state.cursor = 3;
                    } else if ((need(hooks.surface)(point) & 8) && (place = need(hooks.returnPlace)(point))) {
                        const at = this.#position(place);
                        point.x = at.x;
                        point.y = at.y;
                        state.cursor = 3;
                    } else {
                        state.cursor = 6;
                    }
                } else {
                    dais = need(hooks.daisAt)(point);
                    if (!dais) {
                        state.cursor = 0x12;
                    } else {
                        const at = this.#position(dais);
                        point.x = fround(at.x - 3);
                        point.y = fround(at.y - 3);
                        state.cursor = 3;
                    }
                }
            }
            if (!(this.#flags(parent) & NO_MENU) && candidate.sid && (candidate.nugget || !contained)) {
                const graph = need(hooks.graph)(parent);
                if (need(hooks.graph)(candidate.sid) === graph) state.cursor = 0xd;
            }
            return state.cursor >>> 0;
        }
        if (candidate.sid && (candidate.nugget || !contained)) return 0xd;
        if ((need(hooks.surface)(point) & 2) && need(hooks.geyser)(point)) return 3;
        if ((need(hooks.surface)(point) & 8) && need(hooks.returnPlace)(point)) return 3;
        const dais = need(hooks.daisAt)(point);
        if (dais) {
            const at = this.#position(dais);
            point.x = fround(at.x - 3);
            point.y = fround(at.y - 3);
            return 3;
        }
        return 0x12;
    }

    // 실제 선택 모드 5 와 현재 마우스 폴링이 같은 평가기를 사용한다. point 는 값 복사이다. (2026-11-08 WB04-E97)
    highlight(number, point, force) {
        const hooks = this.#hooks;
        const state = this.#state;
        const snapped = RawCarrierDropPoint.snap([point.x, point.y]);
        point = { x: snapped[0], y: snapped[1] };
        if (!number) { need(hooks.assertion)(0xce); return; }
        const parent = this.#number(number);
        if (state.markers) {
            const snap = RawCarrierDropPoint.snap([point.x, point.y]);
            if (state.markerTypes.length === 0) throw new Error('이동 진단 표시 타입 누락');
            const index = (state.markerCounter | 0) % state.markerTypes.length;
            if (index < 0) throw new RangeError('이동 진단 표시 음수 나머지');
            state.markerCounter = ((state.markerCounter >>> 0) + 1) | 0;
            need(hooks.marker)({ x: snap[0], y: snap[1] }, state.markerTypes[index]);
            need(hooks.refreshCursor)();
        }
        if (!force && !need(hooks.multiple)()) {
            const cursor = this.evaluate(parent, point);
            need(hooks.selectCursor)(cursor);
            return;
        }
        need(hooks.selectCursor)(0x12);
    }

    // 같은 TYPE 당 프레임 중복 음향을 억제한다. 원본이 현재 TYPE 을 다시 읽는 시점을 보존한다. (2026-11-08 WB04-E97)
    acknowledge(parent, index) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (this.#stamps[this.#type(parent)] === this.#currentFrame()) return;
        this.#stamps[this.#type(parent)] = this.#currentFrame();
        if (this.#raw(parent)[TYPE_AT] === state.priestType) {
            if (index < 0 || index > 1) index = Math.trunc(need(hooks.random)(0, 2000) / 1000);
            need(hooks.namedSound)(true, index >>> 0);
            return;
        }
        if (this.#raw(parent)[TYPE_AT] === state.golemType) {
            if (index < 0 || index > 4) index = Math.trunc(need(hooks.random)(0, 4000) / 1000);
            need(hooks.namedSound)(false, index >>> 0);
            return;
        }
        need(hooks.sound)(this.#audio[this.#type(parent)].moveSound);
    }

    // 클릭 전의 TYPE 과 클릭 후 현재 소유자/전역을 원문 시점에 각각 읽어 클릭 의미를 분배하고 실제 요청만 송신한다. point 는 값 복사이다. (2026-11-08 WB04-E97)
    commit(number, point) {
        const hooks = this.#hooks;
        const state = this.#state;
        const snap = RawCarrierDropPoint.snap([point.x, point.y]);
        point = { x: snap[0], y: snap[1] };
        if (!number) { need(hooks.highlight)(0); return; }
        const parent = this.#number(number);
        if (this.#raw(parent)[EXTRA_AT] & DISABLED) return;
        const type = this.#type(parent);
        if (this.#types[type].flags2 & FLYER) {
            if (point.x <= 7) point.x = 7;
            if (point.y <= 5) point.y = 5;
        }
        const cursor = this.evaluate(parent, point);
        switch (cursor) {
        case 3: case 0xd: case 0xe: case 0x12:
            if (this.#owner(parent) === need(hooks.localOwner)()) this.acknowledge(parent, -1);
            need(hooks.request)(parent, point, 0);
            return;
        case 8: case 9:
            need(hooks.pickup)(parent);
            need(hooks.select)(0, 0, 1);
            return;
        case 10:
            if (this.#owner(parent) === need(hooks.localOwner)()) need(hooks.sound)(this.#audio[type].dropSound);
            need(hooks.dropCargo)(parent, 0);
            need(hooks.select)(0, 0, 1);
            return;
        case 6: case 0xb: case 0xc:
            need(hooks.select)(0, 0, 1);
            return;
        case 0x11: {
            const dais = need(hooks.daisAt)(point);
            if (!dais) { need(hooks.assertion)(0x153); return; }
            if (need(hooks.checking)() && !(this.#genus(parent) & ALTAR)) need(hooks.assertion)(0x15a);
            if (need(hooks.altarAllowed)(dais, parent)) {
                if (need(hooks.earlySacrifice)()) {
                    if (this.#owner(parent) === need(hooks.localOwner)()) need(hooks.earlyLocal)();
                    else need(hooks.earlyRemote)(this.#owner(parent));
                    return;
                }
                need(hooks.request)(parent, point, 1);
                if (this.#owner(parent) === need(hooks.localOwner)()) {
                    this.acknowledge(parent, -1);
                    state.dais = dais;
                    need(hooks.showSacrifice)(dais);
                }
            }
            return;
        }
        default:
            need(hooks.assertion)(0x179);
        }
    }
}
