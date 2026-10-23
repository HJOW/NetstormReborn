// 원본 소리 장치(DirectSound)의 효과음·음악 재생을 Web Audio 로 옮긴 모듈이다.
// 음량: 원본 옵션 단계(1~5)를 1/100 dB 감쇠로 바꾸고(1=-40dB, 2=-20dB, 3=-10dB, 4=-5dB, 그 밖=0dB) 그 값을 증폭률로 쓴다.
//       기본값은 효과음 3(-10dB), 음악 2(-20dB)이다(원본 Interpret Options 의 soundVolume/musicVolume 기본).
// 음악: 원본 음악 채널은 반복 플래그가 켜진 곡을 파일 끝에서 되감아 이어 재생하므로 AudioBufferSourceNode 의 loop 로 옮긴다.
//       메인 메뉴 곡은 ser22.mus(RIFF WAV PCM)이다.
// 원본 효과음 버퍼는 DSBCAPS_GLOBALFOCUS 없이 만들어 창이 포커스를 잃으면 들리지 않는다. 웹에서는 탭이 숨겨질 때 suspend 로 대응한다.
// 복원 기준: cpppj ClientAudio.cpp(VolumeFromStep)·SoundMusic.cpp·SoundSceneMusic.cpp. (2026-10-23 WB03-M1)

// 원본 기본 효과음 음량 단계이다(설정 soundVolume 기본 3). (2026-10-23 WB03-M1)
export const DEFAULT_SOUND_VOLUME_STEP = 3;

// 원본 기본 음악 음량 단계이다(설정 musicVolume 기본 2). (2026-10-23 WB03-M1)
export const DEFAULT_MUSIC_VOLUME_STEP = 2;

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
        if (!buffer || this.closed) return false;
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

    // 음악을 반복 재생한다. 같은 곡이 이미 재생 중이면 다시 시작하지 않는다(원본 Request 의 같은 이름 무시). 시작했으면 true.
    playMusic(name) {
        const key = String(name).toLowerCase();
        if (this.closed) return false;
        if (this.currentMusic === key && this.musicSource) return true;
        const buffer = this.music.get(key);
        if (!buffer) return false;
        this.stopMusic();
        const source = this.context.createBufferSource();
        source.buffer = buffer;
        source.loop = true;
        source.connect(this.musicGain);
        source.start();
        this.musicSource = source;
        this.currentMusic = key;
        return true;
    }

    // 재생 중인 음악을 멈춘다.
    stopMusic() {
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
