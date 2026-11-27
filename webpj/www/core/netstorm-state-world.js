// 실제 앱 장기 소유자를 공통 State 정리 부모에 연결한다. 앱의 State7과 미션 이탈에서 사용한다.
import { cleanupClientStateWorld } from './client-state-world-cleanup.js';
import { cancelClientCursorForState } from './client-cursor-state-cleanup.js';
import { ProductionMaintenancePanel } from './raw-production-maintenance.js';

// 새 HUD는 유지하고 이전 World의 비소유 입력·표시 참조와 실제 Runtime만 반납한다.
export function retireNetstormStateWorld(app) {
    const world = app._campaignWorld;
    if (!world || world.session().connectionCallbackActive()) throw new Error('State Runtime 반납의 실제 World 수명 오류');
    app._clientGump.releaseFormHost();
    app._campaignWorld = null; app._campaignWorldMissionToken = 0; app._campaignUserInput = null;
    world.dispose();
}

// 현재 Client·표면·시계·UI·음향을 빌려 원문 전체 부모의 효과 순서와 실제 Runtime 반납을 실행한다.
export function cleanupNetstormStateWorld(app, reason) {
    const clock = app._gameClock, storage = app._surfaceStorage, gameplay = app._clientGameplay, gump = app._clientGump;
    if (!clock || !storage || !gameplay || !gump || !app._clientSoundPlayer) throw new Error('앱 전체 State 정리의 실제 장기 수명 누락');
    // 기존 Gump 일괄 삭제보다 먼저 추가 안내의 serial 등록과 자기 줄을 반납한다.
    app._clearCampaignTutorialObjective();
    // Scene 구동기가 없을 때는 보유한 음악 기한도 없다. 존재하는 구동기의 현재 기한만 같은 비트로 작성한다.
    const audio = {
        writeSongEndBits: (bits) => { if (app._sceneMusic) app._sceneMusic.writeSongEndBits(bits); },
        popMute: () => { app._clientSoundPlayer.popMute(); app._audio?.popMute(); },
        startScene: (battle) => { if (battle) app._beginSceneMusic(); else app._endSceneMusic(); },
        sounds: () => app._clientSoundPlayer,
    };
    // 각 장치는 현재 실제 Client 저장소를 사용하며 조회용 대체 Session을 만들지 않는다.
    const client = {
        paused: () => clock.paused(), popGamePause: () => clock.popGamePause(), resetGameClock: () => clock.resetGameClock(),
        wallSeconds: () => clock.wallSeconds(), interpretStateOptions: () => app._interpretClientStateOptions(),
    };
    const context = {
        gameplay, storage, currentStorage: storage, local: app._clientLocal, players: app._clientPlayers,
        coreData: app._clientCoreData, kernel: app._clientKernel, connections: app._clientConnections,
        sp: app._clientSp.store(), templates: app._clientTemplates, chat: app._clientChatChannels,
        activity: app._clientConnectionActivity, random: app._clientRandom, popupState: app._clientHelpPopup,
        nsInfo: app._clientNsInfo, kencloud: app._clientKencloud, audio, client, gameTime: () => clock.time().game,
    };
    cleanupClientStateWorld(reason, context, {
        assertion: (line) => { throw new Error(`원문 State 중복 모드 진단: ${line}`); },
        cancelInitialCursor: () => cancelClientCursorForState({ gameplayGlobals: () => gameplay, getRenderer: () => app._clientRenderer }, storage),
        refreshSp: () => app._refreshCampaignStormPower(), selectCursor: (number) => { app._campaignCursor = number; app._applyCursor(); },
        killGumps: () => gump.lifetime().killExceptType(() => 11),
        cleanupGumps: () => {
            app._gameMenuButton?.destroy(); app._gameMenuButton = null;
            app._campaignProduction?.ui?.close(); app._campaignProduction?.hud?.close();
            gump.lifetime().cleanup(); gump.releaseFormHost(); app._campaignProduction = null;
        },
        replaceUber: () => {
            app._campaignProduction = { session: null, panel: new ProductionMaintenancePanel(), maintenance: null, ui: null, worldBound: false };
            app._createCampaignProductionHud(app._campaignProduction);
        },
        retireRuntime: () => retireNetstormStateWorld(app),
    });
    // 실제 부모가 모두 해제한 메뉴 정지 깊이의 호스트 보유 표식도 반납한다.
    app._menuPauseDepth = 0; app._menuMuteDepth = 0;
}
