// 캠페인 발사/회전·Missile·Disc 예약을 같은 실제 World/Host에 조립한다(cpppj client/RawCampaignArcherBindings, 225553df).
// 사용: prepareFrame 뒤 gunHooks/handler/transmission/receiveAnimationPacket을 상위 Session에 연결한다.
// Native 전송 장치의 실제 설치와 Client/Session 수명 조립은 4-D D·E에서 수행한다. (2026-11-16 WB04-E567)
import { RawDiscRegular } from './raw-disc-regular.js';
import { RawMissile, makeMissileFactoryHooks, makeGunMissileHooks } from './raw-missile.js';
import { GunProcess } from './raw-gun-process.js';
import { GunAnimationFields, GunAnimationKind, GunAnimationProcess, GunAnimationReceiveKind, RawGunAnimation, createGunAnimation, makeGunAnimationHostHooks, initializeReceivedGunAnimation, readGunAnimationProcessData, gunAnimationUsesFirePacket, writeGunAnimationProcessData } from './raw-gun-animation.js';
import { ArcherAnimationFields, ArcherAnimationState, ArcherAnimationProcess, RawArcherAnimation, createArcherAnimation, makeArcherAnimationHostHooks, makeArcherAnimationProcessHooks, makeArcherMissileHooks } from './raw-archer-animation.js';
import { CannonFireState, CannonFireKind, CannonFireProcess, RawCannonFire, createCannonFire, makeCannonFireHostHooks } from './raw-cannon-fire.js';
import { CrossbowAnimationState, CrossbowAnimationProcess, RawCrossbowAnimation, createCrossbowAnimation, makeCrossbowAnimationHostHooks } from './raw-crossbow-animation.js';
import { IceCannonFireProcess, RawIceCannonFire, createIceCannonFire, makeIceCannonFireHostHooks } from './raw-ice-cannon-fire.js';

// 실제 파생 객체의 같은 raw 공통 접두를 빌린다. 다른 프로세스는 빈 span에 해당하는 배열이다. (2026-11-16 WB04-E567)
function animationFields(process) {
    if (process instanceof GunAnimationProcess || process instanceof CannonFireProcess || process instanceof ArcherAnimationProcess || process instanceof CrossbowAnimationProcess) return process.fields.bytes;
    if (process instanceof IceCannonFireProcess) return process.fields.common.bytes;
    return new Uint8Array(0);
}

// 원본 DWORD를 물리 범위 안에서만 읽는다. SID 축소는 수신 함수의 지정 위치에서 검사한다. (2026-11-16 WB04-E567)
function dword(bytes, at) {
    if (at > bytes.length || 4 > bytes.length - at) throw new RangeError('애니메이션 수신의 DWORD 부족');
    return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(at, true);
}

// 같은 장기 TYPE 주소를 값 사본 없이 빌리는 읽기 상자이다. (2026-11-16 WB04-E567)
function typeReference(aux, name) { return { get value() { return aux[name]; } }; }

// 원문의 8DWORD 값형 문맥을 float 비트 네 칸·현재 TYPE/방향/owner/프레임으로 해석한다. (2026-11-16 WB04-E567)
function targetQuery(words) {
    if (words.length !== 8) throw new RangeError('아이스 발사의 실제8DWORD 문맥 부족');
    // raw float 비트의 동일 해석을 위한 임시 보기는 실제 저장소를 교체하지 않는다.
    const view = new DataView(new ArrayBuffer(16));
    // 좌표 네 DWORD를 부동소수 연산 없이 먼저 복사한다.
    for (let index = 0; index < 4; index += 1) view.setUint32(index * 4, words[index] >>> 0, true);
    return { x: view.getFloat32(0, true), y: view.getFloat32(4, true), centerX: view.getFloat32(8, true), centerY: view.getFloat32(12, true), type: words[4] >>> 0, direction: words[5] | 0, owner: words[6] >>> 0, frame: words[7] | 0 };
}

// 같은 Core/전역/난수의 참조를 빌리고 실제 하위 처리기만 소유한다. Kernel/콜백 사용 종료 후 반납한다. (2026-11-16 WB04-E567)
export class RawCampaignArcherBindings {
    // 상위보다 오래 살아야 하는 월드·현재 Core·표적·공유 시각이다.
    #world; #core; #target; #animationState; #missileState;
    // 실제 애니메이션·투사체·예약 처리기이다.
    #missile; #animation; #cannon; #thunderRotate; #sunFire; #thunderFire; #vanderFire; #ice; #crossbowRotate; #crossbow; #disc;
    // 같은 게임 시각을 읽는 독립 파생 상태이며 현재 TYPE 세 칸만 장기 owner를 빌린다.
    #cannonState = { now: 0, sunCannonType: 128 };
    #thunderRotateState = { now: 0, sunCannonType: 131 };
    #sunFireState = new CannonFireState({ kind: CannonFireKind.Sun, type: 128, displayType: 80, effectType: 11, delay: Math.fround(0.07) });
    #thunderFireState = new CannonFireState();
    #vanderFireState = new CannonFireState({ kind: CannonFireKind.Vander, type: 148, displayType: 80, effectType: 19, delay: Math.fround(0.3) });
    #iceState = new CannonFireState({ type: 129, displayType: 130, delay: Math.fround(0.08) });
    #crossbowRotateState = new ArcherAnimationState({ type: 140 });
    #crossbowState = new CrossbowAnimationState();
    // GunHooks의 실제 현재 문맥 함수 사본과 직전 writer의 버퍼 선택이다.
    #iceContext = null;
    #writingAnimation = false;

    // effects.animation/missile/beam/iceCooling은 선택될 때 실제 외부 장치를 요구한다. (2026-11-16 WB04-E567)
    constructor(world, target, random, animationTypes, frameTypes, missileTypes, animationState, missileState, effects = {}) {
        // 같은 실제 Core와 장기 보조 전역을 빌린다.
        const core = world.core();
        if (target.pool !== core.pool) throw new TypeError('캠페인 Archer 현재 표적 Pool 불일치');
        // 같은 Storage의 장기 TYPE·헤더·공통 전송 소유자이다.
        const aux = world.storage.gameAux;
        this.#world = world; this.#core = core; this.#target = target; this.#animationState = animationState; this.#missileState = missileState;
        // 일반 생성과 수신이 공유하는 세 장기 TYPE 참조이며 썬 캐논 두 소비자는 같은 상자를 쓴다.
        const sun = typeReference(aux, 'sunCannonType');
        animationState.currentType = typeReference(aux, 'sunArcherType');
        this.#cannonState.currentSunCannonType = sun; this.#sunFireState.currentType = sun;
        this.#crossbowRotateState.currentType = typeReference(aux, 'windArcherType');
        this.#crossbowState.currentType = this.#crossbowRotateState.currentType;
        this.#disc = new RawDiscRegular(core.pool, animationTypes, core.frame);
        // C++ effects 값 인수처럼 함수 묶음을 복사하고 참조 대상은 같은 실제 월드/난수로 연결한다.
        const flight = { ...effects.missile,
            // 원문 min 포함/max 제외 난수 변환을 같은 게임 스트림에서 읽는다.
            randomInteger: (low, high) => { low |= 0; high |= 0; if (high <= low) throw new RangeError('투사체 정수 난수 구간 오류'); return (low + (random.next((high - low) >>> 0) | 0)) | 0; },
            randomFloat: (low, high) => { low = Math.fround(low); high = Math.fround(high); return low + (random.next(65536) * (1 / 65536)) * Math.abs(high - low); },
            // 실제 자산 owner·공간·중심·파괴를 같은 Core/World로 전달한다.
            owner: (sid, owner) => core.owner.set(sid, owner & 255),
            unpop: (sid, flags) => world.unpop(sid, flags), pop: (sid, x, y, flags) => world.pop(sid, Math.fround(x), Math.fround(y), flags),
            center: (sid) => target.center(sid), destroy: (sid, flags) => world.destroy(sid, flags),
        };
        this.#missile = new RawMissile(core.pool, core.types, missileTypes, missileState, makeMissileFactoryHooks(core.pool, core.factory, flight));
        // 좌표/공간/난수와 출력 통지는 현재 실제 장치로 바꾼 값 사본이다.
        const hooks = { ...effects.animation, common: { ...effects.animation?.common,
            center: (sid) => target.center(sid), unpop: (sid, flags) => world.unpop(sid, flags), repop: (sid, flags) => world.repop(sid, flags),
            randomInteger: (low, high) => ((low | 0) + (random.next(((high | 0) - (low | 0)) >>> 0) | 0)) | 0,
        }, notifySurface: (sid) => core.building.notifySurface(sid) };
        // 회전과 공통 발사는 각각 실제 파생 클래스의 완료/삭제를 사용한다.
        const cannonHooks = makeGunMissileHooks(core.processes, this.#missile, makeGunAnimationHostHooks(core.processes, hooks.common));
        const fireHooks = makeGunMissileHooks(core.processes, this.#missile, makeCannonFireHostHooks(core.processes, hooks.common));
        this.#thunderRotate = new RawGunAnimation(core.pool, core.types, this.#thunderRotateState, cannonHooks);
        this.#cannon = new RawGunAnimation(core.pool, core.types, this.#cannonState, { ...cannonHooks });
        this.#sunFire = new RawCannonFire(core.pool, core.types, frameTypes, this.#sunFireState, { ...fireHooks });
        this.#vanderFire = new RawCannonFire(core.pool, core.types, frameTypes, this.#vanderFireState, { ...fireHooks }, { ...effects.beam, random });
        // 아이스의 문맥은 GunHooks를 연결한 이후의 현재 raw 조회 함수다.
        const iceCommon = makeGunMissileHooks(core.processes, this.#missile, makeIceCannonFireHostHooks(core.processes, hooks.common));
        this.#ice = new RawIceCannonFire(core.pool, core.types, frameTypes, this.#iceState, iceCommon, {
            cooling: effects.iceCooling,
            context: (sid) => { if (typeof this.#iceContext !== 'function') throw new Error('아이스 발사의 실제 현재 문맥 미연결'); return this.#iceContext(sid); },
            mayTarget: (context, sid) => target.mayTarget(targetQuery(context), sid),
        });
        this.#thunderFire = new RawCannonFire(core.pool, core.types, frameTypes, this.#thunderFireState, { ...fireHooks });
        this.#crossbow = new RawCrossbowAnimation(core.pool, core.types, animationTypes, this.#crossbowState, makeGunMissileHooks(core.processes, this.#missile, makeCrossbowAnimationHostHooks(core.processes, hooks.common)));
        // Archer 회전/발사는 실제 Missile 공장을 공유하며 복사한 진단 함수는 같은 현재 Client 전역을 조회한다.
        const archerHooks = makeArcherMissileHooks(core.processes, this.#missile, makeArcherAnimationHostHooks(core.processes, hooks));
        this.#crossbowRotateState.angles = { ...animationState.angles };
        this.#crossbowRotate = new RawArcherAnimation(core.pool, core.types, animationTypes, this.#crossbowRotateState, archerHooks);
        this.#animation = new RawArcherAnimation(core.pool, core.types, animationTypes, animationState, { ...archerHooks, common: { ...archerHooks.common } });
    }

    // 유한 게임 시각/음수가 아닌 경과를 검증한 뒤 다음 Kernel 실행에 같은 시각을 공급한다. (2026-11-16 WB04-E567)
    prepareFrame(time) {
        if (!Number.isFinite(time.game) || !Number.isFinite(time.delta) || time.delta < 0) throw new RangeError('Archer 현재 clock/경과 오류');
        this.#animationState.now = time.game; this.#animationState.delta = time.delta; this.#missileState.now = time.game;
        this.#crossbowRotateState.delta = time.delta;
        // double 입력을 고정/반올림하지 않고 각 실제 파생 상태에 같은 현재 시각을 쓴다.
        for (const state of [this.#cannonState, this.#thunderRotateState, this.#sunFireState, this.#thunderFireState, this.#vanderFireState, this.#iceState, this.#crossbowRotateState, this.#crossbowState]) state.now = time.game;
    }

    // 현재 부모 TYPE/단계로 실제 공장을 고르고 다른 소비자의 입력 훅을 보존한다. (2026-11-16 WB04-E567)
    gunHooks(hooks = {}) {
        this.#iceContext = hooks.shotContext;
        // 다른 파생의 함수는 값으로 캡처한다.
        const fallback = hooks.startAnimation;
        return makeArcherAnimationProcessHooks(this.#core.processes, this.#animation, { ...hooks, startAnimation: (stage, form, target) => {
            // 첫 Archer 분배 뒤에도 현재 Gun과 부모 TYPE을 원문처럼 다시 읽는다.
            const gun = this.#core.processes.fromForm(form);
            if (!(gun instanceof GunProcess)) throw new Error('캠페인 총 애니메이션의 현재 GunProcess 조회 실패');
            // 현재 raw 부모 TYPE이다.
            const type = this.pool.slot(gun.parent)[10];
            // 원문 비교 순서대로 실제 파생 공장 하나만 실행한다.
            const routes = [
                [() => type === this.#ice.type && stage === 5, () => createIceCannonFire(this.#core.processes, this.#ice, form, target)],
                [() => type === this.#sunFire.type && stage === 5, () => createCannonFire(this.#core.processes, this.#sunFire, form, target)],
                [() => type === this.#cannon.sunCannonType() && (stage === 2 || stage === 8), () => createGunAnimation(this.#core.processes, this.#cannon, stage, form, target)],
                [() => type === this.#thunderFire.type && stage === 5, () => createCannonFire(this.#core.processes, this.#thunderFire, form, target)],
                [() => type === this.#vanderFire.type && stage === 5, () => createCannonFire(this.#core.processes, this.#vanderFire, form, target)],
                [() => type === this.#thunderFire.type && stage === 2, () => createGunAnimation(this.#core.processes, this.#thunderRotate, stage, form, target)],
                [() => type === this.#crossbow.type && stage === 2, () => createArcherAnimation(this.#core.processes, this.#crossbowRotate, stage, form, target)],
                [() => type === this.#crossbow.type && stage === 5, () => createCrossbowAnimation(this.#core.processes, this.#crossbow, form, target)],
            ];
            // 첫 일치에서 돌아가며 뒤 TYPE 조회·생성·fallback을 실행하지 않는다.
            for (const [selected, create] of routes) if (selected()) { if (!create()) throw new Error('캠페인 실제 발사 animation 부착 실패'); return; }
            if (typeof fallback !== 'function') throw new Error(`캠페인 총 애니메이션 미복원 TYPE${type} stage${stage}`);
            fallback(stage, form, target);
        } });
    }

    // 실제 Disc 예약만 추가하고 그 밖의 사건은 입력 fallback에 넘긴다. (2026-11-16 WB04-E567)
    handler(fallback = null) { return this.#disc.handler(fallback); }
    // 후속 연결이 같은 처리기와 Pool을 빌린다. (2026-11-16 WB04-E567)
    get animation() { return this.#animation; }
    get missile() { return this.#missile; }
    get pool() { return this.#core.pool; }

    // 3b/3c 기본 생성 공장과 현재 PID decoder를 같은 World의 명시 Form에 연결한다. (2026-11-16 WB04-E567)
    receiveAnimationPacket(message, packet) {
        if (message !== 0x3b && message !== 0x3c) throw new RangeError('애니메이션 수신의 실제 메시지 범위');
        if (packet.length < 9) throw new RangeError('애니메이션 수신의 실제9BYTE 머리 부족');
        // 부모0은 원문 공장/부착 위치에서만 조회하며 DWORD SID는 물리 범위를 검사한다.
        const sid = (value) => { if (value > 65535 || value >= this.pool.capacity) throw new RangeError('애니메이션 수신의 DWORD SID 물리 범위'); return value; };
        // 수신 패킷의 전체 DWORD Form 번호이다.
        const number = dword(packet, 4);
        if (!number) throw new RangeError('애니메이션 수신의 명시 Form0');
        // 선택한 실제 Form 번호와 수신 파생 선택이다.
        const form = sid(number); const fire = message === 0x3b;
        // 발사/회전별 원문 진단 파일이다.
        const file = fire ? 'c:\\ns\\o\\_p_fireanimprocessdata.cpp' : 'c:\\ns\\o\\_p_rotateanimprocessdata.cpp';
        // 현재 World의 장기 물리 저장소이다.
        const storage = this.#world.storage; const aux = storage.gameAux;
        if (this.pool.slot(form)[11] & 1) {
            if (packet[8] !== 0xff) storage.assertDisplay('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', file, 0x42);
            // 현재 부모 TYPE만으로 수신 전용 파생을 선택한다. 일반 발사 초기화는 실행하지 않는다.
            const type = this.pool.slot(sid(dword(packet, 9)))[10];
            let pending = null;
            if (fire && type === aux.sunArcherType) {
                // 해당 실제 파생의 원문 필드 이미지이다.
                const fields = new ArcherAnimationFields(); initializeReceivedGunAnimation(fields.bytes, GunAnimationReceiveKind.ArcherFire);
                pending = new ArcherAnimationProcess(this.#animation, fields, true);
            } else if (type === aux.sunCannonType) {
                // 해당 실제 파생의 원문 필드 이미지이다.
                const fields = new GunAnimationFields(); initializeReceivedGunAnimation(fields.bytes, fire ? GunAnimationReceiveKind.SunFire : GunAnimationReceiveKind.SunRotate);
                pending = fire ? new CannonFireProcess(this.#sunFire, fields) : new GunAnimationProcess(this.#cannon, fields, GunAnimationKind.SunRotate);
            } else if (!fire && type === aux.windArcherType) {
                // 해당 실제 파생의 원문 필드 이미지이다.
                const fields = new ArcherAnimationFields(); initializeReceivedGunAnimation(fields.bytes, GunAnimationReceiveKind.ArcherRotate);
                pending = new ArcherAnimationProcess(this.#crossbowRotate, fields, false);
            } else storage.assertDisplay('false', '\\Ns\\O\\Gunanim.cpp', fire ? 0x23b : 0x248);
            // 앞선 선택 필드 쓰기는 실패해도 보존하며 현재 프로세스 TYPE을 부착 직전에 읽는다.
            const parent = readGunAnimationProcessData(animationFields(pending), packet);
            this.#core.processes.attach(pending, fire ? aux.fireAnimationProcessType : aux.rotateAnimationProcessType, sid(parent), 0x10, form);
            return;
        }
        if (packet[8] & 1) storage.assertDisplay('(*(char *)(( (char *)zacket+8) ) & 1) == 0', file, 0x4d);
        readGunAnimationProcessData(animationFields(this.#core.processes.fromForm(form)), packet);
    }

    // 실제 Native 장기 장치가 설치된 경우에만 그 분기를 선택한다. 미설치 typed World는 직접 writer를 쓴다. (2026-11-16 WB04-E567)
    #nativeTransmission() { return this.#world.storage.usesNativeTransmissionDevices?.() ?? false; }

    // 현재 VT/헤더로 같은 장기 버퍼에 쓰고 다른 프로세스는 원래 writer에 넘긴다. (2026-11-16 WB04-E567)
    transmission(send, fallback = {}) {
        // C++ 값 인수의 함수 사본과 실제 현재 헤더/전송 버퍼를 구분한다.
        const rest = { ...fallback };
        // 현재 파생 필드를 같은 장기 버퍼에 쓰는 함수이다.
        const write = (fields, parentChanged, force) => {
            // 같은 Storage의 장기 TYPE·헤더·공통 전송 소유자이다.
            const aux = this.#world.storage.gameAux;
            // 현재 VT가 선택한 메시지/프로토콜 두 BYTE이다.
            const header = gunAnimationUsesFirePacket(fields) ? aux.fireAnimationHeader : aux.rotateAnimationHeader;
            return writeGunAnimationProcessData(fields, aux.squidTransmission.buffer, parentChanged, force, header[0], header[1]);
        };
        // 같은 현재 버퍼의 작성 범위 보기이다.
        const packet = (length, message) => {
            // 현재 장기 공통 전송 버퍼이다.
            const buffer = this.#world.storage.gameAux.squidTransmission.buffer;
            if (length < 0 || length > buffer.length) throw new RangeError(message);
            return buffer.subarray(0, length);
        };
        return {
            // 직전 writer가 선택한 버퍼는 이 인스턴스의 모든 반환 transmission이 공유한다.
            writeData: (process, parentChanged) => {
                // 해당 실제 파생의 원문 필드 이미지이다.
                const fields = animationFields(process); this.#writingAnimation = fields.length !== 0;
                if (this.#writingAnimation) return this.#nativeTransmission() ? this.#world.storage.serializeProcessForm(process, this.#world.storage.gameAux.squidTransmission.buffer, parentChanged, 0) : write(fields, parentChanged, false);
                if (typeof rest.writeData !== 'function') throw new Error('다른 프로세스의 실제 일반 writer 미연결');
                return rest.writeData(process, parentChanged);
            },
            // force는 DWORD 전체의 비영 여부이며 parentChanged를 대신하지 않는다.
            writeFullData: (process, force) => {
                // 해당 실제 파생의 원문 필드 이미지이다.
                const fields = animationFields(process); this.#writingAnimation = fields.length !== 0;
                if (this.#writingAnimation) return this.#nativeTransmission() ? this.#world.storage.serializeProcessForm(process, this.#world.storage.gameAux.squidTransmission.buffer, false, force) : write(fields, false, (force >>> 0) !== 0);
                if (typeof rest.writeFullData !== 'function') throw new Error('다른 프로세스의 실제 전체 writer 미연결');
                return rest.writeFullData(process, force);
            },
            // 같은 buffer의 보기를 돌려주며 직전 길이를 캐시하거나 복사하지 않는다.
            packet: (length) => {
                if (this.#writingAnimation) return packet(length, '애니메이션의 실제 작성 버퍼 범위');
                if (typeof rest.packet !== 'function') throw new Error('다른 프로세스의 실제 작성 버퍼 미연결');
                return rest.packet(length);
            },
            // 일반/강제 writer와 같은 선택 표식을 읽고 실제 송신 장치를 요구한다.
            transmit: (length) => {
                if (!this.#writingAnimation) { if (typeof rest.transmit !== 'function') throw new Error('다른 프로세스의 실제 송신 미연결'); rest.transmit(length); return; }
                if (this.#nativeTransmission()) { this.#world.storage.broadcastProcessForm(length); return; }
                if (typeof send !== 'function') throw new Error('애니메이션의 실제 송신 장치 미연결');
                send(packet(length, '애니메이션의 실제 송신 길이 범위'));
            },
        };
    }
}
