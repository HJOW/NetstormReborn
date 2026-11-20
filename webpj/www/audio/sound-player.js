// 10.78 SoundPlayer의 재생·복제·위치·정지와 이름 표 상태를 장치 훅에 연결한다.
// 사용: new SoundPlayer(list, state, hooks, report) → play/playOnceAt/playLoopAt/stop. 장치와 이름 표는 재생기보다 오래 살아야 한다.
import { SOUND_FIELD, SOUND_HEADER_BYTES, SOUND_LIST_BASE } from './sound-list.js';
import { soundOnScreen, soundPan, soundToScreen, soundVolume } from './sound-spatial.js';

// 원본의 파일 없음 버퍼 표식과 DirectSound 상태 비트다.
export const SILENT_SOUND_BUFFER = 0xffffffff;
const PLAYING = 1, LOST = 2, LOOPING = 4;
// 파일명 비교는 C 로캘의 ASCII 대소문자만 접는다.
function foldName(name) { return name.replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32)); }

// 원본 장기 재생 전역의 초기값이다. currentView는 선택적인 현재 지도 조회기다.
export function createSoundState() {
    return { initialized: false, device: false, enabled: false, swapSpeakers: false, maxPlaying: 8, playing: 0, masterVolume: 0,
        volumeHoldDepth: 0, pendingMasterVolume: 0, serial: 0, view: { cameraX: 0, cameraY: 0, left: 0, top: 0, right: 640, bottom: 480 }, currentView: null };
}

// 같은 이름 표·전역·장치 훅을 소유하지 않고 빌려 원본 순서로 실행한다.
export class SoundPlayer {
    // 이름 표와 값형 전역, 장치 및 선택적인 진단 장치다.
    #list; #state; #hooks; #report;
    // 원본이 반드시 사용하는 여덟 장치가 모두 있는지 확인한다.
    constructor(list, state, hooks, report = null) {
        if (!list || !state || ['status', 'play', 'setPosition', 'setVolume', 'setPan', 'stop', 'duplicate', 'load'].some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('소리 재생 계층의 필수 장치 효과 누락');
        this.#list = list; this.#state = state; this.#hooks = hooks; this.#report = report;
    }
    // 원본은 각 사용 지점에서 현재 공용 화면을 다시 조회한다.
    currentView() { return this.#state.currentView ? this.#state.currentView() : this.#state.view; }
    // 현재 표의 DWORD를 검증해 읽는다.
    #get(entry, field) { return this.#list.field(entry, field); }
    // 현재 표의 DWORD를 검증해 쓴다.
    #set(entry, field, value) { this.#list.setField(entry, field, value); }
    // 원본 진단이 연결되어 있으면 조건식과 줄 번호를 전달한다.
    #assert(expression, line) { this.#report?.(expression, line); }
    // 선택적인 원본 기록 장치에 채널과 문장을 전달한다.
    #log(channel, text) { this.#hooks.log?.(channel, text); }
    // 재생 시 원본의 signed32 합과 [-10000, 0] 제한을 사용한다.
    #gain(entry, volume) { return Math.max(-10000, Math.min(0, (volume + this.#state.masterVolume - this.#get(entry, SOUND_FIELD.attenuation)) | 0)); }
    // 소리 없음 표식은 정지 상태이며 빈 버퍼는 원본 진단 뒤 거부한다.
    #playing(entry) {
        const buffer = this.#get(entry, SOUND_FIELD.buffer);
        if (!buffer) { this.#assert('buffer', 0x82); throw new Error('버퍼가 없는 소리 항목의 상태 조회'); }
        return buffer === SILENT_SOUND_BUFFER ? 0 : this.#hooks.status(buffer) & PLAYING;
    }
    // 버퍼 유실 진단 뒤 정지·위치 초기화·재생 수 감소 순서를 보존한다.
    #stopBuffer(entry) {
        const buffer = this.#get(entry, SOUND_FIELD.buffer);
        if (!buffer) { this.#assert('buffer', 0x9b); this.#assert('!isLost()', 0x9c); throw new Error('버퍼가 없는 소리 항목의 정지'); }
        if (buffer === SILENT_SOUND_BUFFER) throw new Error('소리 없음 항목의 정지');
        if (this.#hooks.status(buffer) & LOST) this.#assert('!isLost()', 0x9c);
        this.#hooks.stop(buffer); this.#hooks.setPosition(buffer, 0); this.#state.playing = (this.#state.playing - 1) | 0;
    }
    // 실제 재생 중인 항목만 상태를 다시 읽고 정지한다.
    stop(sound) {
        if (!sound || !this.#get(sound, SOUND_FIELD.buffer) || !this.#playing(sound)) return;
        this.#stopBuffer(sound);
    }
    // 초기화·옵션·버퍼 관문을 거친 현재 재생 여부다.
    isPlaying(sound) {
        if (!this.#state.initialized || !sound || !this.#state.enabled || !this.#get(sound, SOUND_FIELD.buffer)) return 0;
        return this.#playing(sound);
    }
    // 원본 물리 표의 모든 항목과 복제 항목을 순서대로 방문한다.
    #each(visitor) {
        // 항목 크기는 현재 이름 길이와 종료 NUL을 포함하며 빈 이름에서 끝난다.
        for (let entry = SOUND_LIST_BASE;;) { const name = this.#list.name(entry); if (!name) return; visitor(entry, name); entry += SOUND_HEADER_BYTES + name.length + 1; }
    }
    // 자연 종료한 버퍼를 포함해 실제 재생 중인 물리 항목 수를 다시 센다.
    recount() {
        this.#state.playing = 0;
        this.#each((entry) => { const buffer = this.#get(entry, SOUND_FIELD.buffer); if (buffer && buffer !== SILENT_SOUND_BUFFER && (this.#hooks.status(buffer) & PLAYING)) this.#state.playing += 1; });
        return this.#state.playing;
    }
    // 조회한 원본 항목만 정지하며 복제 사슬이 있으면 원본 진단을 남긴다.
    stopByName(name) {
        const sound = this.#list.lookup(name);
        if (this.#get(sound, SOUND_FIELD.root) !== sound || this.#get(sound, SOUND_FIELD.next)) this.#assert('sound->isUnique()', 0x3f8);
        this.stop(sound);
    }
    // 이름을 먼저 등록한 뒤 원본 항목의 재생 여부를 조회한다.
    isPlayingByName(name) { return this.isPlaying(this.#list.lookup(name)); }
    // 활성 요청이면 미재생 이름을 시작하고 비활성이면 같은 이름만 정지한다.
    setNamePlaying(on, name, loop, volume) {
        if (!this.#state.initialized || name === null || !this.#state.enabled) return 0;
        if (!on) { this.stopByName(name); return 0; }
        const sound = this.#list.lookup(name);
        if (!this.#get(sound, SOUND_FIELD.buffer) || !this.#playing(sound)) this.playByName(name, loop, volume, 0, 0, 0);
        return on | 0;
    }
    // 원본처럼 재생·반복 상태를 각각 다시 조회해 반복 버퍼만 정지한다.
    stopLoops() {
        this.#each((entry) => { const buffer = this.#get(entry, SOUND_FIELD.buffer); if (buffer && buffer !== SILENT_SOUND_BUFFER && (this.#hooks.status(buffer) & PLAYING) && (this.#hooks.status(buffer) & LOOPING)) this.#stopBuffer(entry); });
    }
    // 보류 깊이가 있으면 예약만 하고, 아니면 모든 실제 버퍼의 전체 음량을 갱신한다.
    setMasterVolume(volume) {
        volume |= 0;
        if (this.#state.volumeHoldDepth) { this.#state.pendingMasterVolume = volume; return; }
        this.#state.masterVolume = volume; if (!this.#state.initialized) return;
        this.#each((entry) => { const buffer = this.#get(entry, SOUND_FIELD.buffer); if (buffer && buffer !== SILENT_SOUND_BUFFER) this.#hooks.setVolume(buffer, (this.#get(entry, SOUND_FIELD.volume) - this.#get(entry, SOUND_FIELD.attenuation) + volume) | 0); });
    }
    // 원본 `SoundMusic::PushMute` 의 효과음 쪽이다. 보류 깊이가 0 이면 현재 전체 음량을 예약해 두고 최소 음량(-10000)으로 낮춘 뒤 깊이를 올린다. 깊이가 있는 동안 음량 변경은 예약만 한다. 일시정지 메뉴가 쓴다. (2026-11-20 WB04-E1089)
    pushMute() {
        const state = this.#state;
        if (!state.volumeHoldDepth) { state.pendingMasterVolume = state.masterVolume; this.setMasterVolume(-10000); }
        state.volumeHoldDepth = (state.volumeHoldDepth + 1) | 0;
    }
    // 원본 `PopMute` 의 효과음 쪽이다. 양수 깊이만 줄이고 0 이 되면 예약한 전체 음량을 복원한다. 깊이가 0 이하이면 아무 일도 없다. (2026-11-20 WB04-E1089)
    popMute() {
        const state = this.#state;
        if (state.volumeHoldDepth <= 0) return;
        state.volumeHoldDepth = (state.volumeHoldDepth - 1) | 0;
        if (!state.volumeHoldDepth) this.setMasterVolume(state.pendingMasterVolume);
    }
    // 현재 음소거 보류 깊이다(시험·정리용). (2026-11-20 WB04-E1089)
    muteDepth() { return this.#state.volumeHoldDepth; }
    // 파일이 미적재일 때만 이름에 해당하는 실제 장치 버퍼와 감쇠를 등록한다.
    preload(name) {
        const entry = this.#list.lookup(name);
        if (!this.#get(entry, SOUND_FIELD.buffer)) { const loaded = this.#hooks.load(name); this.#set(entry, SOUND_FIELD.buffer, loaded.buffer); if (loaded.buffer) this.#set(entry, SOUND_FIELD.attenuation, loaded.attenuation); }
        return entry;
    }
    // 실제 항목을 선택·복제하고 속성 설정 뒤 재생한다. 반환은 요청이 아닌 실제 항목일 수 있다.
    play(sound, loop, volume, pan, priority, limit) {
        if (!this.#state.initialized || !sound || !this.#state.enabled) return 0;
        if (!priority && this.#state.maxPlaying && this.#state.playing >= this.#state.maxPlaying) return 0;
        const failed = (line) => this.#log(9, `Failed to duplicate sound buffer [.\\Sound.cpp line ${line}]\n`);
        if (!this.#get(sound, SOUND_FIELD.buffer)) {
            const root = this.#get(sound, SOUND_FIELD.root);
            if (!this.#get(root, SOUND_FIELD.buffer)) {
                const loaded = this.#hooks.load(this.#list.name(root)); this.#set(root, SOUND_FIELD.buffer, loaded.buffer);
                if (loaded.buffer) this.#set(root, SOUND_FIELD.attenuation, loaded.attenuation);
            }
            if (!this.#get(sound, SOUND_FIELD.buffer)) {
                const copy = this.#hooks.duplicate(this.#get(root, SOUND_FIELD.buffer));
                if (copy.result < 0) { failed(0x310); this.#set(sound, SOUND_FIELD.buffer, 0); return sound; }
                this.#set(sound, SOUND_FIELD.buffer, copy.buffer); this.#log(0x1d, `Restored sound ${this.#list.name(sound)}\n`);
            }
        }
        if (this.#get(sound, SOUND_FIELD.buffer) === SILENT_SOUND_BUFFER) return sound;
        let entry = sound, buffer;
        if (!this.#playing(sound)) buffer = this.#get(sound, SOUND_FIELD.buffer);
        else {
            let walk = this.#get(sound, SOUND_FIELD.root), last = 0, count = 0;
            // 같은 원본의 재생 중 복제 항목을 차례로 세고 첫 빈 항목을 찾는다.
            while (walk && this.#get(walk, SOUND_FIELD.buffer) && this.#playing(walk)) { last = walk; walk = this.#get(walk, SOUND_FIELD.next); count += 1; }
            if (limit && count >= limit) return 0;
            if (walk) {
                buffer = this.#get(walk, SOUND_FIELD.buffer);
                if (!buffer) { const copy = this.#hooks.duplicate(this.#get(sound, SOUND_FIELD.buffer)); if (copy.result < 0) { failed(0x33c); this.#set(walk, SOUND_FIELD.buffer, 0); return walk; } this.#set(walk, SOUND_FIELD.buffer, copy.buffer); buffer = copy.buffer; }
                entry = walk;
            } else {
                const copy = this.#hooks.duplicate(this.#get(sound, SOUND_FIELD.buffer));
                if (copy.result < 0) { failed(0x348); return sound; }
                if (!last) throw new Error('소리 항목의 원본 연결이 비어 있습니다');
                entry = this.#list.appendDuplicate(sound, last, copy.buffer); buffer = copy.buffer;
            }
        }
        this.#set(entry, SOUND_FIELD.priority, priority); this.#set(entry, SOUND_FIELD.serial, this.#state.serial); this.#state.serial = (this.#state.serial + 1) >>> 0;
        this.#set(entry, SOUND_FIELD.volume, volume);
        if (this.#hooks.setVolume(buffer, this.#gain(entry, volume)) < 0) this.#log(0x1d, 'failed to set buffer volume\n');
        this.#hooks.setPan(buffer, pan); this.#hooks.play(buffer, loop ? 1 : 0); this.#state.playing = (this.#state.playing + 1) | 0;
        return entry;
    }
    // 위치에 따라 현재 반복 소리를 유지·중지하고 좌우와 음량을 다시 설정한다.
    playLoopAt(x, y, current, sound) {
        if (!this.#state.enabled || !this.#state.device) return 0;
        if (current && sound) { if (foldName(this.#list.name(current)) === foldName(this.#list.name(sound))) sound = 0; else { this.#stopBuffer(current); current = 0; } }
        const point = soundToScreen(this.currentView(), x, y), visible = soundOnScreen(this.currentView(), point);
        if (visible && !(current && this.#get(current, SOUND_FIELD.buffer) && this.#playing(current))) current = this.play(sound || current, 1, 0, 0, 0, 1);
        if (!current) return 0;
        if (!visible) { if (!this.#get(current, SOUND_FIELD.buffer)) return current; if (this.#playing(current)) { this.#stopBuffer(current); return 0; } }
        if (!this.#get(current, SOUND_FIELD.buffer) || !this.#playing(current)) return current;
        const pan = soundPan(this.currentView(), point, this.#state.swapSpeakers), volume = soundVolume(this.currentView(), point), buffer = this.#get(current, SOUND_FIELD.buffer);
        this.#set(current, SOUND_FIELD.volume, volume);
        if (this.#hooks.setVolume(buffer, this.#gain(current, volume)) !== 0) this.#log(9, 'Failed to set sound buffer volume [file .\\Sound.cpp, line 976]\n');
        if (this.#hooks.setPan(buffer, pan) !== 0) this.#log(9, 'Failed to set sound buffer pan [file .\\Sound.cpp, line 981]\n');
        return current;
    }
    // 화면 밖 소리는 재생하지 않고, 화면 안 소리는 현재 좌우와 음량으로 재생한다.
    playOnceAt(x, y, sound, loop, priority) {
        const point = soundToScreen(this.currentView(), x, y);
        if (!soundOnScreen(this.currentView(), point)) return 0;
        const pan = soundPan(this.currentView(), point, this.#state.swapSpeakers), volume = soundVolume(this.currentView(), point);
        return this.play(sound, loop, volume, pan, priority, 0);
    }
    // 반복 이름의 유일성을 먼저 진단하고 같은 이름 표의 항목을 재생한다.
    playByName(name, loop, volume, pan, priority, limit) {
        if (loop) { const entry = this.#list.lookup(name); if (this.#get(entry, SOUND_FIELD.root) !== entry || this.#get(entry, SOUND_FIELD.next)) this.#assert('qq', 0x3ef); }
        return this.play(this.#list.lookup(name), loop, volume, pan, priority, limit);
    }
    // 화면 밖이어도 이름을 먼저 등록한 뒤 위치 재생을 선택한다.
    playNameAt(x, y, name, loop) { return this.playOnceAt(x, y, this.#list.lookup(name), loop, 0); }
}
