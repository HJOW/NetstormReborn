// State 정리의 커서 취소를 현재 Session 또는 같은 Client의 실제 초기 커서에 연결한다.
// 사용: cancelClientStateCursor(gameplay, cancelWithoutRuntime). 빠진 선택 경계는 도달할 때 거부한다.
import { StateWorldCleanupEffect } from './raw-state-world-cleanup.js';
import { clearClientUserSelection } from './client-user-selection-cleanup.js';
import { clearClientMetaZoneRecords, resetClientClockSynchronization, resetClientSquidPool, resetClientStateWorldStorage, resetClientStateZone } from './client-state-storage-cleanup.js';
import { resetPriestOwnerState } from './raw-priest-post-pop.js';
import { setPlayerSpAmount } from './raw-fort-money-load.js';
import { initializeClientPlayerRecords } from './client-player-runtime.js';

// 실제로 선택된 외부 장치가 없으면 그 위치에서 거부한다.
function need(device) { if (typeof device !== 'function') throw new Error('전체 State 정리의 선택된 실제 외부 장치 누락'); return device; }

// 숫자를 원본 SP 저장소가 사용하는 float32 DWORD로 바꾼다.
function floatBits(value) { const data = new DataView(new ArrayBuffer(4)); data.setFloat32(0, value, true); return data.getUint32(0, true); }

// Session 대여가 있고 실제 커서가 설치된 경우에만 State 전용 취소를 실행한다.
export function cancelClientStateCursor(gameplay, cancelWithoutRuntime) {
    if (gameplay.borrowed()) {
        const session = gameplay.requireSession();
        if (session.hasCursorRuntimeForState()) { session.cancelCursorForState(); return; }
    }
    if (typeof cancelWithoutRuntime !== 'function') throw new Error('전체 State 정리의 선택된 실제 외부 장치 누락');
    cancelWithoutRuntime();
}

// 원문 State 부모가 선택하는 효과와 자식을 현재 Client 소유자에 연결한다. 만들 때 효과는 실행하지 않는다.
// context는 gameplay·storage·local·players·coreData·connections·sp·templates·chat·audio·client 및 저장소 정리 입력을 빌린다.
export function makeClientStateWorldHooks(context, devices, runtime) {
    const { gameplay, storage, local, players, coreData, connections, sp, templates, chat, audio, client } = context;
    const E = StateWorldCleanupEffect;
    return {
        disconnect: (owner) => connections.disconnect(owner | 0), assertion: devices.assertion,
        pauseDepth: () => client.paused() ? 1 : 0,
        saveFort: (slot) => { if (devices.saveFort) devices.saveFort(slot); else gameplay.requireSession().snapshotFort(slot); },
        saveBattle: devices.saveBattle, resetAi: (owner) => runtime.resetAi(owner),
        resetPriest: (owner) => resetPriestOwnerState(storage.priestOwnerState, owner),
        resetOtherPlayer: (owner, amount) => setPlayerSpAmount(owner | 0, amount | 0, sp, {
            clientActive: () => gameplay.player.playerKnown, localOwner: () => gameplay.player.battleOwner, editor: () => gameplay.world.editor,
        }, () => need(devices.refreshSp)()),
        setSp: (owner, value) => sp.set(owner, floatBits(value)),
        selectCore: (owner, templateOwner) => coreData.select(gameplay.player, Number(storage.ownerMode.challenge), owner, templateOwner),
        resetPool: (capacity) => { runtime.retire(devices.retireRuntime); resetClientSquidPool(context, capacity | 0); },
        wallSeconds: () => client.wallSeconds(),
        // 미연결 UI·음향 효과는 그 원문 위치에서만 필수로 요구한다.
        effect(effect) {
            switch (effect) {
            case E.CloseAllConnections: connections.closeAll(); return;
            case E.EndConnections: connections.endPlayers(gameplay.server().value ? 1 : 0); return;
            case E.ResumeGame: client.popGamePause(); return;
            case E.ResumeAudio: need(audio?.popMute?.bind(audio))(); return;
            case E.Clear446e70: cancelClientStateCursor(gameplay, devices.cancelInitialCursor); return;
            case E.Clear4c1f20: resetClientClockSynchronization(local); return;
            case E.ResetClock: client.resetGameClock(); return;
            case E.Clear42b1b0: clearClientMetaZoneRecords(context); return;
            case E.RefreshSp: need(devices.refreshSp)(); return;
            case E.ClearTemplates: templates.clearAll(); return;
            case E.LeaveMetamap: need(devices.leaveMetamap)(); return;
            case E.Clear445140: chat.resetOccupied(); return;
            case E.ClearCursor: clearClientUserSelection(gameplay, storage, devices.selectCursor); return;
            case E.ClearPlayerLinks: runtime.clearPlayerLinks(); return;
            case E.ClearKernel: runtime.clearKernel(); return;
            case E.KillGumps: need(devices.killGumps)(); return;
            case E.CleanupGumps: need(devices.cleanupGumps)(); return;
            case E.ReplaceUber: need(devices.replaceUber)(); return;
            case E.Clear43b540: case E.Reset422490: case E.Clear4c2450: case E.Clear472830: case E.Clear4b2b50:
            case E.Clear4328a0: case E.Clear4ab940: case E.Reset568b60: case E.Reset54c8a0: case E.ResetGraph:
                resetClientStateWorldStorage(context, effect); return;
            case E.MapMode: return; // 원본 004437f0은 C3 단일 RET다.
            case E.SceneMusic: need(audio?.startScene?.bind(audio))(storage.ownerMode.battle); return;
            case E.Clear4a9e50: { const sounds = need(audio?.sounds?.bind(audio))(); need(sounds?.stopLoops?.bind(sounds))(); return; }
            case E.InitializePlayers: initializeClientPlayerRecords({ gameplayGlobals: () => gameplay, playerRecords: () => players }); return;
            case E.ResetZone: resetClientStateZone(local); return;
            case E.InterpretOptions: client.interpretStateOptions(); return;
            default: throw new RangeError('전체 State 정리의 미등록 Effect');
            }
        },
    };
}
