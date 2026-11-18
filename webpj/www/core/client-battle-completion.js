// 실제 Client 장기 소유자를 일반 전투 종료 프레임에 연결한다(10.78 UberGump::PollBattleCompletion).
// 사용: pollClientBattleCompletion(client, devices). 결과 상자와 UI·저장 장치는 선택된 분기에서만 필요하다.
import { RawBattleCompletion } from './raw-battle-completion.js';
import { gameMenuCommandValue } from './game-menu-commands.js';

// 선택된 외부 UI·저장 효과가 없으면 그 호출 지점에서 거부한다.
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`전투 종료의 실제 ${name} 미연결`);
    return device;
}

// 메뉴와 전투에서 같은 Player·연결·State·GameAux를 재조회하며 종료 프레임을 한 번 수행한다.
export function pollClientBattleCompletion(client, devices = {}) {
    if (client.edition() !== 'patch1078') throw new Error('전투 종료 Client의 10.78 판본 필요');
    const game = client.gameplayGlobals(), local = client.localStateGlobals(), storage = client.surfaceStorage(), records = client.playerRecords(), aux = game.gameAux();
    const globals = {
        players: records.records(), options: game.battleOptionsOwner(), alliances: records.alliances(),
        get battle() { return storage.ownerMode.battle; }, get missionPost() { return game.world.missionPost; }, get boss() { return game.authority().boss; },
        get scriptBreak() { return local.scriptBreak().value; }, get loginEnd() { return local.loginEnd().value; },
    };
    const playerHooks = {
        currentPlayers: () => records.records(), serverConnection: () => Boolean(client.connectionGlobals().cleanup().transceivers[9]),
        priestDead: (owner) => game.requireSession().missionPriestDead(owner),
    };
    const frame = {
        players: records.records(), get server() { return game.authority().server; }, get phase() { return local.state().phase[0]; }, get localOwner() { return game.player.battleOwner; },
        get captureEnd() { return local.captureEnd().value; }, set captureEnd(value) { local.captureEnd().value = value; },
        get drawDeadline() { return aux.scheduledDrawDeadline; }, set drawDeadline(value) { aux.scheduledDrawDeadline = value; },
        get settleFrames() { return aux.battleFinishFrames; }, set settleFrames(value) { aux.battleFinishFrames = value; },
        get result() { return devices.resultState.value; }, set result(value) { if (!devices.resultState || typeof devices.resultState !== 'object') throw new Error('전투 종료의 실제 결과 상자 미연결'); devices.resultState.value = value; },
        get gameSeconds() { return client.time().game; },
    };
    const frameHooks = {
        wallSeconds: () => client.wallSeconds(), currentDialog: () => need(devices.currentDialog, '현재 대화상자')(),
        assertion: (expression, file, line) => storage.assertDisplay(expression, file, line),
        deleteSavedState: () => need(devices.deleteSavedState, '저장 정리')(), rejectDraw: () => need(devices.rejectDraw, '무승부 거절')(),
        queueMainMenu: () => { const command = gameMenuCommandValue(0x00542a08); if (command === null) throw new Error('전투 종료의 실제 메뉴 명령 누락'); client.input().pushCommand(command, 1, 1); },
    };
    new RawBattleCompletion(globals, playerHooks).frame(frame, frameHooks);
}
