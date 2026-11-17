// ConnectCursor 3596..3624의 현재 설정·수명·마나/사거리 앞단이다(225553df).
import { SquidForms } from './squid-forms.js';
import { bindCursorManaLifecycle } from './raw-cursor-mana.js';
import { bindCursorRangeLifecycle } from './raw-cursor-range.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { projectCameraCoordinates } from '../render/camera-view.js';

// 같은 Session의 커서 함수 표와 반납 수명이다. 커서 본체 설치는 후속이다.
export class RawCampaignSessionCursor {
    // 원본 owner와 부분 설치·반납 관문이다.
    #owner; #started = false; #ready = false; #placementReady = false; #disposed = false;
    // 후속 커서 본체가 값으로 복사할 실제 게임 함수 표다.
    game = null;

    // 생성은 World나 프로세스를 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납이나 부분 실패 이후에는 빌려준 함수를 호출하지 않는다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 커서'); }

    // 현재 설정·소리·Form 수명과 같은 Host의 프로세스를 원문 순서로 연결한다.
    bindFront() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || o.cursor || !o.gameplayPipeline || !o.buildingMover || !o.cursorMana || !o.cursorRange) throw new Error('Session 커서 앞단 설치 순서/중복/반납 오류');
        this.#started = true; const game = { lifecycle: {}, placement: {} }, life = game.lifecycle;
        if (o.services.production.effects.configInt) game.productTimers = () => { this.#live(); return o.services.production.effects.configInt('useProductTimers') >>> 0; };
        game.unitRate = () => { this.#live(); return o.battleOptions[2]; };
        game.prepare = () => {
            this.#live(); o.assetModules.prepareConstruction(); o.state.gameplayPlacement.geometry.patternTypes = o.state.constructionState.confirm.patternTypes;
            o.cursorDrop.localPlayer = o.playerState.battleOwner; o.cursorDrop.battle = c.ownerMode.battle ? 1 : 0; o.cursorDrop.blocked = o.globals.missionPost;
        };
        life.source = (form) => { this.#live(); return new SquidForms(c.pool).parent(form); };
        life.sourceNotice = (parent, form, reason) => { this.#live(); o.contentNotice.notice(parent, form, reason); };
        // 네 실제 Form 생성자 가상표를 현재 슬롯과 비교한다. 수정 가능한 TYPE은 사용하지 않는다.
        const commonForm = (sid) => {
            const raw = c.pool.slot(sid), table = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true), forms = new SquidForms(c.pool);
            // 원문의 네 Form 가상표만 같은 공통 몸체를 공유한다.
            for (const type of [1, 2, 3, 6]) if (table === forms.vtable(type)) return true;
            return false;
        };
        life.detach = (sid, flags) => { this.#live(); if (commonForm(sid)) c.factory.unpopForm(sid); else o.world.unpop(sid, flags); };
        life.unpop = (sid, flags) => {
            this.#live(); const raw = c.pool.slot(sid); if (commonForm(sid)) { c.factory.popForm(sid, flags); return; }
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); o.world.pop(sid, view.getFloat32(14, true), view.getFloat32(18, true), flags);
        };
        life.destroy = (sid, flags) => { this.#live(); if (!o.world.destroy(sid, flags)) throw new Error('커서 실제 가상 삭제 거부'); };
        life.addLocalSp = (delta) => { this.#live(); o.hp.addSp(o.playerState.battleOwner, delta); };
        life.assertion = (expression, _file, line) => { this.#live(); if (typeof o.services.ai.assertion !== 'function') throw new Error(`커서 취소 원문 진단 장치 미연결 ${line}`); o.services.ai.assertion(expression); };
        life.sound = (name) => { this.#live(); if (typeof o.services.sound.play !== 'function') throw new Error('커서 실제 소리 장치 미연결'); o.services.sound.play(o.soundNames.lookup(name), 0, 0, 0, 0, 0); };
        game.playDrop = () => life.sound('droppiece.wav');
        bindCursorManaLifecycle(life, c.processes, c.kernel, o.cursorMana); bindCursorRangeLifecycle(life, c.processes, o.cursorRange);
        // 기존 Host 어댑터가 만든 함수도 이전 Session 반납 뒤에는 접근하지 않는다.
        for (const name of ['createMana', 'removeProcess', 'processEvent', 'createRange']) { const borrowed = life[name]; life[name] = (...args) => { this.#live(); return borrowed(...args); }; }
        this.game = game; this.#ready = true;
    }

    // 현재 에너지·배치·카메라와 건설 운반을 커서 본체가 빌릴 함수 표에 연결한다.
    bindPlacement() {
        this.#live(); const o = this.#owner, c = o.core, game = this.game, placement = game.placement;
        if (this.#placementReady || o.cursor) throw new Error('Session 커서 배치 중복 연결');
        placement.describe = (text, x, y, type, owner) => { this.#live(); const result = o.energySupply.evaluate(text.value, x, y, type, owner); text.value = result.output; return result.value; };
        placement.source = game.lifecycle.source;
        placement.region = (sid) => { this.#live(); const graph = o.anchor.graphFor(sid); return graph === 254 ? 0 : (graph + 1) | 0; };
        placement.mayPlace = (query) => {
            this.#live(); const result = o.constructionModules.mayPlace(query);
            if (!o.cursor) throw new Error('커서 배치의 실제 본체 미설치');
            o.cursor.placement.obstruction = o.state.gameplayPlacement.placement.blockedRelation; return result ? 1 : 0;
        };
        placement.processEvent = game.lifecycle.processEvent;
        placement.project = (x, y) => { this.#live(); const p = o.gameplayGlobals.camera().snapshot().pixels, result = projectCameraCoordinates(p.x, p.y, x, y); return { x: result[0], y: result[1] }; };
        game.removeRange = (parent, type) => {
            this.#live(); new RawContainedLookup(c.pool, (target, flags) => { if (!o.world.destroy(target, flags)) throw new Error('회전 사정거리 실제 가상 삭제 거부'); }).find(parent, type, 0, true);
        };
        game.moveBuilder = (owner, target, low, high) => { this.#live(); o.buildingMover.move(owner, target, low, high); };
        this.#placementReady = true;
    }

    // 반납 뒤 함수 표가 같은 World에 다시 들어가지 못하게 한다.
    dispose() { this.#disposed = true; }
    // 앞단 정상 복귀이며 커서 본체나 실제 Load 완료를 뜻하지 않는다.
    get frontReady() { return this.#ready; }
    // 배치 함수 표 정상 복귀이며 실제 커서 본체 설치와는 구별한다.
    get placementReady() { return this.#placementReady; }
}
