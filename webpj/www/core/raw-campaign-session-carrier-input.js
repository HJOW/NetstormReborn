// 같은 캠페인 Session의 운반체 사용자 입력을 현재 지형·내용물·이동·음향에 연결한다.
// 사용: new RawCampaignSessionCarrierInput(() => 살아 있는 owner).make(외곽 UI 훅). 원본 RawCampaignCarrierInput.inc의 10.78 경로다.
import { typeString } from '../assets/type-table.js';
import { CarrierInputState, RawCarrierInput } from './raw-carrier-input.js';
import { RawCarrierDropPoint } from './raw-carrier-drop-point.js';
import { RawCursorPosition } from './raw-cursor-position.js';
import { geyserTypeNow } from './raw-carrier-arrival.js';
import { captureCampaignFlag } from './raw-campaign-session-state.js';

// 같은 Session 수명에서 운반 입력의 전역·음향 표·투하 위치 상태를 보존한다.
export class RawCampaignSessionCarrierInput {
    // 현재 owner 조회와 원본 입력 전역·TYPE별 음향·투하점 전역이다.
    #live; #state = new CarrierInputState(); #audio = Array.from({ length: 188 }, () => ({ moveSound: 0, dropSound: 0 }));
    #drop = { mapLimit: 256, invalidPoint: [0, 0] };
    // 외곽이 반납되면 기존 입력 콜백도 같은 owner 조회에서 거부한다.
    constructor(live) { this.#live = live; }

    // 설치된 실제 Session의 사용자 입력 몸체를 생성한다. UI 장치는 도달할 때만 요구한다.
    make(input = {}) {
        const o = this.#live(), c = o.core;
        if (!o.loaded) throw new Error('설치 전 실제 운반체 사용자입력 요청');
        // 같은 TYPE 자산에 지정된 이동·투하 음향 핸들을 원본 순서로 읽는다.
        for (const asset of o.assets.game.types()) {
            const audio = this.#audio[70 + asset.block], move = typeString(asset.definition, 'moveSound'), drop = typeString(asset.definition, 'dropSound');
            audio.moveSound = move ? o.soundNames.lookup(move) : 0; audio.dropSound = drop ? o.soundNames.lookup(drop) : 0;
        }
        this.#state.priestType = o.priestState.movement.priest;
        const golem = o.assets.game.typeTable.find('sunWalker'); if (golem < 0) throw new Error('운반체 입력 골렘 타입 누락');
        this.#state.golemType = golem; this.#drop.mapLimit = o.cursorManaGlobals.mapLimit | 0;
        const live = this.#live, point = new RawCarrierDropPoint(c.pool, c.types, this.#drop, {
            mayPlace: (query) => { const current = live(); current.assetModules.prepareConstruction(); return current.constructionModules.mayPlace(query); },
            center: (sid) => live().gunTarget.center(sid),
        });
        const play = (sound) => { const current = live(), output = current.services.sound.play; if (typeof output !== 'function') throw new Error('운반체 TYPE 음향 실제 장치 미연결'); output(sound, 0, 0, 0, 1, 0); };
        const checking = captureCampaignFlag(o.state.gunState, 'checking'), external = { ...input };
        // 외곽 UI 함수도 Session 반납 뒤 이전 World를 다시 사용하지 못하게 한다.
        for (const [name, callback] of Object.entries(external)) if (typeof callback === 'function') external[name] = (...args) => { live(); return callback(...args); };
        const hooks = { ...external,
            immobile: (sid) => live().priests.priestState.groundImmobile(sid),
            content: (sid, mask) => live().priests.carrier.findContained(sid, mask, 0, 0),
            candidate: (sid, p) => { const found = live().motion.carrierArrival.pickupCandidate(sid, p.x, p.y); return { sid: found.sid, nugget: found.flag }; },
            dropPoint: (sid, type) => { live(); return point.find(sid, type); },
            graph: (sid) => live().anchor.graphFor(sid),
            connected: (a, b) => { const current = live(); return current.anchor.graphAt(a.x, a.y) === current.anchor.graphAt(b.x, b.y); },
            surface: (p) => { const current = live(), cell = RawCursorPosition.truncatePoint([p.x, p.y]), x = cell[0] | 0, y = cell[1] | 0; if (x < 0 || x >= 256 || y < 0 || y >= 256) throw new RangeError('운반체 입력 spot 물리 범위'); return current.core.spots[y * 256 + x]; },
            geyser: (p) => { const current = live(); return current.world.find(p.x, p.y, geyserTypeNow(current.state.motionState.carrier)); },
            validGeyser: (sid, target) => live().motion.carrierArrival.validGeyser(sid, target),
            returnPlace: (p) => live().motion.cleanup.returnPlaceAt(p.x, p.y, 0),
            daisAt: (p) => live().damageablePost.genusAt(p.x, p.y, 0x400000),
            altarAllowed: (dais, priest) => live().sacrificeRequest.allowed(dais, priest),
            localOwner: () => live().playerState.battleOwner, checking: () => { live(); return checking(); },
            earlySacrifice: () => { if (live().globals.missionPost) return false; throw new Error('일반전투 희생 제한시간 부모 미연결'); },
            showSacrifice: (dais) => { if (live().globals.missionPost) return; if (typeof external.showSacrifice !== 'function') throw new Error('일반전투 희생 메뉴 소비자 미연결'); external.showSacrifice(dais); },
            select: (sid, force, activate) => { const selection = live().activeSelection?.deref()?.selection; if (!selection) throw new Error('운반체 입력의 현재 선택 수명 미연결'); selection.select(sid, force, activate); },
            request: (sid, p, sacrifice) => live().carrierMove.request(sid, p, sacrifice),
            pickup: (sid) => live().carrierPickup.request(sid),
            dropCargo: (sid, content) => { const current = live(); if (content > 65535) throw new RangeError('Cast/Drop 내용물 WORD 범위'); current.prepareCarrierDrop(); current.carrierDrop.request(sid, content); },
            random: (low, high) => { if (high <= low) throw new RangeError('이동 음향 정수 난수 구간'); return low + live().random.next(high - low); },
            namedSound: (priest, index) => {
                const names = priest ? ['priestMove1.wav', 'priestMove3.wav'] : ['golemMove1.wav', 'golemMove2.wav', 'golemMove4.wav', 'golemMove5.wav'];
                if (index >= names.length) throw new RangeError('원본 음향 이름 표 밖 인접 DWORD 직접 인덱스'); play(live().soundNames.lookup(names[index]));
            },
            sound: (sound) => play(sound),
        };
        return new RawCarrierInput(c.pool, c.types, this.#audio, o.state.soundState.typeFrames.subarray(0, c.types.length), () => live().state.soundState.frame, this.#state, hooks);
    }
}
