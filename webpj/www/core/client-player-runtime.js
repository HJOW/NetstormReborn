// 공통 시작의 실제 Player 할당과 방향별 동맹 초기화 장치를 연결하는 모듈이다(cpppj `client/ClientPlayerRuntime.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 없음).
// `initializeClientPlayerRecords(client)` 는 원문 0043952c 의 실제 초기화이다: 현재 known 이 0 인 초기 경로만 요구하며(아니면 `공통 Player 시작의 실제 known0 관문` 거부), 없는 World/Chat 을 빈 성공으로 채우지 않는다.
// 같은 소유자(`client.playerRecords()`)에서 known0→전체 9기록/9공장/81방향 동맹을 구성하며, 동맹은 기존 실제 자식(`RawPlayerParticipation.setAlliance`, 00490ea0)이 계산한다.
//   참여 모듈에는 known 이 0 일 때 읽지 않는 World 조회 세 개(battle·metamap·localTransport)를 "미선택 World 장치" 오류로 연결해, 상수 모드로 통과하는 일이 없게 한다.
// client 계약: playerRecords()(`ClientPlayerRecords`)·gameplayGlobals(){ player } — player 는 `battleOwner`/`playerKnown` 속성을 가진 `PlayerWorldState`. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-12 WB04-E332: 4-D ClientPlayerRuntime 이식)
import { PlayerParticipationState, RawPlayerParticipation } from './raw-player-participation.js';

// 공통 Player 시작을 실제 소유자에 실행한다. 원문의 초기 숨김 9BYTE(0052f81e)는 최초 0 이며 known 0 에서는 실제로 읽지 않는다. (2026-11-12 WB04-E332)
export function initializeClientPlayerRecords(client) {
    const game = client.gameplayGlobals();
    if (game.player.playerKnown !== 0) throw new Error('공통 Player 시작의 실제 known0 관문');
    const records = client.playerRecords();
    records.initialize((owner, mask) => {
        const state = new PlayerParticipationState();
        const initialHidden = new Uint8Array(9);
        const initial = new RawPlayerParticipation(records.records(), records.alliances(), initialHidden, state, {});
        // known0 이 아닌 선택을 상수 모드로 통과시키지 않도록 World 조회 세 개는 도달하면 거부한다.
        const unavailable = () => { throw new Error('공통 Player 동맹 시작의 미선택 World 장치'); };
        initial.bindCurrentInputs({ localOwner: () => game.player.battleOwner, playerKnown: () => game.player.playerKnown, battle: unavailable, metamap: unavailable, localTransport: unavailable });
        initial.setAlliance(owner, mask);
    });
}
