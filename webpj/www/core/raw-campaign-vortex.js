// RawCampaignVortex.inc의 독립 typed Session 바인딩이다(225553df, WB04-E589).
// 사용: 같은 owner.connectCombat() 뒤 connectVortex(). Native 장기 장치 대여/51h 등록은 상위 통합에 남는다.
import { RawVortexAscend } from './raw-vortex-ascend.js';
import { RawVortexDeparture } from './raw-vortex-departure.js';
import { RawPriestLookup } from './raw-priest-lookup.js';
import { RawIslandQueries } from './raw-island-queries.js';

// 원문의 참조 멤버를 현재 owner의 같은 칸으로 읽고 쓴다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// 선택된 실제 효과만 요구한다. 누락을 성공으로 채우지 않는다.
function need(fn, message) { if (typeof fn !== 'function') throw new Error(message); return fn; }
// 표시 부호화를 마친 원문 바이트 문자열에 마지막 NUL만 붙인다. 유니코드 손실 변환은 거부한다.
function cString(value) { const bytes = new Uint8Array(value.length + 1); for (let n = 0; n < value.length; ++n) { const byte = value.charCodeAt(n); if (byte > 255) throw new RangeError('신전 이름의 표시 부호화 바이트 범위'); bytes[n] = byte; } return bytes; }
// 현재 슬롯의 비정렬 float 좌표를 값으로 읽는다.
function position(raw) { const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); return [view.getFloat32(14, true), view.getFloat32(18, true)]; }
// 지역8BYTE 패킷은 방송/직접 수신에서 같은 mutable 객체를 사용한다.
function packet(temple, island, complete) { return Uint8Array.of(1, 0x51, 8, 0, island, temple, temple >>> 8, complete); }

// 별도 World/Template을 만들지 않고 같은 상승 후보·수신 부모와 명령을 연결한다.
export class RawCampaignVortex {
    // 기반 owner와 한 번 결선/정상 복귀/반납 상태다. 실패한 접두는 되돌리지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 중첩 Tick의 실제80BYTE 후보 할당 수명과 현재 noIsland 참조를 소유한다.
    candidateBuffers = []; ascendState = null; ascendHooks = null; departureHooks = null;
    // 같은 Session을 빌리며 하위 생성은 bind의 원문 순서에서 실행한다.
    constructor(owner) { this.#owner = owner; }
    // 실제 typed 생성자1537의 부모를 같은 장기 Pool/Hash/섬/난수로 만든다. Native 설치는 아직 제공하지 않는다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.combatModules?.ready || this.#started) throw new Error('Session Vortex의 초기화 전/반복 결선');
        this.#started = true;
        this.ascendState = { sharedNoIslandType: reference(c.bridgeState, 'noIslandType') };
        this.ascendHooks = {
            islandNumber: (x, y) => o.ownership.islandNumber(x, y),
            reserveCandidateBuffer: () => { let memory = null; try { memory = new Uint8Array(0x50); } catch (error) { if (!(error instanceof RangeError)) throw error; } this.candidateBuffers.push(memory); return memory !== null; },
            releaseCandidateBuffer: () => { if (!this.candidateBuffers.length) throw new Error('신전 상승 후보 버퍼의 실제 수명 불일치'); this.candidateBuffers.pop(); },
            destroy: (sid, flags) => o.world.destroy(sid, flags),
        };
        o.vortexAscend = new RawVortexAscend(c.pool, c.hash, c.spots, c.types, o.islands, o.random, this.ascendState, this.ascendHooks);
        const h = {
            assertion: (expression, file, line) => o.world.storage.assertDisplay(expression, file, line),
            state: (sid, state) => this.setState(sid, state),
            readTerritory: (owner, records) => { if (!o.runtime) throw new Error('신전 상승의 실제 Territory reader 미설치'); o.runtime.readTerritory(owner, records); },
            writeTerritory: (owner, records) => o.writeTerritory(owner, records),
            refreshProduction: () => need(o.services.ownership.refreshProduction, '신전 상승의 실제 생산 갱신 미연결')(),
            snapshot: () => need(o.vortexSnapshot, '신전 상승의 실제 현재 snapshot 부모 미설치')(),
            owner: (sid, owner) => o.ownership.setOwner(sid, owner), destroy: (sid, flags) => o.world.destroy(sid, flags),
            islandNumber: (x, y) => o.ownership.islandNumber(x, y), ownedIslands: (owner) => new RawIslandQueries(o.islands, o.finishState).countOwned(owner),
            requestEdit: () => { throw new Error('웹 제외: 신전 상승 후 Fort 편집 상태 요청'); },
            log: (text) => need(o.services.player.log, '신전 상승의 실제 로그 장치 미연결')(29, text),
            sendExcept: (excluded, bytes) => o.tables.broadcast.sendExcept(excluded, bytes), regular: (sid) => { c.processes.addRegular(sid, 3, 0); },
            warnAscend: (owner) => { const name = this.playerName(owner); this.#tell('global.subject', name, 'WarnAscend', '신전 상승의 실제 설정/Tell 장치 미연결'); },
            sound: (name) => { need(o.services.sound.play, '신전 상승의 실제 전역 음향 미연결')(o.soundNames.lookup(name), 0, 0, 0, 0, 0); },
        };
        this.departureHooks = h;
        o.vortexDeparture = new RawVortexDeparture(c.pool, c.hash, c.types, o.chunks, o.islands, o.vortexAscend, reference(o.playerState, 'battleOwner'), reference(c.processState, 'server'), o.gameplayGlobals.mapRevision(), h);
        this.#ready = true;
    }
    // 반납된 기반 owner의 저장소를 다시 소비하지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Vortex 장치'); }
    // 설정/Tell을 둘 다 확인한 뒤 현재 바이트 문자열을 NUL 포함하여 전달한다.
    #tell(key, value, section, message) { const s = this.#owner.services; if (typeof s.install.setConfig !== 'function' || typeof s.factoryTell !== 'function') throw new Error(message); s.install.setConfig(key, cString(value)); s.factoryTell(section); }
    // 현재 handle의 NUL 앞 원문만 실제 Player 이름 조회에 빌린다.
    playerName(owner) { this.#live(); const o = this.#owner; const fn = need(o.services.priestTakenName, '신전 상승의 실제 Player 이름 장치 미연결'); const end = o.handle.indexOf(0); if (end < 0) throw new RangeError('신전 상승 현재 이름의 NUL 누락'); return fn(owner >>> 0, Array.from(o.handle.subarray(0, end), (byte) => String.fromCharCode(byte)).join('')); }
    // WORD 저장 뒤 현재 TYPE가 같은 장기 상태 통지 번호일 때 실제 표면에 통지한다.
    setState(sid, state) { this.#live(); const o = this.#owner; const raw = o.core.pool.allocated(sid); raw[12] = state; raw[13] = state >>> 8; if (o.core.pool.slot(sid)[10] === o.gameplayGlobals.gameAux().stateNotifyType) o.core.building.notifySurface(sid); }
    // event3은 방송의 변이를 포함한 같은 패킷을 sender9 직접 수신에 넘긴다.
    requestDeparture(temple, island) { this.#live(); const o = this.#owner; const bytes = packet(temple, island, 1); o.tables.broadcast.send(bytes); o.vortexDeparture.receive(9, bytes); }
    // event4는 지역 조회가 끝난 뒤의 현재 서버/메타 명령을 읽으며 상태를 직접 바꾸지 않는다.
    queueDescend(x, y) { this.#live();
        const o = this.#owner; const region = o.vortexDeparture.fortTerritoryAt(x, y, reference(o.core.ownerMode, 'fort'), reference(o, 'noRegion'));
        if (!o.services.nsInfoGlobals || typeof o.services.developerQueue !== 'function') throw new Error('신전 Descend의 실제 서버/일반 입력 큐 미연결');
        const token = ((o.services.nsInfoGlobals.serverSelection << 16) | region) >>> 0; o.services.developerQueue(o.localStateGlobals.viewMetaCommand().value, token, token);
    }
    // 첫 살아 있는 목록 사제의 조회 뒤 현재 free와 두 현재 좌표의 섬을 원문 순서대로 읽는다.
    priestOnIsland(temple, owner) { this.#live();
        const o = this.#owner; const number = new RawPriestLookup(o.core.pool, o.priestState.priests).find(owner); if (!number) return false;
        const priest = number & 65535; const island = o.ownership.islandNumber(...position(o.core.pool.slot(priest)));
        if ((o.core.pool.slot(priest)[11] & 1) || island === o.noRegion) return false;
        return island === o.ownership.islandNumber(...position(o.core.pool.slot(temple)));
    }
    // 004dbac0의 확인 인자/상태 관문·실제 소리/송신·공통 부모 위임이다.
    command(temple, code, parameter) { this.#live();
        const o = this.#owner; const c = o.core; code >>>= 0; parameter &= 65535; const command = code & 0x03ffffff;
        if (command === 0x0300006f) { o.world.storage.assertDisplay('0', '\\Ns\\O\\Vortex.cpp', 0x2ea); return 1; }
        if (command === 0x0300007c && !(c.pool.slot(temple)[12] | (c.pool.slot(temple)[13] << 8))) {
            if (!c.ownerMode.battle) o.world.storage.assertDisplay('inBattleMode', '\\Ns\\O\\Vortex.cpp', 0x2ef);
            if (!parameter) { if (!this.priestOnIsland(temple, c.pool.slot(temple)[34])) { this.#tell('global.param1', String((temple | 0x10000) >>> 0), 'PriestOffIsland', '신전 Ascend의 실제 설정/Tell 장치 미연결'); return 1; } }
            else if (parameter !== 1) return 1;
            this.setState(temple, 1); const island = o.ownership.islandNumber(...position(c.pool.slot(temple))); const bytes = packet(temple, island, 0);
            need(o.services.sound.play, '신전 Ascend의 실제 시작 음향 미연결')(o.soundNames.lookup('preparingAscend.wav'), 0, 0, 0, 0, 0); this.sendChatMutable(9, bytes); return 1;
        }
        if (command === 0x030000a7 && !(c.pool.slot(temple)[12] | (c.pool.slot(temple)[13] << 8))) { c.processes.addRegular(temple, 4, 0); return 1; }
        if (!o.objectCommand) throw new Error('신전 명령의 실제 공통 ObjectCommand 미설치'); return o.objectCommand.command(temple, code);
    }
    // 실제 설치 전 관문 뒤 현재 Login만 사용한다. 큐 대신 수신기를 동기로 호출하지 않는다(원문4135).
    sendChatMutable(target, bytes) { this.#live(); const o = this.#owner; if (!o.loaded) throw new Error('실제 설치 전 mutable Chat 패킷 송신'); if (o.login) { o.login.sendMutable(target | 0, bytes); return; } throw new Error('실제 로컬 전송 없는 mutable Chat 송신'); }
    // 정상 생성 구간 완료이며 전체 Session/Load·Native 설치 완료를 뜻하지 않는다.
    get ready() { return this.#ready; }
    // 부모 반납 후 공개 메서드를 선택하지 못하도록 표시한다. 장기 전역은 초기화하지 않는다.
    dispose() { this.#disposed = true; }
}
