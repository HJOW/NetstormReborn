// cpppj client/SoundProcess.cpp/.h(225553df)의10.78 실행/종료와 실제 Host 부착을 옮긴다.
// 사용: new SoundProcessSystem(host, state, hooks).add(parent, sound, alternate, flags). 여섯 재생/시계 경계는 모두 필요하다.
// SoundPlayer 최소 계약은 playLoopAt/playOnceAt/play/stop/isPlaying/playByName/playNameAt이며 이 모듈은 장치를 만들지 않는다.
// CD/10.37 분기는 제외한다. 실제 Session의 장기 상태·SoundPlayer 장치 설치는 후속 통합이다. (2026-11-16 WB04-E571)
import { SquidProcess } from './squid-process.js';

// 소리 ProcessForm의 초기TYPE와 원문의 낮은 네flag다. 위치 한 번의 priority8은 반복 인자 자리에 그대로 들어간다.
export const SOUND_PROCESS_TYPE = 47, SOUND_PLAY_AT = 1, SOUND_USE_ALTERNATE = 2, SOUND_PLAY_ONCE = 4, SOUND_PLAY_PRIORITY = 8;
// 부모 슬롯에서 재생 때 읽는TYPE·상태WORD·비정렬 float 좌표다.
const TYPE = 10, WORD = 12, X = 14, Y = 18;
// 산술 변환 없이 현재 슬롯 float32를 읽는다.
function coordinate(raw, at) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(at, true); }

// 실제 클라이언트가 소유하는 프레임·TYPE·checking·타입별 마지막 소리 프레임이다.
export class SoundProcessState {
    // 독립 입력DWORD이며 실제 Native는 sharedProcessType의 현재 getter로 같은 GameAux 값을 빌린다.
    frame = 0; processType = SOUND_PROCESS_TYPE; sharedProcessType = null;
    // RawCheckingFlag와 같은 현재DWORD 입력이다. 숫자/bool 또는 매번 현재값을 읽는 함수다.
    debug = 0;
    // 원문 TYPE 레코드+1f0에 해당하는256개 마지막 프레임이다.
    typeFrames = new Uint32Array(256);
    // 부착 순간의 현재 전체TYPE를 읽는다.
    currentType() { return (this.sharedProcessType ? this.sharedProcessType() : this.processType) >>> 0; }
    // 진단 소비 위치마다 같은 현재DWORD를 읽는다.
    isDebug() { return (typeof this.debug === 'function' ? this.debug() : Number(this.debug)) >>> 0; }
}

// 부모의 ProcessForm·Kernel 수명을 그대로 사용하는 소리 프로세스다.
export class SoundProcess extends SquidProcess {
    // 같은 시스템과 원문 필드 current/sound/alternate/flags/startTime이다.
    #system; #current = 0; #sound; #alternate; #flags; #startTime = 0;
    // 생성자는 필드만 채우며 부착은 시스템의 실제 Host가 한다.
    constructor(system, sound, alternate, flags) { super(); this.#system = system; this.#sound = sound >>> 0; this.#alternate = alternate >>> 0; this.#flags = flags >>> 0; }
    // 미부착 예약 객체에 생성 인자를 채운다. Session의 new40 예약→같은 Sound47 부착에 쓰며 이미 Host/PID를 가진 객체는 재사용하지 않는다(WB04-E597).
    initializeUnattached(sound, alternate, flags) {
        if (this.host || this.pid || this.form) throw new Error('Sound47 미부착 예약 객체만 초기화할 수 있습니다');
        this.#sound = sound >>> 0; this.#alternate = alternate >>> 0; this.#flags = flags >>> 0; this.#current = 0; this.#startTime = 0;
    }
    // 재생 callback 뒤 현재 부모TYPE와 프레임을 다시 읽는다.
    #markTypeFrame() { const system = this.#system; system.state.typeFrames[system.pool.slot(this.parent)[TYPE]] = system.state.frame; }
    // 선택된 보고가 정상 복귀하면 원문의 다음 작업을 계속한다.
    #report(expression, line) { this.#system.hooks.report?.(expression, line); }
    // 소리 선택→벽시계→flag/중복 프레임→진단→좌표/재생 순서를 보존한다.
    runFrame() {
        const host = this.host;
        if (!host) return;
        // 원문 const span은 같은 물리를 읽는다. wallSeconds/진단 뒤에도 바뀐 좌표/TYPE를 읽어야 하므로 slot 사본을 쓰지 않는다.
        const { state, hooks } = this.#system, raw = host.pool.physical(this.parent);
        const sound = (this.#flags & SOUND_USE_ALTERNATE) && (raw[WORD] | raw[WORD + 1]) ? this.#alternate : this.#sound;
        const now = hooks.wallSeconds();
        if (!(now >= this.#startTime)) return;
        const playAt = Boolean(this.#flags & SOUND_PLAY_AT), playOnce = Boolean(this.#flags & SOUND_PLAY_ONCE), priority = this.#flags & SOUND_PLAY_PRIORITY;
        const played = state.typeFrames[raw[TYPE]] === (state.frame >>> 0);
        if (state.isDebug()) {
            if (!playOnce && priority) this.#report('playOnce || !playPriority', 0x35);
            if (state.isDebug() && !playOnce && !playAt) this.#report('playOnce || playAt', 0x36);
        }
        if (playAt) {
            const x = coordinate(raw, X), y = coordinate(raw, Y);
            if (!playOnce) {
                if (this.#current !== 0 || !played) this.#current = hooks.playLoopAt(x, y, this.#current, sound) >>> 0;
                if (this.#current !== 0) this.#markTypeFrame();
                return;
            }
            if (!played) { hooks.playOnceAt(x, y, sound, priority, 0); this.#markTypeFrame(); }
        } else if (this.#current === 0 && !played) {
            this.#current = hooks.play(sound, playOnce ? 0 : 1, 0, 0, priority, 0) >>> 0;
            this.#markTypeFrame();
        }
        if (playOnce) host.kill(this, 0);
    }
    // 반복 소리만 정지한다. stop 뒤 current를 재조회하며 여전히 재생 중이면 보고한다.
    onFormDestroy(flags) { if (!this.#current || (this.#flags & SOUND_PLAY_ONCE)) return; const hooks = this.#system.hooks; hooks.stop(this.#current); if ((hooks.isPlaying(this.#current) >>> 0) !== 0) this.#report('!isSoundPlaying( currentSound )', 0x15); }
    // 원문 재생 중인 항목DWORD다.
    get current() { return this.#current; }
    // 외부 관측/재생기가 현재 항목을 지정한다.
    setCurrent(value) { this.#current = value >>> 0; }
    // 원문 기본 소리 항목이다.
    get sound() { return this.#sound; }
    // 원문 대체 소리 항목이다.
    get alternate() { return this.#alternate; }
    // 원문flags 전체DWORD다.
    get flags() { return this.#flags; }
    // 지연 시작의 원문double이다.
    get startTime() { return this.#startTime; }
    // 실제 입력double을 절단하지 않고 저장한다.
    setStartTime(value) { this.#startTime = value; }
    // float 지연만 먼저 단정도로 만들고 벽시각double과 더한다.
    delay(seconds) { this.#startTime = Math.fround(seconds) + this.#system.hooks.wallSeconds(); }
}

// 같은 실제 ProcessHost와 공유 상태/재생 경계를 묶는다.
export class SoundProcessSystem {
    // 재생 효과는 C++ 값 인자처럼 구성 때 묶음을 복사하고 함수가 빌린 실제 owner는 유지한다.
    #host; #state; #hooks;
    // 필요한 여섯 효과가 없으면 프로세스를 만들기 전에 거부한다.
    constructor(host, state, hooks) {
        if (host?.pool?.edition !== 'patch1078' || ['wallSeconds', 'playLoopAt', 'playOnceAt', 'play', 'stop', 'isPlaying'].some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('소리 프로세스 필수 효과/10.78 Host 누락');
        this.#host = host; this.#state = state; this.#hooks = { ...hooks };
    }
    // 현재TYPE로 client Form(flags10)을 실제 부모에 부착한다.
    add(parent, sound, alternate, flags) { return this.#host.attach(new SoundProcess(this, sound, alternate, flags), this.#state.currentType(), parent, 0x10); }
    // 같은 실제 SID 풀이다.
    get pool() { return this.#host.pool; }
    // 프로세스가 공유하는 실제 상태다.
    get state() { return this.#state; }
    // 복사한 효과 묶음이다. 호출자는 구성 후 바꾸지 않는다.
    get hooks() { return this.#hooks; }
}
// 같은 풀의 실제 이름 표 조회와 소리 프로세스 부착만 보호막 효과에 덮어쓴다.
export function makePriestShieldSoundHooks(pool, sounds, system, hooks) {
    if (pool !== system.pool) throw new TypeError('보호막/소리 프로세스 SID 풀이 다릅니다');
    return { ...hooks, loadSound: (name) => sounds.lookup(name), attachSound: (sid, sound, alternate, flags) => { system.add(sid, sound, alternate, flags); } };
}
// 실제 재생기의 다섯 메서드와 벽시계를 원문 인자 순서대로 빌린다. signed 세 인자는 같은32비트로 해석한다.
export function makeSoundProcessHooks(player, wallSeconds, report = null) {
    if (typeof wallSeconds !== 'function') throw new TypeError('소리 프로세스의 벽시계 효과 누락');
    return { wallSeconds, playLoopAt: (x, y, current, sound) => player.playLoopAt(x, y, current, sound), playOnceAt: (x, y, sound, loop, priority) => player.playOnceAt(x, y, sound, loop, priority),
        play: (sound, loop, volume, pan, priority, limit) => player.play(sound, loop, volume | 0, pan | 0, priority, limit | 0), stop: (sound) => player.stop(sound), isPlaying: (sound) => player.isPlaying(sound), report };
}
// 안내 구조체의 다섯 인자를 이름 기반 전역 재생에 그대로 넘긴다.
export function makePriestShieldNoticeHooks(player, hooks) { return { ...hooks, noticeSound: (notice) => { const a = notice.arguments; player.playByName(notice.file, a[0], a[1] | 0, a[2] | 0, a[3], a[4] | 0); } }; }
// 낙하 사제SID는 버리고 좌표만 이름 기반 위치 재생에 넘긴다.
export function makePriestFallSound(player) { return (sid, x, y) => { player.playNameAt(x, y, 'priestFall.wav', 0); }; }
// 건설의 진행 소리 부착/위치 완료음/준비 소리를 같은 이름 표와 재생기로 연결한다.
export function makeConstructionProcessSoundHooks(pool, sounds, system, player, hooks) {
    if (pool !== system.pool) throw new TypeError('건설/소리 프로세스 SID 풀이 다릅니다');
    return { ...hooks, loadSound: (name) => sounds.lookup(name), attachSound: (sid, sound, alternate, flags) => { system.add(sid, sound, alternate, flags); },
        playAt: (x, y, name, loop) => { player.playNameAt(x, y, name, loop); }, play: (sound) => { player.play(sound, 0, 0, 0, 0, 0); } };
}
