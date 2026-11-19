// SoundPlayer의 버퍼·복제·재생·음량·좌우 장치를 현재 Web Audio 문맥으로 제공한다.
// 사용: new WebSoundDevice(audio, assets).hooks() → SoundPlayer; 소리 파일은 재생 전에 assets.loadSound(name)으로 준비한다.
import { SILENT_SOUND_BUFFER } from './sound-player.js';

// DirectSound 1/100dB 감쇠를 선형 진폭으로 변환한다. 최저값은 음소거다.
function gainFor(value) { return value <= -10000 ? 0 : 10 ** (value / 2000); }

// 하나의 Web Audio 문맥과 디코딩된 파일을 빌리는 버퍼 소유자다.
export class WebSoundDevice {
    // 실제 재생기·파일 선택기, 논리 버퍼와 다음 주소, 반납 상태다.
    #audio; #assets; #buffers = new Map(); #next = 1; #disposed = false;
    // 재생 가능한 문맥과 원본 파일 선택 경계를 확인한다.
    constructor(audio, assets) {
        if (!audio?.context || !(audio.sounds instanceof Map) || typeof assets?.resolveSound !== 'function') throw new TypeError('웹 소리 장치의 실제 오디오·파일 선택기 누락');
        this.#audio = audio; this.#assets = assets;
    }
    // 반납된 장치나 없는 논리 버퍼는 현재 효과를 실행하지 않는다.
    #buffer(handle) {
        if (this.#disposed) throw new Error('반납한 웹 소리 장치');
        const buffer = this.#buffers.get(handle >>> 0); if (!buffer) throw new Error('웹 소리 장치의 버퍼 수명 오류'); return buffer;
    }
    // 같은 해독된 음원을 새 재생 수명의 독립 버퍼에 넣는다.
    #create(decoded) {
        if (this.#disposed || this.#audio.closed) throw new Error('반납한 웹 소리 장치');
        if (this.#next >= SILENT_SOUND_BUFFER) throw new RangeError('웹 소리 버퍼 주소 소진');
        const handle = this.#next++; this.#buffers.set(handle, { decoded, source: null, nodes: null, loop: false, volume: 0, pan: 0 }); return handle;
    }
    // 현재 노드에 전체 감쇠와 좌우 채널 감쇠를 적용한다.
    #apply(buffer) {
        if (!buffer.nodes) return;
        buffer.nodes.gain.gain.value = gainFor(buffer.volume);
        buffer.nodes.left.gain.value = gainFor(buffer.pan > 0 ? -buffer.pan : 0);
        buffer.nodes.right.gain.value = gainFor(buffer.pan < 0 ? buffer.pan : 0);
    }
    // 종료된 재생의 연결을 해제하되 이미 교체된 새 재생은 건드리지 않는다.
    #finish(buffer, source) {
        if (buffer.source !== source) return;
        source.onended = null; source.disconnect();
        // 재생 당시 만든 각 노드만 해제한다.
        for (const node of Object.values(buffer.nodes)) node.disconnect();
        buffer.source = null; buffer.nodes = null; buffer.loop = false;
    }
    // SoundPlayer가 요구하는 현재 장치 효과 여덟 개를 묶어 빌려준다.
    hooks() {
        return {
            status: (handle) => { const buffer = this.#buffer(handle); return buffer.source ? 1 | (buffer.loop ? 4 : 0) : 0; },
            load: (name) => {
                const selected = this.#assets.resolveSound(name); if (!selected) return { buffer: SILENT_SOUND_BUFFER, attenuation: 0 };
                const physical = selected.entry.path.split('/').at(-1).toLowerCase();
                const decoded = this.#audio.sounds.get(String(name).toLowerCase()) ?? this.#audio.sounds.get(physical);
                if (!decoded) throw new Error(`준비되지 않은 미션 효과음: ${name}`);
                return { buffer: this.#create(decoded), attenuation: selected.attenuation };
            },
            duplicate: (handle) => ({ result: 0, buffer: this.#create(this.#buffer(handle).decoded) }),
            setVolume: (handle, value) => { const buffer = this.#buffer(handle); value |= 0; if (value < -10000 || value > 0) return -1; buffer.volume = value; this.#apply(buffer); return 0; },
            setPan: (handle, value) => { const buffer = this.#buffer(handle); value |= 0; if (value < -10000 || value > 10000) return -1; buffer.pan = value; this.#apply(buffer); return 0; },
            setPosition: (handle, value) => { this.#buffer(handle); if (value !== 0) throw new RangeError('웹 소리 버퍼의 0이 아닌 위치 미지원'); },
            play: (handle, flags) => {
                const buffer = this.#buffer(handle); if (buffer.source || this.#audio.closed) throw new Error('웹 소리 버퍼의 재생 수명 오류');
                const context = this.#audio.context, source = context.createBufferSource(), gain = context.createGain();
                const splitter = context.createChannelSplitter(2), left = context.createGain(), right = context.createGain(), merger = context.createChannelMerger(2);
                gain.channelCount = 2; gain.channelCountMode = 'explicit'; source.buffer = buffer.decoded; source.loop = Boolean(flags & 1);
                source.connect(gain); gain.connect(splitter); splitter.connect(left, 0); splitter.connect(right, 1); left.connect(merger, 0, 0); right.connect(merger, 0, 1); merger.connect(context.destination);
                buffer.source = source; buffer.nodes = { gain, splitter, left, right, merger }; buffer.loop = source.loop; this.#apply(buffer);
                source.onended = () => this.#finish(buffer, source);
                try { source.start(); } catch (error) { this.#finish(buffer, source); throw error; }
            },
            stop: (handle) => { const buffer = this.#buffer(handle), source = buffer.source; if (!source) return; source.stop(); this.#finish(buffer, source); },
        };
    }
    // 활성 소리와 노드 연결을 모두 반납한다. 반복 호출은 안전하다.
    dispose() {
        if (this.#disposed) return;
        // 각 재생은 자신의 노드만 정지하고 해제한다.
        for (const buffer of this.#buffers.values()) { const source = buffer.source; if (source) { source.stop(); this.#finish(buffer, source); } }
        this.#buffers.clear(); this.#disposed = true;
    }
}
