// Evilray.cpp 의 실제 프레임·방출 기한·음향·표시 영역과 세 원형 타원 출력 요청을 옮긴 모듈이다(cpppj `o/RawEvilRayFrame.cpp/.h`, 10.78 전용: 00453290 프레임·00453540 출력).
//  필드는 `raw-evil-ray-lifetime.js` 의 `createEvilRayFields()` 를 쓴다. 실제 입자 저장소를 갱신하며 별도의 게임 모형을 만들지 않는다.
//  - new RawEvilRayFrame(pool, state, colors, hooks): pool 은 source/부모의 현재 raw 를 가진 같은 PATCH SidPool, state = { now, pixelScaleX(16), pixelScaleY(11), rate(1), cameraX, cameraY } 는 같은 Kernel 의 현재 전역,
//    colors 는 공유 3색(수명 모듈의 `buildEvilRayPalette` 결과)을 출력 때 읽는 살아 있는 배열 또는 그 배열을 돌려주는 함수이다.
//    hooks 는 **도달한 위치에서만** 요구한다(미연결이면 그때 오류): shapeWidth(sid)→signed 16비트 SHP 폭, soundId(sid, end)→음향 번호(false 는 현재 TYPE+70, true 는 +74), sound(x, y, id) 위치 재생,
//    invalidate({ left, top, right, bottom }) dirty, kill(flags) 자기 삭제(0041bf10, flags 0), clip([0, 0, 639, 479]) 출력 경계, circle({ x, y, radius, color }) 원형 타원 요청(래스터 wrapper 가 세로 반경을 radius−1 로 씀),
//    assertion(식, 파일, 줄) source/부모/입자 수 진단의 정상 복귀 장치이다.
//  - step(fields): source 진단 → 현재 폭/좌표 → 최초 시계 준비 → 전체 delta 한 번(clamp/시간 조각 없음) → 기한이면 시작 음향과 끝 꼬리 방출(죽은 슬롯 재사용 없음) →
//    사용 꼬리의 양수 수명 입자 이동과 ±8 영역, 이번에 수명이 0 이하가 된 입자는 부모 위치 끝 음향 → 활성 입자가 없고 중단 중이면 kill, 아니면 이전/새 영역 dirty 이다.
//  - draw(fields): 꼬리/용량 진단 → clip → 사용 꼬리의 양수 수명 입자마다 반경 max(1, (13−수명)×8/13) 의 세 색 타원(반경 −2·−1·0, 색 2·1·0 순서)을 요청한다.
//  정수화는 원본 __ftol 처럼 signed64 절삭 뒤 하위 DWORD 이고 비유한/범위 밖은 0 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E190: EvilRay 프레임 이식으로 추가)

// PE 00501450/508ed0/508f00/508ef8 의 시계 배율·방출 간격·입자 수명·반경 배율이다. (2026-11-09 WB04-E190)
const CLOCK_SCALE = 10;
const EMISSION_INTERVAL = 15;
const LIFE = 13;
const RADIUS_SCALE = 8;
// 508ef0 의 반경 단위 binary64(0x3fb3b13b13b13b14 = 1/13)이다. (2026-11-09 WB04-E190)
const RADIUS_UNIT = (() => {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, 0x3fb3b13b13b13b14n, true);
    return view.getFloat64(0, true);
})();
// 새 입자의 세로 속도·영역 여백·출력 clip(양끝 포함)이다. (2026-11-09 WB04-E190)
const VELOCITY_Y = 3;
const MARGIN = 8;
const CLIP = Object.freeze([0, 0, 639, 479]);
// 진단 경계가 받는 원문 파일 이름과 __ftol 불확정 경계이다. (2026-11-09 WB04-E190)
const SOURCE_FILE = '.\\Evilray.cpp';
const PARENT_FILE = '\\Ns\\O\\Baseprocess.cpp';
const TWO_63 = 9223372036854775808;

// 실제 __ftol 의 signed64 결과 하위 DWORD 이다(ToInt32 가 절삭과 2^32 나머지를 같이 수행). 비유한/범위 밖은 0 이다. (2026-11-09 WB04-E190)
function integer(value) {
    if (!Number.isFinite(value) || value >= TWO_63 || value < -TWO_63) return 0;
    return value | 0;
}

// 비정렬 raw float 의 모든 비트를 현재 물리 사본에서 읽는다. (2026-11-09 WB04-E190)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 현재 선택 callable 을 고른다(호출 중 연결이 바뀌어도 이번 호출은 같은 함수를 쓴다). 미연결이면 오류이다. (2026-11-09 WB04-E190)
function need(fn) {
    if (typeof fn !== 'function') throw new Error('EvilRay 현재 선택 장치 미연결');
    return fn;
}

// EvilRay 프레임/출력 몸체이다. 현재 시계/카메라/색은 호출보다 오래 살아야 한다. (2026-11-09 WB04-E190)
export class RawEvilRayFrame {
    #pool;
    #state;
    #colors;
    #hooks;

    // 같은 PATCH 물리 풀을 빌리고 장치를 미리 실행하지 않는다. (2026-11-09 WB04-E190)
    constructor(pool, state, colors, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !state || !hooks) throw new TypeError('EvilRay 프레임 판본 오류');
        this.#pool = pool;
        this.#state = state;
        this.#colors = colors ?? [];
        this.#hooks = hooks;
    }

    // 선택 진단 callable 을 호출한다. 정상 복귀 뒤 호출자는 현재 값을 계속 읽는다. (2026-11-09 WB04-E190)
    #diagnose(expression, line) {
        need(this.#hooks.assertion)(expression, SOURCE_FILE, line);
    }

    // 전체 DWORD 범위를 먼저 확인해 SID 절삭을 막고 현재 물리 슬롯 사본을 읽는다. 원문 32비트 포인터 감김/호스트 밖 메모리는 명시 오류다. (2026-11-09 WB04-E190)
    #physical(sid) {
        if (sid >= this.#pool.capacity || sid > 65535) throw new RangeError('EvilRay 현재 SID 물리 범위');
        return this.#pool.slot(sid);
    }

    // 공통 487ba0 의 여유/카메라/정수화 요청이다. (2026-11-09 WB04-E190)
    #invalidate(left, top, right, bottom) {
        const state = this.#state;
        need(this.#hooks.invalidate)({ left: integer((left - 1) - state.cameraX), top: integer((top - 1) - state.cameraY), right: integer((right + 1) - state.cameraX), bottom: integer((bottom + 1) - state.cameraY) });
    }

    // source 번호는 진단 전에 저장하고 현재 TYPE/좌표는 진단·callback 뒤 읽는다. 원본: 00453290. (2026-11-09 WB04-E190)
    step(fields) {
        const pool = this.#pool;
        const state = this.#state;
        const hooks = this.#hooks;
        const source = fields.source >>> 0;
        if (!source || source >= pool.capacity) this.#diagnose('sourceSquid.isValid()', 0x55);
        this.#physical(source);
        // 현재 가상 프레임의 signed 폭을 먼저 묻고, 그 뒤 현재 raw 좌표를 읽는다.
        const width = (need(hooks.shapeWidth)(source) << 16) >> 16;
        let raw = pool.slot(source);
        const x = Math.fround(state.pixelScaleX * readFloat(raw, 14) - ((width / 2) | 0));
        const y = Math.fround(state.pixelScaleY * readFloat(raw, 18));
        let left = x;
        let top = y;
        let right = x;
        let bottom = y;
        if (!fields.first) {
            fields.lastTime = state.now;
            fields.elapsed = 0;
            fields.first = true;
        }
        // 418454 는 기존 누적 double 에 저장한 delta 를 더하며 clamp/시간 조각 반복이 없다.
        const difference = (state.now - fields.lastTime) * CLOCK_SCALE;
        fields.delta = Math.fround(difference);
        fields.scaledDelta = Math.fround(difference * state.rate);
        fields.lastTime = state.now;
        fields.elapsed += fields.delta;
        if (fields.nextEmission <= fields.elapsed && !fields.stopping) {
            // 최초 source +70 조회 뒤 0 일 때만 현재 부모 +70 을 읽는다.
            let sound = need(hooks.soundId)(source, false) >>> 0;
            if (!sound) {
                const parent = fields.registration.parent >>> 0;
                const parentRaw = this.#physical(parent);
                if (!parentRaw[6] && !parentRaw[7]) need(hooks.assertion)('parent->getHead() != 0', PARENT_FILE, 0xa4);
                sound = need(hooks.soundId)(parent, false) >>> 0;
            }
            // 음향 번호 callback 뒤 현재 source 좌표를 다시 읽는다.
            raw = pool.slot(source);
            need(hooks.sound)(readFloat(raw, 14), readFloat(raw, 18), sound);
            // 원문 487b20 은 끝에만 붙이고 죽은 슬롯을 재사용하지 않는다.
            if (fields.total < fields.particles.length) {
                if (fields.total < 0) throw new RangeError('EvilRay 현재 입자 꼬리 물리 범위');
                const particle = fields.particles[fields.total];
                fields.total += 1;
                particle.velocityY = VELOCITY_Y;
                particle.x = x;
                particle.y = y;
                particle.life = LIFE;
            }
            fields.nextEmission = Math.fround(fields.elapsed + EMISSION_INTERVAL);
        }
        let active = false;
        // 실제 사용 꼬리는 음향 callback 뒤 현재 값으로 다시 읽는다. 이번에 죽은 입자도 이번 호출에서는 활성으로 센다.
        for (let index = 0; index < fields.total; index += 1) {
            if (index >= fields.particles.length) throw new RangeError('EvilRay 입자 용량 밖 조회');
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            particle.life = Math.fround(particle.life - fields.delta);
            particle.x = Math.fround(particle.velocityX * fields.scaledDelta + particle.x);
            // y 는 저장 전 넓은 값을 정수화에 넘긴다.
            const wideY = particle.velocityY * fields.scaledDelta + particle.y;
            particle.y = Math.fround(wideY);
            const px = integer(particle.x);
            const py = integer(wideY);
            // 먼저 int32 로 감긴 뒤 float 사각형에 넣는다.
            const lowX = Math.fround((px - MARGIN) | 0);
            const lowY = Math.fround((py - MARGIN) | 0);
            const highX = Math.fround((px + MARGIN) | 0);
            const highY = Math.fround((py + MARGIN) | 0);
            left = left < lowX ? left : lowX;
            top = top < lowY ? top : lowY;
            right = highX < right ? right : highX;
            if (bottom <= highY) bottom = highY;
            active = true;
            if (particle.life <= 0) {
                // 원문은 메타데이터 호출 앞에 현재 부모 좌표를 저장한다.
                const parent = fields.registration.parent >>> 0;
                const parentRaw = this.#physical(parent);
                const parentX = readFloat(parentRaw, 14);
                const parentY = readFloat(parentRaw, 18);
                const end = need(hooks.soundId)(parent, true) >>> 0;
                need(hooks.sound)(parentX, parentY, end);
            }
        }
        if (!active && fields.stopping) {
            need(hooks.kill)(0);
            return;
        }
        const rect = fields.registration;
        this.#invalidate(rect.left, rect.top, rect.right, rect.bottom);
        this.#invalidate(left, top, right, bottom);
        rect.left = left;
        rect.top = top;
        rect.right = right;
        rect.bottom = bottom;
    }

    // 원문 진단/clip 뒤에만 현재 사용 꼬리와 색을 읽는다. 원본: 00453540. (2026-11-09 WB04-E190)
    draw(fields) {
        const state = this.#state;
        const hooks = this.#hooks;
        if (fields.total > fields.particles.length) this.#diagnose('totalParticles <= maxParticles', 0x8c);
        need(hooks.clip)(CLIP.slice());
        // 용량 밖은 앞선 진단/clip 뒤 명시 물리 경계로 드러난다.
        for (let index = 0; index < fields.total; index += 1) {
            if (index >= fields.particles.length) throw new RangeError('EvilRay 입자 용량 밖 조회');
            const particle = fields.particles[index];
            if (!(particle.life > 0)) continue;
            // 원문 두 좌표 정수화와 두 double 곱의 순서이다.
            const x = Math.fround(integer(particle.x) - state.cameraX);
            const y = integer(integer(particle.y) - state.cameraY);
            const radius = Math.max(1, integer(((LIFE - particle.life) * RADIUS_SCALE) * RADIUS_UNIT));
            const colors = typeof this.#colors === 'function' ? this.#colors() : this.#colors;
            if (!colors || colors.length < 3) throw new RangeError('EvilRay 현재 공유 3색 미연결');
            need(hooks.circle)({ x: integer(x), y, radius: (radius - 2) | 0, color: colors[2] >>> 0 });
            need(hooks.circle)({ x: integer(x), y, radius: (radius - 1) | 0, color: colors[1] >>> 0 });
            need(hooks.circle)({ x: integer(x), y, radius, color: colors[0] >>> 0 });
        }
    }
}
