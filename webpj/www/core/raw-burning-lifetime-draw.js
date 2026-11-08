// 실제 Burning 출력·소멸과 RenderProcess 의 프레임 진입 판정, 궁수 캡처 그림 제출을 옮긴 모듈이다(cpppj `o/RawBurningLifetimeDraw.cpp/.h`, 10.78 전용: 423ec0 입자 출력·423cd0/4240f0 소멸·487940 프레임 진입 판정·71/128 의 +a4 캡처 그림).
//  - drawBurningVisual(fields, state, palette, pixel): 별도 source 입자는 그리지 않고 양수 수명(ordered `life > 0`)의 이동 입자만 사용 수(total, capacity 가 아님)까지 순회한다. 각 입자는 좌표를 먼저 0 방향으로 정수화 → 카메라를 빼 float 에 저장 → 색을 조회(수명 ≤ 9 로 제한한 값 × 색수 ÷ 9 의 x87 곱, 0 이상으로 제한, 인접 DWORD 를 읽을 수 있음)한 뒤
//    두 번째 정수화를 하고, 현재 dirty clip 의 5점(x, y)·(x+1, y)·(x+1, y−1)·(x, y−1)·(x, y−2)이 모두 clip 안(좌/상 이상, 우/하 미만)일 때만 `pixel(x, y, color)` 를 그 순서로 호출한다. state = { cameraX, cameraY, viewport: { left, top, right, bottom } } 는 전체 viewport 로 확대하지 않는 현재 clip 이다.
//    palette = { count(원본 공유 표의 요청 색수 8), readable(같은 실제 할당의 유효 snapshot DWORD 배열, 임의 끝색을 추가하지 않음) } 이며 pixel 의 color 는 DWORD 의 하위 BYTE 이다.
//  - destroyBurningVisual(fields, registry, flags, release, stopSound): 423cd0/4240f0 — 캡처 이미지 → 행 표식 → (소리 핸들 stopSound) → 입자 버퍼 순으로 존재할 때만 `release(BurningRelease.*)` 하고 비운 뒤 표시 등록 제거(base 소멸) → flags bit 0 이면 객체 반납 순서이다. 공유 팔레트는 월드 소유라 반납하지 않는다.
//  - renderProcessFrameDue(slot, frame): 실제 487940 은 슬롯이 7 미만(signed)이거나 (slot XOR frame)의 하위 2비트가 0 일 때만 파생 프레임(+24)으로 간다.
//  - isBurningArcherCapture(raw)/drawBurningArcherCapture(raw, base, muzzleX, muzzleY, draw, x, y): 복원한 71(발사기)/128(캐논)의 실제 두 가상 표(0x50b888/0x50b9b8)와 TYPE 를 함께 확인하고, 캐논은 그림을 제출하지 않고 발사기는 출력 WORD bit 0·extra(+40 의 0x9 비트)·현재 프레임(+36)이 기본 프레임과 같은지에 따라
//    `draw(71, 현재 프레임, x, y)` 와 조건부 포구 디스크 `draw(83, 0, x + muzzleX, y + muzzleY)` 를 순서대로 제출한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E184: Burning 출력·소멸·프레임 판정 이식으로 추가)

// 원본 실제 소멸자가 반납하는 순서별 소유 버퍼 종류이다. (2026-11-09 WB04-E184)
export const BurningRelease = Object.freeze({ Image: 'Image', Rows: 'Rows', Particles: 'Particles', Object: 'Object' });

// 71(발사기)/128(캐논)의 실제 보호 가상 표 기록값과 그림 제출 종류(발사기 71·포구 디스크 83)이다. (2026-11-09 WB04-E184)
const ARCHER_VTABLE = 0x50b888;
const CANNON_VTABLE = 0x50b9b8;
const ARCHER_TYPE = 71;
const CANNON_TYPE = 128;
const MUZZLE_DISK = 83;
const TWO_31 = 2147483648;

// 실제 CRT 의 정상 화면 좌표에서 0 방향 절삭한다. 비유한/범위 밖은 호스트 계약 오류이다. (2026-11-09 WB04-E184)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_31 || value >= TWO_31) throw new RangeError('Burning Draw 원본 정수화 범위 오류');
    return Math.trunc(value) | 0;
}

// 원본 signed DWORD 덧셈의 감김이다. (2026-11-09 WB04-E184)
const add = (value, amount) => (value + amount) | 0;

// 실제 487e50 의 두 float 저장 뒤 넓은 곱이며 수명 9 에서는 인접 DWORD 8 을 읽을 수 있다(색 수 × min(수명, 9) × float(1/9)). (2026-11-09 WB04-E184)
function color(life, palette) {
    if (palette.count < 0) throw new RangeError('Burning Draw 음수 색수');
    const reciprocal = Math.fround(1.0 / 9.0);
    const index = Math.max(0, truncate(Math.fround(palette.count) * (life < 9 ? life : 9) * reciprocal));
    if (index >= palette.readable.length) throw new RangeError('Burning Draw 인접 색 DWORD 미공급');
    return palette.readable[index] & 0xff;
}

// 비정렬 DWORD 를 읽는다. (2026-11-09 WB04-E184)
const dword = (raw, at) => (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;

// 보호 가상 표 기록값과 실제 타입을 함께 확인한다. (2026-11-09 WB04-E184)
export function isBurningArcherCapture(raw) {
    const table = dword(raw, 0);
    return (raw[10] === ARCHER_TYPE && table === ARCHER_VTABLE) || (raw[10] === CANNON_TYPE && table === CANNON_VTABLE);
}

// +78 은 TYPE+128 이며 캐논의 +a4 는 ret8 이다. 발사기는 현재 그림 뒤 출력 WORD bit 0 이 꺼졌을 때만 포구 디스크를 더한다. (2026-11-09 WB04-E184)
export function drawBurningArcherCapture(raw, base, muzzleX, muzzleY, draw, x, y) {
    if (!isBurningArcherCapture(raw) || typeof draw !== 'function') throw new TypeError('연소 캡처의 다른 발사기/그림 경계');
    if (raw[10] === CANNON_TYPE) return;
    const current = dword(raw, 36);
    if (raw[40] & 9) {
        draw(ARCHER_TYPE, current, x, y);
        return;
    }
    if (current === (base >>> 0)) return;
    draw(ARCHER_TYPE, current, x, y);
    if (!(raw[12] & 1)) draw(MUZZLE_DISK, 0, add(x, muzzleX), add(y, muzzleY));
}

// 등록된 현재 slot 과 원본 현재 DWORD 프레임만 사용한다(signed slot 비교, XOR 은 unsigned 비트). (2026-11-09 WB04-E184)
export function renderProcessFrameDue(slot, frame) {
    return (slot | 0) < 7 || ((((slot | 0) >>> 0) ^ (frame >>> 0)) & 3) === 0;
}

// 별도 source 입자는 그리지 않고 양수 수명의 이동 입자만 사용 수까지 순회한다. (2026-11-09 WB04-E184)
export function drawBurningVisual(fields, state, palette, pixel) {
    if (!fields.capacity) return;
    if (fields.total > fields.capacity || fields.particles.length !== fields.capacity || typeof pixel !== 'function') throw new RangeError('Burning Draw 실제 입자/출력 연결 오류');
    // 원본 VFX 의 현재 dirty clip 이며 전체 viewport 로 확대하지 않는다.
    const clip = state.viewport;
    // 원본은 capacity 대신 현재 total 을 순회한다.
    for (let index = 0; index < fields.total; index += 1) {
        const particle = fields.particles[index];
        if (!(particle.life > 0) || particle.source) continue;
        // 먼저 좌표 정수화 → 카메라 감산 → float 저장이다.
        const fx = Math.fround(truncate(particle.x) - state.cameraX);
        const fy = Math.fround(truncate(particle.y) - state.cameraY);
        // 색 조회 뒤 두 번째 정수화를 수행한다.
        const pickedColor = color(particle.life, palette);
        const x = truncate(fx);
        const y = truncate(fy);
        const right = add(x, 1);
        const top = add(y, -2);
        if (x < clip.left || x >= clip.right || y < clip.top || y >= clip.bottom || right < clip.left || right >= clip.right || top < clip.top || top >= clip.bottom) continue;
        pixel(x, y, pickedColor);
        pixel(right, y, pickedColor);
        pixel(right, add(y, -1), pickedColor);
        pixel(x, add(y, -1), pickedColor);
        pixel(x, top, pickedColor);
    }
}

// 공유 팔레트는 월드 소유라 이 소멸자가 반납하지 않는다(캡처 → 행 → 소리 → 입자 → 표시 등록 제거 → 객체 순서). (2026-11-09 WB04-E184)
export function destroyBurningVisual(fields, registry, flags, release, stopSound) {
    if (typeof release !== 'function' || typeof stopSound !== 'function') throw new TypeError('Burning 소멸 실제 반납/소리 경계 누락');
    if (fields.image.pixels.length) {
        release(BurningRelease.Image);
        fields.image.pixels = [];
    }
    if (fields.rows.length) {
        release(BurningRelease.Rows);
        fields.rows = [];
    }
    stopSound(fields.sound);
    if (fields.particles.length) {
        release(BurningRelease.Particles);
        fields.particles = [];
    }
    registry.destroy(fields.registration);
    if (flags & 1) release(BurningRelease.Object);
}
