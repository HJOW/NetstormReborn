// State 전환에서 같은 Client의 AI·Player·Kernel 자식을 정리하고 실제 Runtime을 반납한다.
// 사용: new ClientStateRuntimeRetirement({ gameplay, storage, players, kernel, coreData, currentStorage }).resetAi(1) 등, retire(releaseOwner).
import { cleanupPlayerLinks } from './raw-player-links-cleanup.js';

// 생성 시 실제 표면 세대와 현재 Session을 고정하며 자식을 만들거나 정리하지 않는다.
export class ClientStateRuntimeRetirement {
    // 장기 소유자와 최초 Session·표면 세대, 단일 Kernel 정리 증빙이다.
    #context; #session; #generation; #processPass = null;
    constructor(context) {
        const { gameplay, storage, kernel, currentStorage } = context;
        if (!storage || storage.pool.edition !== 'patch1078' || storage.authority !== gameplay.authority() || storage.gameAux !== gameplay.gameAux() ||
            (currentStorage && currentStorage !== storage) || !storage.processes.usesKernel(kernel)) throw new TypeError('State Runtime 반납의 같은 Client/표면/권한/Kernel 필요');
        this.#context = { ...context }; this.#session = gameplay.borrowed() ? gameplay.requireSession() : null; this.#generation = storage.generation; this.#current();
    }
    // 각 자식 호출에서 현재 대여·콜백·표면·Player 정체성을 다시 확인한다.
    #current() {
        const { gameplay, storage, players } = this.#context;
        if (storage.generation !== this.#generation) throw new Error('State 정리 도중 표면 대여 세대 변경');
        if (gameplay.borrowed()) {
            const session = gameplay.requireSession();
            if (session.connectionCallbackActive()) throw new Error('패킷 콜백 실행 중 공유 Runtime 저장소 정리 재진입');
            if (session !== this.#session || session.world().storage !== storage || !storage.borrowed || session.playerRecords() !== players.records()) throw new Error('State 정리 도중 현재 Session/표면/Player 정체성 변경');
            return session;
        }
        if (this.#session || storage.borrowed) throw new Error('State 자식 정상 반납 전에 현재 Runtime 대여가 사라짐');
        return null;
    }
    // 지정된 Player의 실제 AI 정리를 완료하고 같은 Session에 증빙을 반납한다.
    resetAi(owner) {
        if (!Number.isInteger(owner) || owner < 1 || owner > 8) throw new RangeError('State AI 정리의 owner1..8 범위');
        const session = this.#current();
        if (session) { const pass = session.resetAiForState(owner); this.#current().returnAiForState(pass); return; }
        const bytes = this.#context.players.records(), at = owner * 180 + 0xb0;
        if (at + 4 > bytes.length) throw new RangeError('State AI 정리의 현재 Player1..8 물리 범위');
        if (new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(at, true)) throw new Error('현재 비영 brain의 실제 Runtime 소멸 장치 누락');
    }
    // 실제 Player 링크와 선택된 Deck만 정리하고 Session이 있으면 정상 증빙을 반납한다.
    clearPlayerLinks() {
        const session = this.#current(), players = this.#context.players;
        if (session) { const pass = session.clearPlayerLinksForState(); this.#current().returnPlayerLinksForState(pass); return; }
        cleanupPlayerLinks({ players: () => players.records(), release: (token) => players.productionDeckMemory().release(token) });
    }
    // Kernel을 한 번 정리하고 Session의 정상 복귀 증빙을 보관한다.
    clearKernel() {
        if (this.#processPass) throw new Error('같은 State Runtime의 단일 Kernel Clear 증빙 중복 생성');
        const session = this.#current(); if (session) this.#processPass = session.clearProcessesForState(); else this.#context.kernel.clear();
    }
    // Core 해제와 모든 자식 증빙을 검사한 뒤 실제 외곽 소유자를 반납한다.
    retire(releaseOwner) {
        const session = this.#current(), { gameplay, storage, coreData } = this.#context;
        if (gameplay.player.playerKnown || coreData.active || (coreData.current && coreData.current.length)) throw new Error('State Runtime 반납 전 실제 known0/Core0 선택 필요');
        if (!session) return;
        if (!session.production().deck.playerLinksReturnedForState()) throw new Error('State Runtime 반납 전 실제 Player 링크 증빙 필요');
        // 각 Player의 AI 반납 증빙을 개별 확인한다.
        for (let owner = 1; owner < 9; owner += 1) if (!session.participation().brainReturnedForState(owner)) throw new Error('State Runtime 반납 전 실제 여덟 AI 증빙 필요');
        if (typeof releaseOwner !== 'function') throw new Error('State Runtime의 실제 소유자 반납 장치 누락');
        if (!session.world().processBindingsReturned) {
            if (!this.#processPass) throw new Error('State Runtime 반납 전 실제 단일 Kernel Clear 증빙 필요');
            session.returnProcessBindingsForState(this.#processPass); this.#processPass = null;
        }
        releaseOwner();
        if (gameplay.borrowed() || storage.borrowed) throw new Error('State 소유자 장치가 실제 Session/표면 대여를 반납하지 않음');
        this.#session = null;
    }
}
