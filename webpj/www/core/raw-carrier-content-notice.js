// 패치 00429560: 사제/Walker/Carrier 의 내용물 가상 +24 통지 전체 분기를 옮긴 모듈이다(cpppj `o/RawCarrierContentNotice.cpp/.h`, 10.78 전용).
// 순서(각 원본 읽기 위치에서 현재 전역과 raw 를 다시 읽음): 저장 게임이면 전부 생략 → 로딩 중이면 전부 생략 → 내용물(form)이 dead 가 아니고 kind 의 genus 에 0x100(폭탄)이 있고 폭탄 억제가 꺼져 있으면 부모 중심점 두 번의 화면 투영으로 번개(5.0)와
//   thunderCrack.wav 를 내고 현재 부모 소유자가 로컬이면 40바이트를 확보해 내용물 생성(타입 +6c 추가 DWORD)과 1.5초 타이머를 설정 / 그렇지 않고 부모 소유자가 로컬이고 내용물이 현재 사제 종류이며 dead 가 아니면 타입 +7c 추가 DWORD 의 소리 →
//   내용물이 dead 가 아니고 로딩이 아니며 reason 비트 0 이 있고 +20 WORD 가 있고 genus 에 0x208100 이 있으면 Carrier 지면 처리 → reason 비트 1 이 있고 내용물이 사제 종류이면 현재 부모 위치에 genus 0x400000 객체가 없고 같은 위치에 rainWalker(현재 156) 타입이 없을 때 priestFree.wav 를 낸다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E74: Carrier 내용물 통지 이식으로 추가)

import { RawContentParentNotice } from './content-parent-notice.js';

// 이 몸체가 맡는 내용물 통지 원본 진입점, 슬롯의 TYPE/상태/소유자/kind/조건 WORD/좌표 위치, 내용물 genus 비트들, 투영 배율, 번개/타이머 상수, 사제 해방 genus 이다. (2026-11-08 WB04-E74)
const CARRIER_NOTICE_ENTRY = 0x00429560;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const KIND_OFFSET = 18;
const WORD20_OFFSET = 20;
const OWNER_OFFSET = 34;
const DEAD = 2;
const BOMB_GENUS = 0x100;
const GROUND_CHECK_GENUS = 0x208100;
const FREE_BLOCK_GENUS = 0x400000;
const SCALE_X = 16;
const SCALE_Y = 11;
const BOLT_TIME = 5.0;
const DURATION = 1.5;
const ALLOCATION_BYTES = 40;
const CREATE_ARGUMENTS = [0, 12];
const TYPE_COUNT = 188;
const BOMB_OFFSET = 0x6c;
const SOUND_OFFSET = 0x7c;

// 선택된 실제 효과 누락을 빈 성공으로 바꾸지 않는다. (2026-11-08 WB04-E74)
function required(hook) {
    if (typeof hook !== 'function') throw new Error('Carrier 내용물 통지 실제 효과 미연결');
    return hook;
}

// 원본 화면 투영의 CRT 하위 DWORD 와 카메라 뺄셈 감김을 유지한다. (2026-11-08 WB04-E74)
function projectCoordinate(coordinate, scale, camera) {
    const value = coordinate * scale + 0.5;
    let integer = 0n;
    if (Number.isFinite(value) && value >= -9223372036854775808 && value < 9223372036854775808) integer = BigInt(Math.trunc(value));
    return (Number(BigInt.asUintN(32, integer)) - (camera >>> 0)) | 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E74)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), state 는 현재 전역을 속성으로 읽는 객체 { savedGame, loadingDepth, bombBlocked, localOwner, priestType, cameraX, cameraY, freeSoundBlockerType }(getter 가능),
//   hooks = { center(부모)→[x, y], bolt(부모, [x, y], [x, y], 시간), soundAt(x, y, 파일), typeWord(타입, 오프셋)→DWORD, allocate(바이트)→토큰, create(토큰, 부모, 값, 0, 12)→토큰, duration(토큰, 초), sound(값), checkCarrier(부모),
//   genusAt(x, y, genus)→SID, typeAt(x, y, 타입)→SID, freeSound(파일) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E74)
export class RawCarrierContentNotice {
    // 실제 PATCH 50바이트 풀, 현재 kind 의 genus 를 읽는 타입 표, 원본 전역 참조, 외부 몸체, 부모 VT 의 실제 가상 진입점 조회기이다.
    #pool;
    #types;
    #state;
    #hooks;
    #dispatch;

    // 실제 판본과 현재 타입 표를 확인하며 생성 때 가상 효과를 실행하지 않는다. (2026-11-08 WB04-E74)
    constructor(pool, types, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('Carrier 내용물 통지 PATCH 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
        this.#dispatch = new RawContentParentNotice(pool);
    }

    // 현재 부모의 실제 VT 를 검사하므로 타입 번호만으로 파생 범위를 넓히지 않는다. 알 수 없는 VT 는 거부한다. (2026-11-08 WB04-E74)
    handles(parent) {
        return this.#dispatch.nativeEntry(parent) === CARRIER_NOTICE_ENTRY;
    }

    // 원본이 사용하는 필드 폭만 읽는다. (2026-11-08 WB04-E74)
    #word(sid, offset, width = 2) {
        const raw = this.#pool.slot(sid);
        let value = 0;
        // 비정렬 리틀엔디언 바이트를 조립한다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
        return value;
    }

    // 효과 뒤 바뀐 kind 와 타입 플래그를 매번 조회한다. (2026-11-08 WB04-E74)
    #contentType(form) {
        const type = this.#word(form, KIND_OFFSET);
        if (type >= this.#types.length) throw new RangeError('Carrier 내용물 kind 범위');
        return this.#types[type];
    }

    // 497220/41cc50 의 16/11 배·0.5·ftol·현재 원점 투영이다. (2026-11-08 WB04-E74)
    #project(point) {
        return [projectCoordinate(point[0], SCALE_X, this.#state.cameraX | 0), projectCoordinate(point[1], SCALE_Y, this.#state.cameraY | 0)];
    }

    // 해당 호출에 필요한 두 현재 float 의 비트다. (2026-11-08 WB04-E74)
    #position(parent) {
        return [floatOf(this.#word(parent, X_OFFSET, 4)), floatOf(this.#word(parent, Y_OFFSET, 4))];
    }

    // 원본 reason 의 하위 BYTE 비트를 유지하며 429560 전체의 분기를 실행한다. reason 4 를 이유로 나머지 조건을 통째로 생략하지 않는다. (2026-11-08 WB04-E74)
    notice(parent, form, reason) {
        const state = this.#state;
        const hooks = this.#hooks;
        if (state.savedGame) return;
        if (state.loadingDepth) return;
        if (!(this.#word(form, STATE_OFFSET, 1) & DEAD) && (this.#contentType(form).flags2 & BOMB_GENUS) && !state.bombBlocked) {
            const center = required(hooks.center)(parent);
            const first = this.#project(center);
            const second = this.#project(center);
            required(hooks.bolt)(parent, [first[0], 0], second, BOLT_TIME);
            const soundCenter = required(hooks.center)(parent);
            required(hooks.soundAt)(soundCenter[0], soundCenter[1], 'thunderCrack.wav');
            const value = required(hooks.typeWord)(this.#word(form, KIND_OFFSET), BOMB_OFFSET);
            if (this.#word(parent, OWNER_OFFSET, 1) === (state.localOwner >>> 0)) {
                const storage = required(hooks.allocate)(ALLOCATION_BYTES);
                let process = 0;
                if (storage) process = required(hooks.create)(storage, parent, value, CREATE_ARGUMENTS[0], CREATE_ARGUMENTS[1]);
                if (process) required(hooks.duration)(process, DURATION);
            }
        } else if (this.#word(parent, OWNER_OFFSET, 1) === (state.localOwner >>> 0) && this.#word(form, KIND_OFFSET) === (state.priestType >>> 0) && !(this.#word(form, STATE_OFFSET, 1) & DEAD)) {
            const sound = required(hooks.typeWord)(this.#word(parent, TYPE_OFFSET, 1), SOUND_OFFSET);
            required(hooks.sound)(sound);
        }
        if (!(this.#word(form, STATE_OFFSET, 1) & DEAD) && !state.loadingDepth && (reason & 1) && this.#word(form, WORD20_OFFSET) && (this.#contentType(form).flags2 & GROUND_CHECK_GENUS)) {
            required(hooks.checkCarrier)(parent);
        }
        if ((reason & 2) && this.#word(form, KIND_OFFSET) === (state.priestType >>> 0)) {
            const position = this.#position(parent);
            if (!required(hooks.genusAt)(position[0], position[1], FREE_BLOCK_GENUS)) {
                const current = this.#position(parent);
                if (!required(hooks.typeAt)(current[0], current[1], state.freeSoundBlockerType >>> 0)) required(hooks.freeSound)('priestFree.wav');
            }
        }
    }
}
