// 애시드 포획 Dissolve(분해) 효과의 생성(00450510/004881d0)과 소유권 반납(0044fd80/00423aa0)을 옮긴 모듈이다(cpppj `o/RawDissolveLifetime.cpp/.h`, 10.78 전용).
//  - new RawDissolveLifetime(pool, registry, hooks): registry 는 같은 풀의 `RawRenderRegistry`, hooks 는 모두 필수이다.
//    reserve(바이트)→성공 여부(실제 120바이트 객체 요청), create(type, flags2)→더미 SID(TYPE 110/flags 2), pop(dummy, x, y, flags)(생성 뒤 현재 source 좌표에 flags 16 으로 배치),
//    attach(registration, processType, dummy, flags)(등록 뒤 process 18 부착, form/parent 를 채움), depend(form, dummy)(Form 과 더미의 상호 감시),
//    capture(source)→`{ width, height, hotspotX, hotspotY, pixels }`(현재 대상의 실제 원본 그림), prepare(fields, first, second, xOriented)(`RawDissolveFrame.initialize` 의 현재 부모/폴 입자 계산)이다.
//  - reserveObject(): 원본 120바이트 요청이다. 실패하면 생성/등록/더미 생성에 들어가지 않는다(원본 null 역참조는 재현하지 않음).
//  - initialize(fields, token, source, first, second, xOriented, dummy): fields 는 `createDissolveFrameFields()`, dummy 는 `{ value }` 상자(생성 직후 더미 SID)이다.
//    parent 0 등록(process 18) → 필드 초기화(입자 확보 여부 false) → 더미 생성/현재 source 좌표 Pop/부착/상호 감시 → 캡처 그림·불투명 수·시계 시작 초기화 → 캡처 → 준비 순서이며, 시계는 첫 프레임까지 시작하지 않는다.
//  - destroy(fields, flags, release): 캡처 그림(있을 때) → 입자(실제 확보했을 때, 용량 0 이어도) → 공통 등록 제거/base → flags bit 0 이면 객체 순서이다. release 는 `ShrapnelRelease` 값을 받는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E189: Dissolve 수명 이식으로 추가)
import { shrapnelFloatFromBits } from './raw-shrapnel-frame.js';
import { ShrapnelRelease } from './raw-shrapnel-lifetime.js';

// 원본 wrapper 의 객체 크기·process TYPE·더미 TYPE·생성 flags·부착 flags 이다. (2026-11-09 WB04-E189)
const OBJECT_BYTES = 120;
const PROCESS_TYPE = 18;
const DUMMY_TYPE = 110;
const CREATE_FLAGS = 2;
const ATTACH_FLAGS = 16;

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E189)
function readFloat(raw, offset) {
    return shrapnelFloatFromBits(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
}

// Dissolve 생성·소멸 몸체이다. 세계 경계의 누락은 등록/더미 생성을 시작하기 전에 거부한다. (2026-11-09 WB04-E189)
export class RawDissolveLifetime {
    #pool;
    #registry;
    #hooks;

    // 같은 PATCH 풀/표시 목록과 필수 경계를 확인한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E189)
    constructor(pool, registry, hooks) {
        const names = ['reserve', 'create', 'pop', 'attach', 'depend', 'capture', 'prepare'];
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || !hooks || names.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('분해 판본/실제 수명 경계 누락');
        this.#pool = pool;
        this.#registry = registry;
        this.#hooks = hooks;
    }

    // 원본 120바이트 요청이다. 실패는 정상 생성과 구별해 false 로 돌려준다. (2026-11-09 WB04-E189)
    reserveObject() {
        return Boolean(this.#hooks.reserve(OBJECT_BYTES));
    }

    // source 삭제 전에 캡처를 끝내고 첫 프레임까지 시계는 시작하지 않는다. 원본: 00450510·004881d0·00450460. (2026-11-09 WB04-E189)
    initialize(fields, token, source, first, second, xOriented, dummy) {
        if (!dummy || typeof dummy !== 'object') throw new TypeError('분해 더미 출력 상자 누락');
        const pool = this.#pool;
        const hooks = this.#hooks;
        const value = fields.common;
        // source 가 풀 밖이면 등록 전에 거부한다.
        pool.slot(source);
        this.#registry.construct(value.registration, token, 0, PROCESS_TYPE);
        value.particles = [];
        value.total = 0;
        value.lastTime = 0;
        value.elapsed = 0;
        value.first = false;
        fields.particlesAllocated = false;
        dummy.value = hooks.create(DUMMY_TYPE, CREATE_FLAGS);
        // 생성 callback 이 source 를 바꿨을 수 있으므로 현재 raw 를 다시 읽는다.
        const raw = pool.slot(source);
        hooks.pop(dummy.value, readFloat(raw, 14), readFloat(raw, 18), ATTACH_FLAGS);
        hooks.attach(value.registration, PROCESS_TYPE, dummy.value, ATTACH_FLAGS);
        const form = value.registration.form;
        if (!Number.isInteger(form) || form < 1 || form > 65535) throw new RangeError('분해 현재 Form 번호 오류');
        hooks.depend(form, dummy.value);
        value.image.pixels = new Uint8Array(0);
        fields.opaqueCount = 0;
        fields.clockStarted = false;
        value.image = hooks.capture(source);
        hooks.prepare(fields, first, second, xOriented);
    }

    // 투명 그림의 new 0 도 실제 비영 포인터이므로 입자 확보 여부를 따로 본다. 원본은 해제한 포인터/용량을 0 으로 다시 쓰지 않는다. 원본: 0044fd80/00423aa0. (2026-11-09 WB04-E189)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('분해 실제 반납 경계 누락');
        const value = fields.common;
        if (value.image.pixels.length) {
            release(ShrapnelRelease.Source);
            value.image.pixels = new Uint8Array(0);
        }
        if (fields.particlesAllocated) {
            release(ShrapnelRelease.Particles);
            value.particles = [];
            fields.particlesAllocated = false;
        }
        this.#registry.destroy(value.registration);
        if (flags & 1) release(ShrapnelRelease.Object);
    }
}
