// Session InstallContents의 실제 통지/공통 삭제 접두3020..3049를 같은 owner에 연결한다(225553df, WB04-E596).
// 사용: connectWorldRuntimeHooks 뒤 bindNotices. 내용물 수명/생산과 최종 World 공개는 후속 구간이다.
import { GumpFormHost } from './gump-form-host.js';
import { RawPlayerContentNotice } from './raw-player-content-notice.js';
import { RawContentParentNotice } from './content-parent-notice.js';
import { RawPriestTakenNotice } from './raw-priest-taken-notice.js';
import { bindSpStoreNotice } from './raw-sp-store-notice-bindings.js';
import { makeCampaignDeletionEffects } from './raw-campaign-lifetime-bindings.js';

// 원문 FillMissing의 선택된 호출에서만 실패하는 실제 미연결 장치다.
function missing(line) { return () => { throw new Error(`실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:${line}`); }; }
// SP의 DWORD 비트를 원문 bit_cast<float>처럼 해석한다. 수치 정수 변환을 하지 않는다.
function spFloat(bits) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setUint32(0, bits, true); return bytes.getFloat32(0, true); }
// 선택 지점의 현재 서비스만 요구한다. 생성 때 미선택 장치를 실행하지 않는다.
function need(fn, message) { if (typeof fn !== 'function') throw new Error(message); return fn; }

// 같은 Impl의 Gump/Player/내용물/포획 통지 부모를 소유하는 초기화 단계다.
export class RawCampaignSessionContents {
    // 부모 참조와 부분 실패/반복/반납 관문이다. 수명/생산 준비는 별도 완료값으로 다룬다.
    #owner; #started = false; #noticesReady = false; #disposed = false;
    // 실제 포획 통지 객체가 빌리는 값형 훅이다. 서비스는 호출 때 조회한다.
    takenHooks = null;

    // 생성은 World/Player 상태를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 원문 순서대로 Gump 등록→삭제→Player/부모 통지→현재 Form 상태→포획 통지를 설치한다.
    bindNotices() {
        const o = this.#owner; const c = o.core; const g = o.gameplayGlobals;
        if (this.#disposed || !o.runtimeModules?.ready || this.#started) throw new Error('Session 내용물 통지의 초기화 전/반복 결선');
        this.#started = true;
        o.gumpForms = new GumpFormHost(c.pool, c.factory, c.types, c.destroy, {
            notice: (owner, parent) => { this.#live(); need(o.services.gumpNotice, '현재Session GumpForm UI통지미연결')(owner, parent); },
            fallback: c.forms.hooks(),
        });
        c.forms.bind(o.gumpForms);
        if (!o.services.unitLost) o.services.unitLost = missing(3025);
        o.world.bindDeletion(o.hp, o.worldHooks.assets.selected, makeCampaignDeletionEffects(o.worldHooks.assets, o.services.unitLost));
        o.world.deletionState.silentType = 147;
        const hooks = { ...o.services.playerContent, money: (owner) => { this.#live(); return spFloat(o.sp.get(owner)); } };
        if (!hooks.gump) hooks.gump = missing(3030); if (!hooks.handle) hooks.handle = missing(3030);
        const reads = bindSpStoreNotice(o.sp); const patch = o.priestState.regen.patch;
        o.playerContent = new RawPlayerContentNotice(c.pool, {
            ...reads,
            // 진단 상자와 남은 횟수는 같은 주소에 쓰며 그 밖의 전역은 현재 조회기다.
            get problemCode() { return g.patchDiagnostic().value; }, set problemCode(value) { g.patchDiagnostic().value = value; },
            fallbackCode: () => patch.readSample(),
            get moneyChecks() { return patch.remaining; }, set moneyChecks(value) { patch.remaining = value; },
            battleShown: () => o.playerState.playerKnown, localPlayer: () => o.playerState.battleOwner,
            gumpFormType: () => o.contentGumpType, contentEvent: () => o.contentEvent,
        }, hooks);
        o.contentNotice = new RawContentParentNotice(c.pool, (parent, form, reason) => {
            this.#live(); if (!o.lifetime) throw new Error('현재 내용물 수명 부모 미설치'); o.lifetime.parentNotice(parent, form, reason);
        });
        o.contentFormState = {
            contentType: () => g.finder.containedType, inBattle: () => c.ownerMode.battle, checkingLinks: () => g.aiDebug().checking().value,
            assertion: (expression, file, line) => { this.#live(); need(o.services.ai.assertion, `ContentForm 실제 진단 장치 미연결 ${line}`)(expression); },
        };
        this.takenHooks = {
            refresh: (owner) => { this.#live(); o.priestParticipation.refresh(owner); },
            wallSeconds: () => { this.#live(); return need(o.services.sound.wallSeconds, '포획 안내 실제 벽시각 미연결')(); },
            name: (owner) => {
                this.#live(); const fn = need(o.services.priestTakenName, '포획 안내 실제 Player 이름 미연결'); const end = o.handle.indexOf(0);
                if (end < 0) throw new Error('포획 안내 현재 이름 NUL 누락');
                return fn(owner, Array.from(o.handle.subarray(0, end), (byte) => String.fromCharCode(byte)).join(''));
            },
            sound: (name) => { this.#live(); const play = need(o.services.sound.play, '포획 안내 실제 전역 음향 미연결'); play(o.soundNames.lookup(name), 0, 0, 0, 1, 0); },
            tell: (section) => { this.#live(); need(o.services.factoryTell, '포획 안내 실제 Tell 미연결')(section); },
        };
        o.priestTakenNotice = new RawPriestTakenNotice(c.pool, () => o.playerState.battleOwner, g.priestTakenDeadlines(), this.takenHooks);
        this.#noticesReady = true;
    }

    // 초기화 중 생성 효과도 허용하되 반납 뒤 자기 closure는 거부한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 내용물 통지'); }
    // World 정리 뒤 부모가 한 번 무효화한다. 등록된 Form을 다른 풀로 옮기지 않는다.
    dispose() { this.#disposed = true; }
    // InstallContents 전체가 아니라 통지 접두의 정상 복귀 여부다.
    get noticesReady() { return this.#noticesReady; }
}
