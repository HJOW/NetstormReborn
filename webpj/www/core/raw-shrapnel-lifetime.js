// Flyingshrapnel.cpp 의 부모 없는 표시 등록 → 더미 생성/Pop/부착 → 캡처/준비와 세 파생 생성자, 공통 반납 순서를 옮긴 모듈이다(cpppj `o/RawShrapnelLifetime.cpp/.h`, 10.78 전용: 004604a0 wrapper 와 세 가상 생성자·공통 소멸).
//  - ShrapnelRelease: scalar deleting destructor 가 구별해 반납하는 세 대상(캡처 원본 그림·입자 저장소·객체 자체)이다.
//  - new RawShrapnelLifetime(pool, registry, hooks): registry 는 같은 풀의 `RawRenderRegistry`, hooks 는 모두 필수이다.
//    reserve(바이트)→성공 여부(wrapper 의 104바이트 요청), create(type, flags2)→더미 SID(실제 TYPE 110/flags 2 의 client 자산 생성), pop(dummy, x, y, flags)(현재 source 좌표의 더미 Pop),
//    attach(registration, processType, dummy, flags)(parent 0 등록 뒤 실제 process 16 부착, form/parent 를 채움), depend(form, dummy)(004b1510 상호 감시 쌍),
//    capture(source)→`{ width, height, hotspotX, hotspotY, pixels }`(현재 source 의 실제 그림 캡처), prepare(fields, kind)(같은 두 난수/시각의 `RawShrapnelFrame.prepare`)이다.
//  - reserveObject(): wrapper 의 new104 실패 분기이다. 실패하면 생성자·등록·더미 생성에 들어가지 않는다.
//  - initialize(fields, token, source, kind, dummy): kind 는 `ShrapnelKind` 번호, dummy 는 `{ value }` 상자이며 생성 직후 더미 SID 가 들어가므로 부착 전 예외의 소유권 정리에도 쓴다.
//    초기 깊이는 SID 0 에서 오고 최종 깊이는 캡처/준비 뒤 현재 source 의 깊이다. 좌표와 깊이는 callback 뒤 현재 raw 를 다시 읽는다.
//  - destroy(fields, flags, release): 캡처 그림 → 입자 → 공통 등록 제거/base → flags bit 0 이면 객체 순서이다. release 는 예외를 던지거나 등록을 바꾸지 않아야 한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E188: 파편 수명 이식으로 추가)
import { ShrapnelKind, shrapnelFloatFromBits } from './raw-shrapnel-frame.js';

// scalar deleting destructor 에서 구별하는 세 실제 반납 대상이다. (2026-11-09 WB04-E188)
export const ShrapnelRelease = Object.freeze({ Source: 'Source', Particles: 'Particles', Object: 'Object' });

// 보호 10.78 의 실제 객체 크기·process TYPE·더미 TYPE·생성 flags·부착 flags 이다. (2026-11-09 WB04-E188)
const OBJECT_BYTES = 104;
const PROCESS_TYPE = 16;
const DUMMY_TYPE = 110;
const CREATE_FLAGS = 2;
const ATTACH_FLAGS = 16;
// 세 가상 생성자가 저장하는 정확한 binary32 중력이다(폭발 6·붕괴 0.24·셋째 1.2). (2026-11-09 WB04-E188)
const GRAVITY_BITS = Object.freeze([0x40c00000, 0x3e75c28f, 0x3f99999a]);
// 패치 깊이 BYTE 의 raw 위치이다. (2026-11-09 WB04-E188)
const PATCH_Z = 35;

// 비정렬 raw float 를 실제 little endian DWORD 로 읽는다. (2026-11-09 WB04-E188)
function readFloat(raw, offset) {
    return shrapnelFloatFromBits(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
}

// 파편 생성·소멸 몸체이다. 다른 월드/판본과 실제 경계 누락은 등록 전에 거부한다. (2026-11-09 WB04-E188)
export class RawShrapnelLifetime {
    #pool;
    #registry;
    #hooks;

    // 같은 PATCH 풀/표시 목록과 모든 실제 경계를 확인한다(모든 참조는 이 객체보다 오래 살아야 함). (2026-11-09 WB04-E188)
    constructor(pool, registry, hooks) {
        const names = ['reserve', 'create', 'pop', 'attach', 'depend', 'capture', 'prepare'];
        if (pool?.edition !== 'patch1078' || registry?.pool !== pool || !hooks || names.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('파편 수명 판본/실제 경계 누락');
        this.#pool = pool;
        this.#registry = registry;
        this.#hooks = hooks;
    }

    // 원본 wrapper 의 new104 요청이다. 성공 뒤 호스트가 실제 객체를 확보한다. (2026-11-09 WB04-E188)
    reserveObject() {
        return Boolean(this.#hooks.reserve(OBJECT_BYTES));
    }

    // parent 0 등록 → 필드 초기화 → 더미 생성/Pop/부착/상호 감시 → 셋째 생성기 깊이+1 → 중력 → 캡처/준비 → 현재 source 깊이 순서이다. 원본: 004604a0 과 세 가상 생성자. (2026-11-09 WB04-E188)
    initialize(fields, token, source, kind, dummy) {
        if (kind !== ShrapnelKind.Explosion && kind !== ShrapnelKind.Collapse && kind !== ShrapnelKind.Variant2) throw new RangeError('파편 가상 생성기 범위 오류');
        if (!dummy || typeof dummy !== 'object') throw new TypeError('파편 더미 출력 상자 누락');
        const pool = this.#pool;
        const hooks = this.#hooks;
        // source 가 풀 밖이면 등록 전에 거부한다.
        pool.slot(source);
        this.#registry.construct(fields.registration, token, 0, PROCESS_TYPE);
        fields.particles = [];
        fields.total = 0;
        fields.lastTime = 0;
        fields.elapsed = 0;
        fields.first = false;
        dummy.value = hooks.create(DUMMY_TYPE, CREATE_FLAGS);
        // 생성 callback 이 source 를 바꿨을 수 있으므로 현재 raw 를 다시 읽는다.
        const raw = pool.slot(source);
        hooks.pop(dummy.value, readFloat(raw, 14), readFloat(raw, 18), ATTACH_FLAGS);
        hooks.attach(fields.registration, PROCESS_TYPE, dummy.value, ATTACH_FLAGS);
        const form = fields.registration.form;
        if (!Number.isInteger(form) || form < 1 || form > 65535) throw new RangeError('파편 현재 Form 번호 오류');
        hooks.depend(form, dummy.value);
        if (kind === ShrapnelKind.Variant2) fields.registration.zOrder = (fields.registration.zOrder + 1) | 0;
        fields.image.pixels = new Uint8Array(0);
        fields.gravity = shrapnelFloatFromBits(GRAVITY_BITS[kind]);
        fields.image = hooks.capture(source);
        hooks.prepare(fields, kind);
        fields.registration.zOrder = (pool.slot(source)[PATCH_Z] << 24) >> 24;
    }

    // 원본은 해제한 raw 포인터/용량을 0 으로 다시 쓰지 않는다. 호스트 배열 소유권만 실제 반납한다. 원본: 공통 scalar deleting destructor. (2026-11-09 WB04-E188)
    destroy(fields, flags, release) {
        if (typeof release !== 'function') throw new TypeError('파편 실제 반납 경계 누락');
        // 원본 포인터가 있는 저장소만 차례로 반납한다.
        if (fields.image.pixels.length) {
            release(ShrapnelRelease.Source);
            fields.image.pixels = new Uint8Array(0);
        }
        if (fields.particles.length) {
            release(ShrapnelRelease.Particles);
            fields.particles = [];
        }
        this.#registry.destroy(fields.registration);
        if (flags & 1) release(ShrapnelRelease.Object);
    }
}
