// 원본 Shift-F7 화면 저장(`GifScreenshot`)의 순수 계산을 옮긴 모듈이다(cpppj `client/GifScreenshot.cpp`). 파일 장치와 LZW 출력을 분리한다.
// 사용:
//   dispatchScreenshotKey(event, save, tell) → Shift-F7(Control 없음, 뗌 아님)이면 save() 와 tell('TellGifWritten') 를 실행하고 true 를 돌려준다. 정지 중에도 저장한다.
//   screenshotPrimaryPalette(primaryBytes) → 고정 primary 표(256×56바이트, 색마다 +24 에 R·G·B double)에서 256색 RGB 768바이트를 만든다(현재 flash·날씨 팔레트를 섞지 않는다).
//   encodeScreenshotGif(width, height, pitch, pixels, rgb) → 팔레트 인덱스를 그대로 GIF87a 한 장(전역 색표 256, LZW 최소 코드 8)으로 인코딩한 Uint8Array 이다.
//   screenshotFileName(counter) → 원문 GifOutName 형식 `pic<signed 번호>.gif` 이다.
// 웹은 디스크 대신 브라우저 내려받기를 쓰므로 파일 존재 확인·잠금은 옮기지 않고 번호 증가와 파일 이름만 남긴다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-20 WB04-E1100: GifScreenshot 이식)
import { InputCode } from './input-queue.js';

// 수정키를 지운 입력 코드 마스크와 F7 가상 키이다. (2026-11-20 WB04-E1100)
const KEY_MASK = 0x03ffffff;
const KEY_F7 = 0x760000;
// 선택된 외부 장치가 없으면 그 위치에서 거부한다. (2026-11-20 WB04-E1100)
function need(device) {
    if (typeof device !== 'function') throw new Error('스크린샷 실제 장치 미연결');
    return device;
}

// Shift-F7 화면 저장 키를 분배한다. 뗌이거나 Control 이 있거나 Shift 가 없으면 소비하지 않는다. (2026-11-20 WB04-E1100)
export function dispatchScreenshotKey(event, save, tell) {
    if ((event.code & KEY_MASK) !== KEY_F7 || (event.code & (InputCode.release | InputCode.control)) || !(event.code & InputCode.shift)) return false;
    need(save)(); need(tell)('TellGifWritten'); return true;
}

// 원문 `pic<N>.gif` 이름이다. 번호는 signed DWORD 로 쓴다. (2026-11-20 WB04-E1100)
export function screenshotFileName(counter) {
    return `pic${counter | 0}.gif`;
}

// 고정 primary 표에서 256색 RGB 를 읽는다. 값은 0..255 범위의 double 곱(×255)이며 범위 밖이면 거부한다. (2026-11-20 WB04-E1100)
export function screenshotPrimaryPalette(primary) {
    if (primary.length !== 256 * 56) throw new Error('스크린샷 primary0x3800BYTE 필요');
    const rgb = new Uint8Array(768), view = new DataView(primary.buffer, primary.byteOffset, primary.byteLength);
    // 원래 색 번호 순서의 256개 기록을 방문한다.
    for (let color = 0; color < 256; color += 1) {
        for (let channel = 0; channel < 3; channel += 1) {
            const scaled = view.getFloat64(color * 56 + 24 + channel * 8, true) * 255;
            if (!(scaled >= 0 && scaled <= 255)) throw new RangeError('스크린샷 primary RGB 범위');
            rgb[color * 3 + channel] = Math.trunc(scaled);
        }
    }
    return rgb;
}

// 팔레트 인덱스 화면을 GIF87a 한 장으로 인코딩한다. 외부 양자화 없이 색 번호와 행 방향을 그대로 쓰며 행 패딩(pitch - width)은 건너뛴다. (2026-11-20 WB04-E1100)
export function encodeScreenshotGif(width, height, pitch, pixels, rgb) {
    if (!(width > 0) || !(height > 0) || width > 65535 || height > 65535 || pitch < width || pitch * (height - 1) + width > pixels.length || rgb.length !== 768) throw new RangeError('스크린샷 GIF 폭/높이/행 간격 범위');
    const output = [0x47, 0x49, 0x46, 0x38, 0x37, 0x61]; // 'GIF87a' — 투명·애니메이션 확장이 없는 단일 이미지 형식이다.
    const word = (value) => { output.push(value & 0xff, (value >>> 8) & 0xff); };
    word(width); word(height); output.push(0xf7, 0, 0); for (const byte of rgb) output.push(byte);
    output.push(0x2c); word(0); word(0); word(width); word(height); output.push(0, 8);
    const compressed = [], dictionary = new Map();
    let bits = 0, bitCount = 0, codeWidth = 9, next = 258;
    // 현재 폭으로 코드를 LSB 부터 쓰고 디코더의 한 항목 늦은 폭 증가에 맞춘다.
    const emit = (code) => {
        bits = (bits | (code << bitCount)) >>> 0; bitCount += codeWidth;
        while (bitCount >= 8) { compressed.push(bits & 0xff); bits >>>= 8; bitCount -= 8; }
        if (next === (1 << codeWidth) && codeWidth < 12) codeWidth += 1;
    };
    emit(256);
    let prefix = pixels[0], first = true;
    // 행 패딩을 건너뛰고 논리 화면의 위에서 아래로 방문한다.
    for (let y = 0; y < height; y += 1) {
        for (let x = 0; x < width; x += 1) {
            if (first) { first = false; continue; }
            const byte = pixels[y * pitch + x], key = prefix * 256 + byte, found = dictionary.get(key);
            if (found !== undefined) { prefix = found; continue; }
            emit(prefix);
            if (next < 4096) { dictionary.set(key, next); next += 1; } else { emit(256); dictionary.clear(); next = 258; codeWidth = 9; }
            prefix = byte;
        }
    }
    emit(prefix); emit(257); if (bitCount) compressed.push(bits & 0xff);
    // GIF 최대 255바이트 서브 블록으로 나눈다.
    for (let offset = 0; offset < compressed.length; offset += 255) {
        const count = Math.min(255, compressed.length - offset); output.push(count);
        for (let index = 0; index < count; index += 1) output.push(compressed[offset + index]);
    }
    output.push(0, 0x3b);
    return Uint8Array.from(output);
}
