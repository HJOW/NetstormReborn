// Session 생성자1866..1871의 Template 선택/MakeRuntime과 Fort 사제·섬 채우기 부모다(225553df, WB04-E622).
// 사용: connectTerrain 뒤 connectWorldInstall(fort). 외곽 Session/AI·Load는 다음 생성 구간이다.
import { RawCampaignWorldRuntime } from './raw-campaign-world-runtime.js';
import { RawFortPriestPreparation } from './raw-fort-priest-preparation.js';
import { RawFortFinishOwner } from './raw-fort-finish-owner.js';
import { RawFortRandomPlacement, makeFortSinglePlacementHook } from './raw-fort-random-placement.js';
import { makeNormalTerrainFarmWorldHooks } from './raw-normal-terrain-world-binding.js';
import { RawIslandPopulation } from './raw-island-population.js';
import { addRandomObjects, findFortTerritoryIsland } from './raw-add-random.js';

// 보호 PE의 현재 TYPE 기본값이며 각 typed Impl이 자기 수정 가능한 Map을 만든다.
export const CAMPAIGN_TUTORIAL_TYPES = Object.freeze([[0x541178, 72], [0x541184, 75], [0x541188, 76], [0x54118c, 77], [0x541190, 78], [0x541244, 123], [0x541248, 124], [0x54124c, 125], [0x541258, 128], [0x541264, 131], [0x541268, 132], [0x54126c, 133], [0x541288, 140], [0x54128c, 141], [0x5412ec, 165]].map(Object.freeze));
// 생성 당시의 독립 typed 역할이다. 이후 현재값 쓰기는 같은 owner의 필드/Map만 갱신한다.
export function createCampaignTutorialRoles() { return { tutorialWorkshopType: 85, tutorialArcherType: 71, tutorialTypes: new Map(CAMPAIGN_TUTORIAL_TYPES) }; }
// 같은 현재 속성을 실제 참조로 빌린다.
function reference(owner, key) { return { get value() { return owner[key]; }, set value(value) { owner[key] = value; } }; }
// 원문 FillMissing/SessionMissing처럼 선택한 위치에서만 실패한다.
function missing(line) { return () => { throw new Error(`실제 세션 설치 하위 미연결: RawCampaignSession.cpp:${line}`); }; }
// 선택한 장치가 없으면 앞선 효과를 보존한 채 거부한다.
function need(device, message) { if (typeof device !== 'function') throw new Error(message); return device; }
// 같은 Pool/헤더의 비정렬 DWORD다.
function dword(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }
// 원문 double→int→float 저장과 좌표 범위를 보존한다.
function integer(value) { if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Fort 사제 지역 좌표 signed 범위'); return Math.trunc(value) | 0; }

// 원문 snprintf의 사제 수 서식(d/s/%%·폭/정밀도)을256BYTE 지역 버퍼에 작성한다. UTF-8 본문 바이트는 그대로 보존한다.
export function formatFortPriestCount(bytes, count) {
    count >>>= 0;
    const end = bytes.indexOf(0); if (end < 0) throw new RangeError('Fort 사제 수 번역의 실제 NUL 범위');
    const format = String.fromCharCode(...bytes.subarray(0, end)); const args = [count | 0, count === 1 ? '' : 's']; let argument = 0; let output = '';
    // 서식의 일반 바이트를 순서대로 복사하고 실제 지정자만 인자를 소비한다.
    for (let at = 0; at < format.length;) {
        if (format[at] !== '%') { output += format[at++]; }
        else {
            const match = /^%([-+ #0]*)(\d*)(?:\.(\d+))?([ds%])/.exec(format.slice(at));
            if (!match) throw new Error('Fort 사제 수의 CRT 서식 미복원'); at += match[0].length;
            if (match[4] === '%') output += '%';
            else {
                const value = args[argument++]; const width = Number(match[2] || 0); const precision = match[3] === undefined ? null : Number(match[3]);
                if (width >= 256 || (precision !== null && (precision > 0x7fffffff || (match[4] === 'd' && precision >= 256)))) throw new RangeError('Fort 사제 수의 원문256BYTE 범위');
                let part;
                if (match[4] === 'd') {
                    if (typeof value !== 'number') throw new Error('Fort 사제 수 CRT 정수 인자 불일치');
                    let digits = value === 0 && precision === 0 ? '' : String(Math.abs(value)); if (precision !== null) digits = digits.padStart(precision, '0');
                    const sign = value < 0 ? '-' : match[1].includes('+') ? '+' : match[1].includes(' ') ? ' ' : '';
                    if (match[1].includes('0') && !match[1].includes('-') && precision === null) digits = digits.padStart(Math.max(0, width - sign.length), '0');
                    part = sign + digits;
                } else { if (typeof value !== 'string') throw new Error('Fort 사제 수 CRT 문자열 인자 불일치'); part = precision === null ? value : value.slice(0, precision); }
                output += match[1].includes('-') ? part.padEnd(width, ' ') : part.padStart(width, ' ');
            }
        }
        if (output.length >= 256) throw new RangeError('Fort 사제 수의 원문256BYTE 범위');
    }
    return Uint8Array.from(output + '\0', (character) => character.charCodeAt(0));
}

// 같은 실제 Template·World Runtime과 현재 사제/섬 부모의 결선 및 대여 수명이다.
export class RawCampaignSessionInstall {
    // 현재 owner와 실패 접두/반복/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // C++ 값형 install 훅과 같은 Template9 참조이며 이후 서비스 표 변경과 구별한다.
    hooks = null; data = null;
    // 생성 자체로 Template나 World를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 캠페인의 실제 지연 파일 선택/이미 읽은 이미지/논리 Fort 선택 뒤 같은 Runtime만 만든다.
    bind(fort) {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.terrainModules?.ready || this.#started) throw new Error('Session 설치의 초기화 전/반복 결선');
        this.#started = true;
        if (o.services.missionFortFileSelect && o.services.localLogin) {
            if (o.services.missionFortImage) throw new Error('지연 미션 파일 선택에 선행 이미지가 함께 전달됨');
        } else if (o.services.missionFortImage) {
            o.templateStorage.installFile(1, fort, o.services.missionFortImage, o.services.missionFortFileName || o.missionFortName, c.types.length); o.services.missionFortImage = null;
        } else o.templateStorage.install(1, fort, o.missionFortName);
        const hooks = { ...o.services.install }; this.hooks = hooks;
        // 각 외부 UI 함수는 이 값형 표에만 채우고 원래 services.install은 바꾸지 않는다.
        for (const name of ['translateYou', 'setConfig', 'refreshWindowTitle', 'resetCamera']) if (!hooks[name]) hooks[name] = missing(3476);
        Object.assign(hooks, {
            preparePriests: () => this.preparePriests(), decoderDiagnostics: o.metadata.geometry.decoderDiagnostics, builderAssertion: missing(3478), territoryAssertion: missing(3478),
            notifySurface: (sid) => { this.#live(); c.building.notifySurface(sid); }, setState: (sid, value) => { this.#live(); o.assetBindings.setState(sid, value); },
            populateIsland: (builder) => this.populateAlternate(builder), virtualCount: (sid) => { this.#live(); return o.rewardCount(sid) | 0; },
        });
        const data = { headers: o.headers, handle: o.handle, templateAt: (slot) => { this.#live(); return o.templateStorage.template(slot); }, territoryAt: (slot) => { this.#live(); return o.templateStorage.selectTerritory(slot); }, campaignObjects: (objects, slot) => { this.#live(); o.readFortObjectStreams(objects, slot); } }; this.data = data;
        o.runtime = new RawCampaignWorldRuntime(o.world, o.chunks, o.islands, o.player, o.normal, o.random, o.hp, o.ownership, o.finish, o.contained, o.objectState, o.globals, reference(o, 'piece'), reference(o, 'noRegion'), data, hooks);
        this.#ready = true;
    }

    // 실제 공용 단일 배치와 현재 장기 전역을 빌리는 지역 finish를 만든다. 각 호출의 body가 끝날 때까지 같은 객체를 유지한다.
    #placement(label, invalidType = missing(3400)) {
        const o = this.#owner; const c = o.core; const hooks = makeNormalTerrainFarmWorldHooks(o.world, o.chunks, o.islands, o.pipeline, o.hp);
        hooks.mayPlace = (query) => { this.#live(); return o.constructionModules.mayPlace(query); };
        const single = new RawFortRandomPlacement(c.pool, c.hash, c.spots, c.types, c.frames, o.random, o.randomPlacement, hooks);
        const finish = new RawFortFinishOwner(c.pool, c.types, o.state.assetState.currentResidences(), o.finishState, makeFortSinglePlacementHook(c.pool, single, {
            regionBounds: (region) => o.islands.bounds(region), wallSeconds: () => need(o.services.sound.wallSeconds, `${label}의 실제 벽시각 미연결`)(), invalidType,
        })); finish.bindPlayerRecords(o.players); return finish;
    }

    // WithIslandPopulation의 실제 권한/TYPE 참조와 모든 World/HP/소유권/생산 효과다.
    withPopulation(label, body) {
        this.#live(); const o = this.#owner; const c = o.core; const finish = this.#placement(label);
        const inputs = { boss: () => o.gameplayGlobals.authority().boss, fort: () => c.ownerMode.fort, residence: () => o.finishState.residenceType, primary: () => o.tutorialArcherType, fallback: () => o.templeProduct, replacement: () => o.rewardState.types.priest };
        const hooks = {
            assertion: (reason) => { throw new Error(`${label} 원문 population 진단:${reason}`); }, wallSeconds: () => need(o.services.sound.wallSeconds, `${label} 교체의 실제 벽시각 미연결`)(),
            position: (sid) => c.neighbors.center(sid), destroy: (sid, flags) => o.world.destroy(sid, flags), create: (type, flags) => o.world.create(type, flags),
            defaultFrame: (type) => { if (type < 70 || type - 70 >= o.assets.game.types().length) throw new RangeError(`${label} 현재 TYPE 기본 프레임 범위`); return o.assets.game.types()[type - 70].definition.defaultFrame; },
            typeHitPoints: (type) => o.hp.typeHitPoints(type), hasHitPoints: (sid) => o.hp.hasHitPoints(sid), setOwner: (sid, owner) => o.ownership.setOwner(sid, owner), pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags),
            refreshProduction: () => need(o.services.ownership.refreshProduction, `${label} 실제 생산 통지 미연결`)(),
        };
        return body(new RawIslandPopulation(c.pool, o.chunks, o.chunks, o.islands, finish, inputs, hooks));
    }

    // 초기 사제 배치는 같은 ChunkMap 경계의 DWORD 감김16배/float32 뒤 실제 ReplaceInitial을 사용한다.
    populateInitial(region) {
        return this.withPopulation('Fort 사제 준비', (fill) => {
            const rect = this.#owner.chunks.bounds(region); const cell = (value) => Math.fround(value << 4);
            fill.replaceInitial({ left: cell(rect.left), top: cell(rect.top), right: cell(rect.right), bottom: cell(rect.bottom) }, region);
        });
    }

    // 원문 alternate Fort는 임시 rainFactory 배치→섬0 채우기→임시 객체 가상 삭제 순서다.
    populateAlternate(builder) {
        this.#live(); const o = this.#owner; const c = o.core; const rect = builder.bounds;
        if (rect.left === -1 || rect.top === -1 || rect.right === -1 || rect.bottom === -1) need(o.services.ai.assertion, 'Fort 섬 채우기의 실제 진단 미연결')('cRect.isValid()');
        const finish = this.#placement('Fort 섬 채우기 AddRandom', () => need(o.services.player.log, 'Fort 섬 채우기 AddRandom의 실제 잘못된 타입 로그 미연결')(0x1d, 'AddRandom: Invalid Type\n'));
        const created = addRandomObjects(o.gameplayGlobals.gameAux().secondTimerFactoryType, 1, {
            wallSeconds: () => need(o.services.sound.wallSeconds, 'Fort 섬 채우기 AddRandom의 실제 벽시각 미연결')(), local: () => o.playerState.battleOwner,
            island: (owner, region) => findFortTerritoryIsland(o.islandBytes, c.ownerMode.fort, owner | 0, region), place: (random, rectangle, region, count, type, force) => finish.placeRandom(random, rectangle, region, count, type, force),
        });
        this.withPopulation('Fort 섬 채우기', (fill) => fill.populate(0));
        if (created) { if (created > 65535 || created >= c.pool.capacity) throw new RangeError('Fort 섬 채우기 임시 객체의 현재 SID 물리 범위'); o.world.destroy(created, 0); }
    }

    // 현재 모드가 선택한 실제 기술/사제 목록/Template 부모를 실행한다. 미션의 자연 조기는 하위 원문 몸체가 처리한다.
    preparePriests() {
        this.#live(); const o = this.#owner; const c = o.core;
        // 전체 DWORD를 실제 Pool에 접근할 때만 WORD/용량 범위로 검증한다.
        const check = (sid) => { if (sid > 65535 || sid >= c.pool.capacity) throw new RangeError('Fort 사제 준비 현재 SID 범위'); return sid; };
        // 현재 typed 역할은 source map.at처럼 누락 주소를 기본 TYPE로 대체하지 않는다.
        const role = (address) => { if (!o.tutorialTypes.has(address)) throw new RangeError('Fort 사제 준비 현재 TYPE 전역 미연결'); return o.tutorialTypes.get(address); };
        const hooks = {
            baseType: (index) => index === 0 ? o.templeProduct : index === 1 ? o.tutorialArcherType : role(0x541258), themeType: (theme) => role(0x541184 + theme * 4), technology: () => o.player.currentTechnology(),
            contains: (parent, type) => { const found = o.priests.carrier.findContained(check(parent), 4, type, 0); return found ? c.pool.slot(found)[18] | (c.pool.slot(found)[19] << 8) : 0; },
            createTechnology: (parent, type) => { o.contained.create(check(parent), type, 0, 4, 0x10, 0); },
            templatePresent: (owner) => { if (owner >= 9) throw new RangeError('Fort 사제 준비 현재 Template owner'); return dword(o.headers, owner * 20) !== 0; }, state: (owner) => o.templateState.get(owner), territory: (owner, bytes) => o.runtime.readTerritory(owner, bytes),
            island: (owner, index) => findFortTerritoryIsland(o.islandBytes, c.ownerMode.fort, owner | 0, index | 0),
            region: (x, y) => { if (!o.motion) throw new Error('Fort 사제 준비의 실제 지형 이동 부모 미연결'); const snap = (value) => integer(Math.fround(integer(value + Math.fround(0.99999)))); return o.motion.terrain.regionAt(snap(x), snap(y)) | 0; },
            setOwner: (sid, owner) => o.ownership.setOwner(check(sid), owner), populate: (region) => this.populateInitial(region),
            tellPriests: (count) => { const translate = need(o.services.editorPlayerUi?.translate, 'Fort 사제 수의 실제 번역/설정 장치 미연결'); const config = need(o.services.install.setConfig, 'Fort 사제 수의 실제 번역/설정 장치 미연결'); config('global.priests', formatFortPriestCount(translate('%d Priest%s'), count)); },
        };
        // 기술 생성/소유자 효과 뒤에도 원문 부모가 다시 읽는 동일 현재 전역이다.
        const globals = { get missionPost() { return o.globals.missionPost; }, get editor() { return o.globals.editor; }, get subscriber() { return o.globals.mySubscriber; }, get templateOwner() { return o.playerState.templateOwner; }, get playerOwner() { return o.playerState.battleOwner; } };
        new RawFortPriestPreparation(c.pool, o.priestState.priests, globals, hooks).run();
    }

    // 하위 효과는 정상 생성 복귀 뒤에만 사용하며 반납 뒤의 대여를 거부한다.
    #live() { if (!this.#ready || this.#disposed) throw new Error('반납/미연결 Session 설치 부모'); }
    // Host 정리가 끝난 뒤 현재 Runtime의 큰 후단 대여만 닫는다.
    dispose() { this.#disposed = true; }
    // Template 선택과 실제 WorldRuntime 구성 완료이며 Load/외곽 Session 완료가 아니다.
    get ready() { return this.#ready; }
}
