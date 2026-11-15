// Session 생성자1340..1397의 피해 상태·AI 통지·실제 Hit 효과를 같은 owner에 연결한다(225553df, WB04-E581).
// 사용: owner.connectHitBodies(). GunHit/투사체·나머지 생성자·전체 Load/공개는 후속 구간이다.
import { RawAiDamage } from './raw-ai-damage.js';
import { RawPlayerDamageNotice } from './raw-player-damage-notice.js';
import { RawCampaignHitBindings } from './raw-campaign-hit-bindings.js';

// 현재 실제 모듈이 설치된 뒤에만 선택한 하위를 실행한다. 대체 몸체를 만들지 않는다.
function module(owner, key, label) { if (!owner[key]) throw new Error(`${label} 실제 설치 전 호출`); return owner[key]; }
// 원문 FillMissing이 남기는 선택 지점의 명시 오류다.
function missing(label) { return () => { throw new Error(`Session 피격의 실제 ${label} 미연결`); }; }
// 실제 Player/AI 기록의 비정렬 DWORD를 signed/unsigned 해석 전에 읽는다.
function read(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }

// 같은 피해 상태/HP·AI 메모리와 외부 효과 묶음의 생성 순서다. 전체 Session을 공개하지 않는다.
export class RawCampaignSessionHits {
    // 실제 기반 owner와 단일 실행/정상 복귀/반납 상태다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 지역 효과의 보관이다. 실제 Bindings는 이 묶음의 함수/참조를 값 인수로 복사한다.
    effects = null;
    // 같은 기반 자료만 빌리며 실제 생성은 bind에서 수행한다.
    constructor(owner) { this.#owner = owner; }
    // 총/커서 구간 뒤 피해 상태 getter→AI 통지→효과 기본값→실제 Hit Bindings 순서다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.gunViewModules?.ready || this.#started) throw new Error('Session 피격의 초기화 전/반복 결선');
        this.#started = true;
        s.priestDamageState.levelCorrectionDisabled = o.globals.missionPost;
        s.priestDamageState.levelCorrectionDisabledRead = () => o.globals.missionPost;
        s.generalDamageState.serverRead = () => c.processState.server;
        s.generalDamageState.localOwnerRead = () => o.playerState.battleOwner & 0xff;
        s.generalDamageState.primaryIslandRead = (owner) => read(o.players, (owner & 0xff) * 0xb4 + 0x78) | 0;
        const input = o.services.hit;
        const h = { ...input, priest: { ...input.priest }, general: { ...input.general }, burning: { ...input.burning }, vortex: { ...input.vortex }, dudes: { ...input.dudes } }; this.effects = h;
        h.priest.defaultFrame = (sid) => o.projectileTypes[c.pool.slot(sid)[10]].defaultFrame;
        o.aiDamage = new RawAiDamage(o.aiMemory, c.pool, c.types, {
            home: (sid) => module(o, 'flyer', 'AI 피격의 실제 비행 홈 조회').home(sid),
            log: (token, value) => {
                if (value.type >= c.types.length) throw new RangeError('AI 피격 로그 현재 TYPE 범위 오류');
                const text = `I hate ${value.more ? 'MORE ' : ''}${c.types[value.type].name} at (${value.x},${value.y})\n`;
                const raw = o.aiMemory.bytes(token); if (!read(raw, 0xb00)) return;
                if (typeof o.services.player.log !== 'function') throw new Error('AI 피격 실제 로그 출력 소비자 미연결');
                o.services.player.log(29, `AI${read(raw, 0x260) | 0}: ${text}\n`);
            },
        });
        o.playerDamage = new RawPlayerDamageNotice(o.players, c.ownerMode, (token, victim, attacker) => { o.aiDamage.notice(token, victim, attacker); });
        h.priest.ownerDamageNotice ??= (owner, victim, attacker) => { o.playerDamage.notice(owner, victim, attacker); };
        h.general.ownerDamageNotice ??= (owner, victim, attacker) => { o.playerDamage.notice(owner, victim, attacker); };
        h.general.realTime ??= o.services.sound.wallSeconds;
        h.general.warningSound ??= () => {
            if (typeof o.services.sound.play !== 'function') throw new Error('실제 섬 피격 전역 음향 소비자 미연결');
            o.services.sound.play(o.soundNames.lookup('islandUnderAttack.wav'), 0, 0, 0, 1, 0);
        };
        h.general.warningText ??= () => {
            if (typeof o.services.priest.shield.tell !== 'function') throw new Error('실제 섬 피격 Tell 소비자 미연결');
            o.services.priest.shield.tell('islandUnderAttack');
        };
        h.general.realTime ??= missing('벽시각'); h.general.warningEvent ??= missing('경고 이벤트');
        const remove = h.general.warningEvent; h.general.warningEvent = () => { if (!o.globals.missionPost) remove(); };
        h.burning.assertion ??= (expression, _file, line) => {
            if (typeof o.services.ai.assertion !== 'function') throw new Error(`burning 실제 진단 장치 미연결 ${line >>> 0}`);
            o.services.ai.assertion(expression);
        };
        h.burning.create ??= (victim) => { const process = module(o, 'particles', '피격 Burning').addBurning(victim, 1); return process ? process.form : 0; };
        h.burning.bulfSound ??= (_sid, x, y) => {
            if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('불프 피격 위치 음향 미연결');
            o.services.sound.playOnceAt(x, y, o.soundNames.lookup('bulfHurt.wav'), 0, 0);
        };
        h.vortex.constructionType ??= () => s.buildingState.currentType();
        h.vortex.resetEnd ??= (form, amount) => {
            if (typeof o.prepareConstruction !== 'function') throw new Error('피격의 실제 PrepareConstruction 설치 전 호출');
            o.prepareConstruction(); module(o, 'building', '피격의 실제 건설 기한').resetEndFromForm(form, amount);
        };
        h.vortex.refresh ??= (victim) => { c.display.update(c.pool.slot(victim), 0); };
        h.dudes.createBurning ??= (victim) => { module(o, 'particles', 'Dude 피격 Burning').addBurning(victim, 0); };
        h.dudes.pathEndpoint ??= (sid) => {
            const path = module(o, 'motion', 'Dude 현재 경로 조회 전 이동').paths.currentPath(sid); if (!path) return null;
            const runtime = path.runtime; const point = runtime.point(runtime.count - 1); return [point.x, point.y];
        };
        h.dudes.startPath ??= (sid, x, y) => module(o, 'motion', 'Dude 대피 시작 전 이동').paths.startDude({ parent: sid, destination: { x, y } }) !== null;
        o.hits = new RawCampaignHitBindings(o.world, o.priests, o.hp, o.random, o.dudeSpeeds, s.assetState.currentResidences(),
            s.priestDamageState, s.generalDamageState, s.burningState, s.impactState, h);
        this.#ready = true;
    }
    // 부모가 현재 World 프로세스를 먼저 정리한 뒤 단계의 재사용만 금지한다. 하위 몸체는 부모와 함께 해제된다.
    dispose() { this.#disposed = true; }
    // 이 생성 구간의 정상 복귀이며 전체 Session/교전/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
