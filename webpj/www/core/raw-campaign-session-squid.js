// 실제 Session의 공통 객체 수신은 같은 Pool·TYPE·Player와 기술 장부를 빌린다(cpppj RawCampaignSession.cpp 5148~5170).
// 사용: receiveSessionSquid(현재 Session 소유자, 수신 35BYTE). 선택된 장치가 미연결이면 그 호출 위치에서 중단한다.
import { receiveRawSquid } from './raw-squid-receive.js';
import { setRawSquidWord } from './raw-squid-word.js';
import { SquidFactory } from './squid-factory.js';
import { decodeOriginalText } from '../assets/text-encoding.js';

// 원문의 비정렬 DWORD를 실제 배열에서 읽거나 쓴다.
function dword(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset + offset, 4); }

// 원본 공통 객체 수신을 현재 Session의 실제 장치에 연결한다.
export function receiveSessionSquid(o, packet) {
    const c = o.core, aux = o.gameplayGlobals.gameAux();
    // 각 조회 시점의 실제 TYPE 표와 진단 장치를 사용한다.
    const typeRecord = (type) => { if (type >= c.types.length) throw new RangeError('객체 수신의 현재 TYPE 물리 범위'); return c.types[type]; };
    const assertion = (expression, file, line) => o.world.storage.assertDisplay(expression, file, line);
    const hooks = {
        assertion,
        take: (type, sid) => c.factory.take(type, sid),
        word: (sid, word) => setRawSquidWord(c.pool, sid, word, c.bridgeState.bridgeType, (current) => c.building.notifySurface(current)),
        owner: (sid, owner) => o.ownership.setOwner(sid, owner),
        flags1: (type) => typeRecord(type).flags1,
        // 현재 가상 표가 원본 생성 자산에 속할 때만 해당 FirstPop을 호출한다.
        firstPop: (sid) => { if (!SquidFactory.supportsConstructedVtable(dword(c.pool.slot(sid), 0).getUint32(0, true))) throw new Error('객체 수신의 현재 가상30 미연결'); return c.factory.firstPopFlags(sid); },
        // 각각의 진단 복귀 뒤 현재 TYPE와 같은 Core 기술 장부를 다시 읽는다.
        hasTechnology: (owner, type) => {
            if (!owner || typeRecord(type).techBitIndex === -1) return true;
            if (owner > 8) assertion('playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS', '\\Ns\\O\\Player.cpp', 0x68);
            const index = typeRecord(type).techBitIndex;
            if (index < 0 || index >= 32) assertion('techBitIndex >= 0 && techBitIndex < 32', '\\Ns\\O\\Ch.cpp', 0x164);
            const records = o.coreDataGlobals.records(), offset = owner * 16 + 10;
            if (offset + 4 > records.length) throw new RangeError('객체 수신의 실제 Core 장부 물리 범위');
            return (dword(records, offset).getUint32(0, true) & (1 << (index & 31))) !== 0;
        },
        // 이름의 원본 바이트와 객체 설명을 결합한 뒤 실제 표시 문자로 변환한다.
        noMake: (sid) => {
            const owner = c.pool.slot(sid)[34]; if (owner > 8) throw new RangeError('PlayerNoMake의 실제 Player 물리 범위');
            const offset = owner * 180 + 12, name = o.players.subarray(offset, offset + 32);
            if (!name.includes(0)) throw new RangeError('PlayerNoMake의 현재 이름 NUL 누락');
            const description = o.objectNames.baseName(sid), end = o.players.indexOf(0, offset);
            if (end < 0) throw new RangeError('PlayerNoMake의 현재 이름 물리 범위');
            const rawName = String.fromCharCode(...o.players.subarray(offset, end)), message = `PlayerNoMake(${rawName},${description})`;
            if (message.length >= 256) throw new RangeError('PlayerNoMake의 원문256BYTE 범위');
            if (typeof o.services.factoryTell !== 'function') throw new Error('PlayerNoMake의 실제 Tell 미연결');
            o.services.factoryTell(decodeOriginalText(Uint8Array.from(message, (letter) => letter.charCodeAt(0))));
        },
        // 안내 호출 뒤의 현재 owner를 다시 선택해 같은 Player 표식만 갱신한다.
        markNoMake: (sid) => {
            const owner = c.pool.slot(sid)[34]; if (owner > 8) throw new RangeError('PlayerNoMake의 현재 Player 표식 범위');
            const view = dword(o.players, owner * 180 + 0x54); view.setUint32(0, view.getUint32(0, true) | 0x400, true);
        },
        pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags),
        containedPop: () => { throw new Error('객체 수신의 실제 BaseAsset 부착 미연결'); },
        typeName: (sid) => typeRecord(c.pool.slot(sid)[10]).name.split('\0', 1)[0],
        log: o.services.player.log,
    };
    // 검증 SID와 이전 TYPE은 다른 모듈과 공유하는 같은 장기 DWORD다.
    const state = {
        get verifySid() { return aux.verifySid; }, set verifySid(value) { aux.verifySid = value; },
        get typeWas() { return aux.supportTypeWas; }, set typeWas(value) { aux.supportTypeWas = value; },
        noMakeWarnings: o.localStateGlobals.playerNoMakeWarnings(),
    };
    receiveRawSquid(c.pool, packet, state, hooks);
}
