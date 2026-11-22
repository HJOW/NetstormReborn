// 원본 소리 장치(DirectSound)의 효과음·음악 재생을 Web Audio 로 옮긴 모듈이다.
// 음량: 원본 옵션 단계(1~5)를 1/100 dB 감쇠로 바꾸고(1=-40dB, 2=-20dB, 3=-10dB, 4=-5dB, 그 밖=0dB) 그 값을 증폭률로 쓴다.
//       설정 없는 최초 실행의 기본값은 효과음·음악 모두 5(0dB)이다. 명시한 사용자 단계는 그대로 적용한다. (2026-11-16 WB04-E616)
// 음악: 원본 음악 채널의 반복 플래그를 AudioBufferSourceNode 의 loop 로 옮긴다. 승리·패배 곡은 반복하지 않는다.
//       메인 메뉴 곡은 ser22.mus(RIFF WAV PCM)이다.
// 원본 효과음 버퍼는 DSBCAPS_GLOBALFOCUS 없이 만들어 창이 포커스를 잃으면 들리지 않는다. 웹에서는 탭이 숨겨질 때 suspend 로 대응한다.
// 복원 기준: cpppj ClientAudio.cpp(VolumeFromStep)·SoundMusic.cpp·SoundSceneMusic.cpp. (2026-10-23 WB03-M1)

// 설정 없는 원본 최초 실행의 효과음 음량 단계이다. (2026-11-16 WB04-E616)
export const DEFAULT_SOUND_VOLUME_STEP = 5;

// 설정 없는 원본 최초 실행의 음악 음량 단계이다. (2026-11-16 WB04-E616)
export const DEFAULT_MUSIC_VOLUME_STEP = 5;

// 원본 소리 장치의 출력 표본 빈도(Hz)이다. 원본 효과음·음악 파일이 22,050Hz 이므로 같은 빈도로 문맥을 만들어 재표본을 줄인다.
// 브라우저가 이 값을 거부하면 기본 빈도로 만든다. (2026-10-23 WB03-M1)
export const PREFERRED_SAMPLE_RATE = 22050;

// 원본 음량 단계(1~5)를 DirectSound 의 1/100 dB 감쇠 값으로 바꾼다(원본 VolumeFromStep).
// 0 이하·6 이상 등 표에 없는 값은 0(감쇠 없음)이다. (2026-10-23 WB03-M1)
export function volumeStepToCentibels(step) {
    switch ((step - 1) >>> 0) {
        case 0: return -4000;
        case 1: return -2000;
        case 2: return -1000;
        case 3: return -500;
        default: return 0;
    }
}

// 1/100 dB 감쇠 값을 Web Audio 증폭률(진폭 배율)로 바꾼다. -2000 → 0.1, 0 → 1. (2026-10-23 WB03-M1)
export function centibelsToGain(centibels) {
    return Math.pow(10, centibels / 2000);
}

// 원본 표본 빈도로 AudioContext 를 만든다. 사용자 입력 처리 안에서 불러야 브라우저 자동 재생 정책을 통과한다.
// view 는 루트 문서의 window 다. 지원하지 않으면 null 이다. (2026-10-23 WB03-M1)
export function createAudioContext(view) {
    const AudioContextClass = view.AudioContext ?? view.webkitAudioContext;
    if (!AudioContextClass) return null;
    try {
        return new AudioContextClass({ sampleRate: PREFERRED_SAMPLE_RATE });
    } catch (error) {
        return new AudioContextClass();
    }
}

// 효과음·음악 재생기이다. 이름으로 효과음 버퍼를 등록하고 재생하며, 음악 하나를 반복 재생한다.
// 사용: const audio = new AudioSystem(context); await audio.loadSound('button.wav', bytes); audio.playSound('button.wav');
//       await audio.loadMusic('ser22.mus', bytes); audio.playMusic('ser22.mus'); (2026-10-23 WB03-M1)
export class AudioSystem {
    // AudioContext 와 음량 단계로 재생기를 만든다.
    constructor(context, { soundVolumeStep = DEFAULT_SOUND_VOLUME_STEP, musicVolumeStep = DEFAULT_MUSIC_VOLUME_STEP } = {}) {
        // 소리를 내는 Web Audio 문맥이다.
        this.context = context;
        // 효과음 전체 음량 노드다(원본 SetMasterVolume).
        this.soundGain = context.createGain();
        this.soundGain.gain.value = centibelsToGain(volumeStepToCentibels(soundVolumeStep));
        this.soundGain.connect(context.destination);
        // 음악 음량 노드다(원본 music SetVolume).
        this.musicGain = context.createGain();
        this.musicGain.gain.value = centibelsToGain(volumeStepToCentibels(musicVolumeStep));
        this.musicGain.connect(context.destination);
        // 이름(소문자) → 해독한 효과음 AudioBuffer 표다.
        this.sounds = new Map();
        // 이름(소문자) → 해독한 음악 AudioBuffer 표다.
        this.music = new Map();
        // 지금 재생 중인 음악 이름(소문자)이다. 없으면 null 이다.
        this.currentMusic = null;
        // 지금 재생 중인 음악 소스 노드다.
        this.musicSource = null;
        // close 뒤인지 여부다. 닫힌 뒤에는 재생 요청을 무시한다.
        this.closed = false;
        // 효과음을 낼지 여부다(옵션 메뉴 Sound On). 꺼져 있으면 playSound 가 아무 소리도 내지 않는다. (2026-10-29 WB03-F)
        this.soundEnabled = true;
        // 음악을 낼지 여부다(옵션 메뉴 Play Music). 꺼져 있어도 재생을 요청한 곡은 wantedMusic 에 남아 다시 켜면 이어서 재생된다. (2026-10-29 WB03-F)
        this.musicEnabled = true;
        // 재생하기로 요청받은 음악 이름(소문자)이다. 음악이 꺼진 동안에도 기억한다. 없으면 null. (2026-10-29 WB03-F)
        this.wantedMusic = null;
        // 음소거 보류 깊이와 해제 때 복원할 음악 음량(증폭률)이다. 일시정지 메뉴의 PushMute/PopMute 가 쓴다. (2026-11-20 WB04-E1089)
        this.musicMuteDepth = 0; this.pendingMusicGain = 1;
    }

    // 효과음 음량 단계(1~5)를 바꾼다. 재생 중인 소리에도 바로 적용된다(원본 SetMasterVolume). (2026-10-29 WB03-F)
    setSoundVolumeStep(step) {
        this.soundGain.gain.value = centibelsToGain(volumeStepToCentibels(step));
    }

    // 음악 음량 단계(1~5)를 바꾼다. 재생 중인 음악에도 바로 적용된다. 음소거 중이면 해제할 때 적용하도록 예약만 한다(원본 SoundMusic::SetVolume 의 보류). (2026-10-29 WB03-F, 2026-11-20 WB04-E1089)
    setMusicVolumeStep(step) {
        const gain = centibelsToGain(volumeStepToCentibels(step));
        if (this.musicMuteDepth > 0) { this.pendingMusicGain = gain; return; }
        this.musicGain.gain.value = gain;
    }

    // 원본 `SoundMusic::PushMute` 의 음악 쪽이다. 깊이가 0 이면 현재 음량을 예약하고 음악을 무음(0)으로 낮춘 뒤 깊이를 올린다. 일시정지 메뉴가 쓰며 짝이 되는 popMute 가 복원한다. (2026-11-20 WB04-E1089)
    pushMute() {
        if (this.musicMuteDepth === 0) { this.pendingMusicGain = this.musicGain.gain.value; this.musicGain.gain.value = 0; }
        this.musicMuteDepth += 1;
    }

    // 원본 `PopMute` 의 음악 쪽이다. 양수 깊이만 줄이고 0 이 되면 예약한 음량을 복원한다. 깊이가 0 이하이면 아무 일도 없다. (2026-11-20 WB04-E1089)
    popMute() {
        if (this.musicMuteDepth <= 0) return;
        this.musicMuteDepth -= 1;
        if (this.musicMuteDepth === 0) this.musicGain.gain.value = this.pendingMusicGain;
    }

    // 소리(Sound On)를 켜거나 끈다. 끄면 이후 playSound 는 소리를 내지 않고(이미 재생 중인 짧은 소리는 끝까지 난다) 재생 중인 음악도 멈춘다.
    // 원본 ReinterpretClientAudioOptions(4355df..435680)는 소리 옵션이 꺼지면 장치를 닫아 음악까지 무음이 되고, 다시 켜면 장치를 열고 현재 곡(Select)을 처음부터 다시 재생한다.
    // 요청받은 곡 이름(wantedMusic)은 꺼진 동안에도 남는다. 같은 값을 다시 넣는 호출(옵션 갱신마다 부른다)은 재생 중인 음악을 건드리지 않는다.
    // (2026-10-29 WB03-F, 2026-11-23 WB04-E1239: 끄면 음악도 무음, 켜면 현재 곡을 처음부터)
    setSoundEnabled(enabled) {
        const next = Boolean(enabled), previous = this.soundEnabled;
        this.soundEnabled = next;
        if (!next) this._stopMusicSource();
        else if (!previous && this.musicEnabled && this.wantedMusic) this.playMusic(this.wantedMusic);
    }

    // 음악을 켜거나 끈다. 끄면 재생 중인 곡을 멈추고, 다시 켜면 마지막으로 요청받은 곡을 처음부터 반복 재생한다(원본도 음악 장치를 다시 시작한다). 소리가 꺼져 있으면 곡은 켜 둔 채 기억만 한다. (2026-10-29 WB03-F)
    setMusicEnabled(enabled) {
        this.musicEnabled = Boolean(enabled);
        if (!this.musicEnabled) this._stopMusicSource();
        else if (this.wantedMusic) this.playMusic(this.wantedMusic);
    }

    // 파일 바이트를 AudioBuffer 로 해독한다. decodeAudioData 는 입력 버퍼를 넘겨받으므로 복사본을 넘긴다.
    async decode(bytes) {
        const copy = bytes.slice().buffer;
        return await this.context.decodeAudioData(copy);
    }

    // 효과음을 해독해 이름으로 등록한다. 이름은 원본처럼 대소문자를 구분하지 않는다.
    async loadSound(name, bytes) {
        const buffer = await this.decode(bytes);
        this.sounds.set(String(name).toLowerCase(), buffer);
        return buffer;
    }

    // 등록한 효과음을 한 번 재생한다. 등록되지 않았거나 닫혔으면 아무것도 하지 않고 false 를 돌려준다.
    playSound(name) {
        const buffer = this.sounds.get(String(name).toLowerCase());
        if (!buffer || this.closed || !this.soundEnabled) return false;
        const source = this.context.createBufferSource();
        source.buffer = buffer;
        source.connect(this.soundGain);
        source.start();
        return true;
    }

    // 음악을 해독해 이름으로 등록한다.
    async loadMusic(name, bytes) {
        const buffer = await this.decode(bytes);
        this.music.set(String(name).toLowerCase(), buffer);
        return buffer;
    }

    // 음악을 재생한다. 승리·패배 곡은 한 번만 재생하며 같은 곡이 이미 재생 중이면 다시 시작하지 않는다. 시작했으면 true.
    // 음악이 꺼져 있거나 소리(Sound On)가 꺼져 있으면 곡 이름만 기억하고(다시 켜면 재생) false 를 돌려준다. (2026-11-23 WB04-E1239: 소리가 꺼진 동안도 기억만)
    playMusic(name) {
        const key = String(name).toLowerCase();
        if (this.closed) return false;
        this.wantedMusic = key;
        if (!this.musicEnabled || !this.soundEnabled) return false;
        if (this.currentMusic === key && this.musicSource) return true;
        const buffer = this.music.get(key);
        if (!buffer) return false;
        this._stopMusicSource();
        const source = this.context.createBufferSource();
        source.buffer = buffer;
        source.loop = key !== 'fanfare.mus' && key !== 'defeat.mus';
        source.connect(this.musicGain);
        // 이전 소스의 지연 종료가 새 곡의 현재 상태를 지우지 않도록 동일 소스만 정리한다.
        source.onended = () => {
            if (this.musicSource !== source) return;
            source.disconnect(); this.musicSource = null; this.currentMusic = null;
        };
        this.musicSource = source;
        this.currentMusic = key;
        source.start();
        return true;
    }

    // 재생 중인 음악을 멈추고 요청받은 곡도 잊는다(다시 켜도 재생하지 않음). (2026-10-29 WB03-F: 소스 정지는 _stopMusicSource 로 분리)
    stopMusic() {
        this._stopMusicSource();
        this.wantedMusic = null;
    }

    // 재생 중인 음악 소스만 멈춘다. 요청받은 곡(wantedMusic)은 그대로 둔다. (2026-10-29 WB03-F)
    _stopMusicSource() {
        if (this.musicSource) {
            try {
                this.musicSource.stop();
            } catch (error) {
                // 이미 멈춘 소스는 무시한다.
            }
            this.musicSource.disconnect();
        }
        this.musicSource = null;
        this.currentMusic = null;
    }

    // 소리 출력을 잠시 멈춘다(탭 숨김). 음악 위치는 유지된다.
    async suspend() {
        if (!this.closed && this.context.state === 'running') await this.context.suspend();
    }

    // 멈춘 소리 출력을 다시 켠다(탭 복귀). 사용자 입력으로 한 번 활성화된 문맥은 입력 없이도 재개할 수 있다.
    async resume() {
        if (!this.closed && this.context.state === 'suspended') await this.context.resume();
    }

    // 음악을 멈추고 문맥을 닫는다. 이후 재생 요청은 무시한다.
    close() {
        if (this.closed) return;
        this.stopMusic();
        this.closed = true;
        Promise.resolve(this.context.close?.()).catch(() => {});
    }
}
