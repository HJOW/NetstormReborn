// Session1463..1483의 현재 TYPE 생성자 ABI와 연소 전역을 같은 owner에 연결한다(225553df, WB04-E585).
// 사용: 입자 결선 뒤 owner.connectTeleport(). 경로/펜스 몸체는 호출 시 같은 owner의 현재 슬롯을 읽는다.
import { createCursorManaProcess } from './raw-cursor-mana.js';
import { createCursorRangeProcess } from './raw-cursor-range.js';
import { BombEffectTimerProcess } from './raw-bomb-effect-timer.js';
import { FENCE_TYPE_OPERATION } from './raw-fence-type-parent.js';
import { dispatchCampaignFence } from './raw-campaign-fence-dispatch.js';

// 선택된 cdecl의 서로 다른 인수/반환 수명을 보존한다. 전체 Session/Load 공개는 상위의 몫이다.
export class RawCampaignSessionTeleport {
    // 기반 소유자와 한 번 결선/정상 복귀/반납 상태다. 실패 접두를 되돌리거나 다시 실행하지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;
    // postPop이 빌리는 동일 함수와 연소가 빌리는 장기 현재값 getter다.
    prefix = null; wind = null; weakenedMana = null;

    // 소유 자료를 빌리며 실제 callback 설치는 bind에서 원문 순서로 실행한다.
    constructor(owner) { this.#owner = owner; }

    // 실제 입자 뒤 Teleport 접두·바람·약화 HP·현재 사제/제단 TYPE을 연결한다.
    bind() {
        const o = this.#owner;
        if (this.#disposed || !o.particleModules?.ready || this.#started) throw new Error('Session Teleport의 초기화 전/반복 결선');
        this.#started = true;
        this.prefix = (sid, flags) => this.#construct(sid, flags);
        o.core.postPop.setTeleportPrefix(this.prefix);
        this.wind = () => o.world.storage.wind.x;
        o.particles.bindWind(this.wind);
        this.weakenedMana = () => o.gameplayGlobals.gameAux().priestQuarterHp;
        o.particles.bindWeakenedMana(this.weakenedMana);
        o.particles.bindHitPointTypes(o.gameplayGlobals.gameAux());
        this.#ready = true;
    }

    // flags1에서 현재 Streak TYPE의 함수 주소를 한 번 선택한다. 고정 객체134/process12로 바꾸지 않는다.
    #construct(sid, flags) {
        if (!(flags & 1)) return;
        const o = this.#owner; const c = o.core; const type = o.gameplayGlobals.gameAux().streakProcessType >>> 0;
        if (type >= c.types.length) throw new RangeError('Teleport 현재 process TYPE 표 물리 범위');
        const address = c.types[type].constructorAddress >>> 0;
        if (address === 0x4741d0) { createCursorManaProcess(c.processes, o.cursorMana, sid, 0); return; }
        if (address === 0x474900) { createCursorRangeProcess(c.processes, o.cursorRange, sid, 0); return; }
        if (address === 0x420ad0) { o.unstartedBombTimers.push(this.makeEmptyBombTimer()); return; }
        if (o.motion) {
            const path = o.motion.paths.createByConstructor(address);
            if (path) { o.unstartedPaths.push(path); return; }
        }
        if (c.factory.constructByAddress(address, sid)) return;
        o.particles.createTailByConstructor(address, sid, (expression, _file, line) => {
            const assertion = o.services.ai?.assertion;
            if (typeof assertion !== 'function') throw new Error(`Teleport 생성 함수 실제 진단 장치 미연결 ${line}`);
            assertion(expression);
        });
    }

    // 같은 World와 펜스 분배기를 두 종류의 상태 타이머에 빌려 준다.
    #bombTimerHooks() {
        const o = this.#owner; const c = o.core;
        return {
            unpop: (sid, flags) => o.world.unpop(sid, flags),
            repop: (sid, flags) => o.world.repop(sid, flags),
            refresh: (sid) => c.display.update(c.pool.slot(sid), 0),
            fence: (sid) => dispatchCampaignFence(o, sid, FENCE_TYPE_OPERATION.rebuild),
        };
    }

    // Devastation.inc17..23의 실제 장치를 연결한 빈 타이머다. Init/부착/Frame은 실행하지 않는다.
    makeEmptyBombTimer() {
        const c = this.#owner.core;
        return new BombEffectTimerProcess(c.pool, c.types, () => c.processState.now, this.#bombTimerHooks());
    }

    // 현재 게임 시각과 대상 목록을 복사한 지연 상태 타이머를 생성한다.
    makeBombTimer(delay, members, flags) {
        const c = this.#owner.core;
        return BombEffectTimerProcess.withTimer(c.pool, c.types, () => c.processState.now, delay, members, flags, this.#bombTimerHooks());
    }

    // World 프로세스를 먼저 정리한 부모가 미부착 결과를 motion/입자보다 먼저 반납한다.
    dispose() {
        if (this.#disposed) return;
        this.#disposed = true;
        this.#owner.core.postPop.setTeleportPrefix(null);
        // 미부착 Path도 원문의 가상 소멸자를 실행하며 Kernel에는 등록하지 않는다.
        for (const path of this.#owner.unstartedPaths) path.destroy();
        this.#owner.unstartedPaths.length = 0;
        this.#owner.unstartedBombTimers.length = 0;
    }
    // 이 생성 구간의 정상 복귀다. 전체 Session/Load 완료와 구별한다.
    get ready() { return this.#ready; }
}
