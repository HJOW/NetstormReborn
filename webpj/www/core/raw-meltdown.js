// 제네레이터(Whirligig 계열) Meltdown 의 대상 선택(420340/4203a0)·요청(41c290/4207a0)·폭발 패킷(4205f0)·실제 피해 순서(4204f0)를 옮긴 모듈이다(cpppj `o/RawMeltdown.cpp/.h`, 10.78 전용, 41c290/420340..420919).
// 원문 대상 개수/현재 조회와 float 저장 위치를 유지하며 일반 주문 피해로 대체하지 않는다. 검색·전송·월드·시청각 효과는 훅이다.
//  - command(parent, code, base): 명령 코드(상위 입력 비트 제거)가 0x030000f8 이면 헤더(요청)+부모 SID(WORD)의 6바이트를 송신(send(9, packet))하고, 어느 경우든 이어서 공통 부모(base)를 호출한다(반환값 없음, release 거부 없음).
//  - mayTarget(target): 다리(genus&4)는 보호 프레임 비트 0x40 만 제외한다(SID 0 은 허용). 그 밖의 대상은 HP 보유 여부(hasHitPoints)와 허용 genus 마스크(0x80074200)를 쓴다.
//  - find(x, y, radius, output): 권한이 없으면 진단(0x664). 반경으로 만든 사각형(내림/올림 스냅·1~255 제한)을 정수 영역으로 같은 네 단계 해시를 훑어, 예약 대역(5..14999)·extra bit0·거리 ≥ 허용(발자국/2+반경, 엄격 `<`)·자격 미달 후보를 거르고 목록 한도까지 담는다(dead 는 여기서 거르지 않음).
//  - apply(x, y, radius, owner, target, parent): dead 대상은 무시. 비다리는 owner/대상 소유자 동맹 표(공격자×9+대상)가 0 이면 피해(damage 100, owner, parent)와 피해 전에 캡처한 대상 중심의 지면 효과(ground)를 한다. 다리는 중심 거리가 `발자국/2+반경−1` 이하이면 삭제(0x100048),
//    아니면 서버(boss)일 때 다리 끝 칸 변환(edge)이다.
//  - receiveRequest(packet): 부모(패킷 WORD +4)가 dead 면 무시, 권한이 없으면 진단(0x6c3). 최초 중심에서 반경 4 로 검색한 뒤 **현재** 부모 중심/소유자로 220바이트 효과 패킷(검색 결과 전체를 WORD 로 복사)을 만들어 배포(broadcast)·로컬 수신(receive(현재 로컬 owner, 0, 패킷))·부모 삭제(0)한다.
//  - receiveEffect(packet): 중심을 스냅해 0.5 를 빼고 세 위치에 시각 효과(variant 1, 0.04)와 소리(whirligigFire-1000.wav)를 먼저 낸 뒤, 패킷의 현재 대상 수(signed)와 대상 WORD 를 원문 위치에서 다시 읽어 dead/삭제 대상을 건너뛰고 apply 한다.
//  - Meltdown.finalizePacket(packet, writing): 송신일 때만 count==-1 진단(오류) 뒤 선언 길이 WORD 를 `(count+10)×2` 로 확정한다.
// state = { boss(), localOwner(), alliances(81 DWORD), request/effect: { message, protocol, length } }, hooks = { hasHitPoints, assertion, find, send, broadcast, receive, destroy, damage, ground, edge, visual, sound }(도달한 것만 필수).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E111: 제네레이터 Meltdown 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯의 TYPE/상태/소유자/프레임/extra/좌표 위치, 명령 코드와 효과 패킷 크기, 시각 변형·주기 상수, 스냅 편향, ftol 경계, 판본의 TYPE 수이다. (2026-11-08 WB04-E111)
const TYPE_AT = 10;
const STATE_AT = 11;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;
const FRAME_AT = 36;
const EXTRA_AT = 40;
const COMMAND = 0x030000f8;
const COMMAND_MASK = 0x03ffffff;
const EFFECT_BYTES = 220;
const REQUEST_RADIUS = 4;
const VISUAL_INTERVAL = new DataView(new Uint8Array([0x0a, 0xd7, 0x23, 0x3d]).buffer).getFloat32(0, true);
const SNAP_UPPER = 0.9999899864196777;
const INT64_BOUNDARY = 2 ** 63;
const TYPE_COUNT = 188;
const fround = Math.fround;

// 미선택 하위를 요구하지 않는다. (2026-11-08 WB04-E111)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Meltdown 실제 하위 미연결');
    return hook;
}

// 패킷의 지정 BYTE/WORD/DWORD 만 조합한다(범위 밖은 거부). (2026-11-08 WB04-E111)
function get(raw, at, width = 4) {
    if (at > raw.length || width > raw.length - at) throw new RangeError('Meltdown 패킷 읽기 범위');
    let result = 0;
    // 낮은 바이트부터 원문 폭을 읽는다.
    for (let byte = 0; byte < width; byte += 1) result += raw[at + byte] * 2 ** (8 * byte);
    return result;
}

// 패킷의 지정 폭만 쓰며 0xff 채움을 보존한다(범위 밖은 거부). (2026-11-08 WB04-E111)
function put(raw, at, value, width = 4) {
    if (at > raw.length || width > raw.length - at) throw new RangeError('Meltdown 패킷 쓰기 범위');
    // 원문의 little endian 이다.
    for (let byte = 0; byte < width; byte += 1) raw[at + byte] = (value >>> (byte * 8)) & 0xff;
}

// 실제 df590/df230 의 헤더 폭이다: byte0 = protocol&0x7f, byte1 = message, WORD = max(4, length). (2026-11-08 WB04-E111)
function header(packet, value) {
    put(packet, 0, value.protocol & 0x7f, 1);
    put(packet, 1, value.message, 1);
    put(packet, 2, Math.max(4, value.length), 2);
}

// 단정도 float 의 비트(uint32)이다. (2026-11-08 WB04-E111)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 41d770/41d7a0 의 _ftol 하위 DWORD 를 float 로 저장한다(upper 면 올림 편향). (2026-11-08 WB04-E111)
function snap(value, upper) {
    const adjusted = value + (upper ? SNAP_UPPER : 0);
    const low = (!Number.isFinite(adjusted) || adjusted < -INT64_BOUNDARY || adjusted >= INT64_BOUNDARY) ? 0 : Number(BigInt.asIntN(32, BigInt(Math.trunc(adjusted))));
    return fround(low);
}

// 0041d0d0 의 하한 1·상한 255 제한(패치는 NaN 을 보존한다; RawGunImpact::Clamp 와 같은 식이며 해당 모듈 이식 때 옮긴다). (2026-11-08 WB04-E111)
function clampPoint(x, y) {
    let a = x < 1 ? 1 : x;
    let b = y < 1 ? 1 : y;
    if (a > 255) a = 255;
    if (b > 255) b = 255;
    return [a, b];
}

// 제네레이터 Meltdown 처리기이다. (2026-11-08 WB04-E111)
export class RawMeltdown {
    // 같은 세계의 유일한 물리 객체 저장소이다. (2026-11-08 WB04-E111)
    #pool;
    // 검색의 네 단계 공간 해시이다. (2026-11-08 WB04-E111)
    #hash;
    // 현재 188 TYPE 표이다. (2026-11-08 WB04-E111)
    #types;
    // 보호 프레임 코드를 읽는 188칸 프레임 표이다. (2026-11-08 WB04-E111)
    #frames;
    // 선택 시점의 실제 전역(권한·로컬 owner·동맹·패킷 헤더)이다. (2026-11-08 WB04-E111)
    #state;
    // 선택 시점의 큰 외부 효과이다. (2026-11-08 WB04-E111)
    #hooks;

    // 현재 188 TYPE/프레임과 동맹 81칸만 받아 다른 판본 물리를 혼합하지 않는다. (2026-11-08 WB04-E111)
    constructor(pool, hash, types, frames, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !frames || frames.length !== TYPE_COUNT || !state?.alliances || state.alliances.length !== 81) {
            throw new Error('Meltdown 현재10.78 물리/TYPE/동맹 계약');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 비정렬 raw 필드를 지정 폭으로 읽는다(free/dead 필터는 각 원문 소비자에 맡긴다). (2026-11-08 WB04-E111)
    #read(sid, offset, width = 4) { return get(this.#pool.slot(sid), offset, width); }

    // 현재 좌표 float 이다. (2026-11-08 WB04-E111)
    #coordinate(sid, offset) {
        const raw = this.#pool.slot(sid);
        return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
    }

    // 효과 뒤 현재 TYPE 을 다시 읽는다. (2026-11-08 WB04-E111)
    #type(sid) {
        const type = this.#read(sid, TYPE_AT, 1);
        if (type >= this.#types.length) throw new RangeError('Meltdown 현재 TYPE 범위');
        return this.#types[type];
    }

    // 4adea0 의 현재 좌표−발자국 절반(float 저장)이다. (2026-11-08 WB04-E111)
    #center(sid) {
        const type = this.#type(sid);
        return [fround(this.#coordinate(sid, X_AT) - type.footX * 0.5), fround(this.#coordinate(sid, Y_AT) - type.footY * 0.5)];
    }

    // 41c290 은 f8 요청 뒤에도 공통 부모를 호출하며 원문 반환값은 void 이다. (2026-11-08 WB04-E111)
    command(parent, code, base) {
        if (((code & COMMAND_MASK) >>> 0) === COMMAND) {
            const packet = new Uint8Array(6);
            header(packet, this.#state.request);
            put(packet, 4, parent, 2);
            need(this.#hooks.send)(9, packet);
        }
        need(base)(parent, code);
    }

    // 420340: 다리는 보호 프레임 비트 0x40 만 제외한다. 다른 대상은 HP 값이 아니라 HP 보유 여부를 사용한다. (2026-11-08 WB04-E111)
    mayTarget(target) {
        const genus = this.#type(target).flags2 >>> 0;
        if (genus & 4) {
            if (!target) return true;
            const frame = this.#read(target, FRAME_AT) | 0;
            const codes = this.#frames[this.#read(target, TYPE_AT, 1)].codes;
            if (frame < 0 || frame >= codes.length) throw new RangeError('Meltdown 보호 프레임 범위');
            return !(codes[frame].flags & 0x40);
        }
        return Boolean(need(this.#hooks.hasHitPoints)(target)) && (genus & 0x80074200) !== 0;
    }

    // 4203a0: 원문은 후보의 dead 를 여기서 거르지 않으며 거리의 등호는 포함하지 않는다. (2026-11-08 WB04-E111)
    find(x, y, radius, output) {
        if (!need(this.#state.boss)()) need(this.#hooks.assertion)(0x664);
        const r = fround(radius);
        const low = clampPoint(fround(x - r), fround(y - r));
        const high = clampPoint(fround(r + x), fround(r + y));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const area = { left: Math.trunc(snap(low[0], false)), top: Math.trunc(snap(low[1], false)), right: Math.trunc(snap(high[0], true)), bottom: Math.trunc(snap(high[1], true)) };
        // 같은 네 단계 해시의 실제 순서를 따른다.
        for (let sid = finder.begin(area); sid; sid = finder.next()) {
            if (sid >= 5 && sid < 15000) continue;
            const limit = fround(this.#type(sid).footX * 0.5 + r);
            if (this.#read(sid, EXTRA_AT, 1) & 1) continue;
            const center = this.#center(sid);
            const dx = x - center[0];
            const dy = y - center[1];
            if (!(Math.sqrt(dx * dx + dy * dy) < limit) || !this.mayTarget(sid)) continue;
            if (output.count < output.capacity) {
                if (output.count < 0 || output.count >= output.storage.length) throw new RangeError('Meltdown 실제 대상 버퍼 범위');
                output.storage[output.count] = sid;
                output.count += 1;
            }
        }
    }

    // 4204f0: 피해/삭제 뒤 현재 슬롯을 다시 읽지 않고 원문처럼 먼저 캡처한 중심을 지면 효과에 사용한다. (2026-11-08 WB04-E111)
    apply(x, y, radius, owner, target, parent) {
        const hooks = this.#hooks;
        if (this.#read(target, STATE_AT, 1) & 2) return;
        const genus = this.#type(target).flags2 >>> 0;
        const center = this.#center(target);
        if (!(genus & 4)) {
            const victim = this.#read(target, OWNER_AT, 1);
            if (owner >= 9 || victim >= 9) throw new RangeError('Meltdown 동맹 owner 범위');
            if (!this.#state.alliances[owner * 9 + victim]) {
                need(hooks.damage)(target, 100, owner, parent);
                need(hooks.ground)(center[0], center[1]);
            }
            return;
        }
        // 원문 fiadd 는 정수 반경을 먼저 float 로 축소하지 않는다.
        const sum = fround(this.#type(target).footX * 0.5 + radius);
        const dx = x - center[0];
        const dy = y - center[1];
        if (Math.sqrt(dx * dx + dy * dy) <= sum - 1.0) { need(hooks.destroy)(target, 0x100048); return; }
        if (need(this.#state.boss)()) need(hooks.edge)(target, x, y);
    }

    // 4207a0: 검색은 최초 중심으로 하고 패킷에는 검색 뒤 현재 부모의 중심/owner 를 다시 읽어 넣는다. (2026-11-08 WB04-E111)
    receiveRequest(packet) {
        const hooks = this.#hooks;
        const parent = get(packet, 4, 2);
        if (this.#read(parent, STATE_AT, 1) & 2) return;
        if (!need(this.#state.boss)()) need(hooks.assertion)(0x6c3);
        const storage = new Array(100).fill(0);
        const list = { storage, capacity: 100, count: 0 };
        const initial = this.#center(parent);
        need(hooks.find)(initial[0], initial[1], REQUEST_RADIUS, list);
        const effect = new Uint8Array(EFFECT_BYTES).fill(255);
        header(effect, this.#state.effect);
        const center = this.#center(parent);
        put(effect, 4, floatBits(center[0]));
        put(effect, 8, floatBits(center[1]));
        put(effect, 12, REQUEST_RADIUS, 1);
        put(effect, 13, this.#read(parent, OWNER_AT, 1), 1);
        put(effect, 14, parent, 2);
        put(effect, 16, list.count >>> 0);
        // 디컴파일에서 사라진 실제 4208b8 의 WORD 복사 루프이다.
        for (let index = 0; index < list.count; index += 1) {
            if (index >= storage.length) throw new RangeError('Meltdown 검색 뒤 대상 수 범위');
            put(effect, 20 + index * 2, storage[index], 2);
        }
        need(hooks.broadcast)(effect);
        need(hooks.receive)(need(this.#state.localOwner)(), 0, effect);
        need(hooks.destroy)(parent, 0);
    }

    // 4205f0: 시각 효과 세 개와 소리를 먼저 실행하며 각 대상과 수/owner/반경은 원문 위치에서 다시 읽는다. (2026-11-08 WB04-E111)
    receiveEffect(packet) {
        const hooks = this.#hooks;
        const x = fround(snap(new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getFloat32(4, true), true) - 0.5);
        const y = fround(snap(new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getFloat32(8, true), true) - 0.5);
        const first = clampPoint(fround(x - get(packet, 12, 1)), fround(y - get(packet, 12, 1)));
        need(hooks.visual)(first[0], first[1], 1, VISUAL_INTERVAL);
        const second = clampPoint(fround(get(packet, 12, 1) + x), y);
        need(hooks.visual)(second[0], second[1], 1, VISUAL_INTERVAL);
        const third = clampPoint(x, fround(get(packet, 12, 1) + y));
        need(hooks.visual)(third[0], third[1], 1, VISUAL_INTERVAL);
        need(hooks.sound)('whirligigFire-1000.wav');
        // 효과 뒤 바뀐 현재 목록 수도 원문처럼 다시 읽는다.
        for (let index = 0; index < (get(packet, 16) | 0); index += 1) {
            const target = get(packet, 20 + index * 2, 2);
            if (this.#read(target, STATE_AT, 1) & 3) continue;
            this.apply(x, y, get(packet, 12, 1), get(packet, 13, 1), target, get(packet, 14, 2));
        }
    }

    // 40b150: 원본은 쓰기 요청일 때만 현재 count 로 WORD 길이를 갱신하며 count=-1 을 진단한다. (2026-11-08 WB04-E111)
    static finalizePacket(packet, writing) {
        if (!writing) return;
        const count = get(packet, 16);
        if (count === 0xffffffff) throw new Error('원본 Meltdown count 진단b23');
        put(packet, 2, (count + 10) * 2, 2);
    }
}
