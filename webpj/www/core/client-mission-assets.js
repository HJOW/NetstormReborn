// 앱의 실제 설정·파일·게임 TYPE으로 선택한 미션 스크립트와 Fort 입력을 준비한다.
// 사용: const mission = new ClientMissionAssets(gameData, gameAssets, name); mission.script / mission.fort; 사용 뒤 mission.dispose().
import { parseFort } from '../assets/fort.js';
import { createFortTypes } from '../assets/type-flags.js';
import { MissionScript } from './mission-script.js';

// 현재 미션 등록 설정과 그 스크립트가 선택한 실제 Fort를 함께 소유한다.
export class ClientMissionAssets {
    // 미션 등록 객체와 입력 Fort는 같은 미션 수명에서만 공개한다.
    #script; #fort; #disposed = false;

    // 누락·형식 오류는 현재 스크립트를 반납한 뒤 원래 예외를 전달한다.
    constructor(data, game, name) {
        if (!data || !game?.typeTable || !String(name ?? '')) throw new TypeError('미션 자산의 현재 설정·게임 TYPE·이름 누락');
        const script = new MissionScript(data, name);
        try {
            if (!script.loaded) throw new Error(`미션 스크립트 없음: ${name}`);
            const fortName = script.fortName();
            this.#fort = parseFort(script.readFortImage(fortName), createFortTypes(game.typeTable));
            this.#script = script;
        } catch (error) { script.dispose(); throw error; }
    }

    // 반납 뒤 오래된 미션 입력을 다시 공개하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Client 미션 자산'); }
    // 같은 미션 설정 객체를 빌린다.
    get script() { this.#live(); return this.#script; }
    // 같은 Fort 입력을 빌린다. Session이 내용을 인수할 수 있으므로 재사용은 호출자가 관리한다.
    get fort() { this.#live(); return this.#fort; }
    // 등록 설정을 해제하고 이전 미션 입력을 반납한다.
    dispose() { if (this.#disposed) return; this.#script.dispose(); this.#script = null; this.#fort = null; this.#disposed = true; }
}
