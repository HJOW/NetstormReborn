// 원본 배경 GIF(타이틀·구름 등)를 8비트 색 번호 그대로 해독하는 모듈이다.
// 원본 GIF 경로(00419ec0 → 004dae60)는 파일의 색 표 RGB 를 쓰지 않고, 해독한 색 번호를 화면 팔레트에 그대로 복사한다.
// 그래서 브라우저의 이미지 디코더(RGB 로 바꿔 버림)를 쓰지 않고 LZW 를 직접 푼다.
// 복원 기준: cpppj GifImage.cpp(DecodeGif). (2026-10-23 WB03-M1: 메인 메뉴 타이틀·구름 표시를 위해 추가)

// 손상된 파일 때문에 지나치게 큰 버퍼를 만들지 않도록 정한 이미지 픽셀 수 상한이다. 원본 자료보다 충분히 크다. (2026-10-23 WB03-M1)
const MAX_GIF_PIXELS = 16 * 1024 * 1024;

// LZW 사전의 최대 항목 수(12비트 코드)이다. (2026-10-23 WB03-M1)
const LZW_TABLE_SIZE = 4096;

// GIF 인터레이스의 네 순회 시작 행이다. 각 패스는 INTERLACE_STEPS 간격으로 행을 채운다. (2026-10-23 WB03-M1)
const INTERLACE_STARTS = Object.freeze([0, 4, 2, 1]);

// GIF 인터레이스의 네 순회 행 간격이다. (2026-10-23 WB03-M1)
const INTERLACE_STEPS = Object.freeze([8, 8, 4, 2]);

// GIF 파일을 순서대로 읽는 바이트 읽기 도구이다. 파일 끝을 넘으면 오류를 던진다.
// 사용: decodeGif 내부에서만 쓴다. (2026-10-23 WB03-M1)
class GifReader {
    // 읽을 파일 바이트(Uint8Array)를 받는다.
    constructor(bytes) {
        // 읽고 있는 파일 바이트다.
        this.bytes = bytes;
        // 다음에 읽을 위치(바이트)다.
        this.position = 0;
    }

    // 한 바이트를 읽는다.
    byte() {
        if (this.position >= this.bytes.length) throw new Error('GIF 파일이 잘렸습니다.');
        return this.bytes[this.position++];
    }

    // 리틀 엔디언 16비트 정수를 읽는다.
    word() {
        const low = this.byte();
        return low | (this.byte() << 8);
    }

    // 쓰지 않는 바이트(색 표 등)를 건너뛴다. 파일 끝을 넘으면 오류다.
    skip(count) {
        if (count > this.bytes.length - this.position) throw new Error('GIF 색 표가 잘렸습니다.');
        this.position += count;
    }

    // 길이 바이트가 0인 블록이 나올 때까지 서브 블록을 이어 붙여 돌려준다.
    blocks() {
        const parts = [];
        let total = 0;
        // 길이 바이트와 데이터가 번갈아 나오는 GIF 서브 블록을 끝 표시(0)까지 읽는다.
        for (let size = this.byte(); size !== 0; size = this.byte()) {
            if (size > this.bytes.length - this.position) throw new Error('GIF 블록이 잘렸습니다.');
            parts.push(this.bytes.subarray(this.position, this.position + size));
            this.position += size;
            total += size;
        }
        const result = new Uint8Array(total);
        let offset = 0;
        // 모은 조각을 하나의 배열로 복사한다.
        for (const part of parts) {
            result.set(part, offset);
            offset += part.length;
        }
        return result;
    }
}

// GIF LZW 압축을 푼다(낮은 비트부터 읽기, 사전 초기화·코드 폭 증가·자기 참조 코드 처리).
// minimum 은 최소 코드 크기, pixels 는 기대하는 픽셀 수이며 종료 코드에서 개수가 다르면 오류다. (2026-10-23 WB03-M1)
function decodeLzw(bytes, minimum, pixels) {
    if (minimum < 2 || minimum > 8) throw new Error('GIF LZW 코드 크기가 잘못됐습니다.');
    const clear = 1 << minimum;
    const end = clear + 1;
    const prefix = new Uint16Array(LZW_TABLE_SIZE);
    const suffix = new Uint8Array(LZW_TABLE_SIZE);
    const stack = new Uint8Array(LZW_TABLE_SIZE);
    const output = new Uint8Array(pixels);
    let written = 0;
    let next = end + 1;
    let width = minimum + 1;
    let previous = LZW_TABLE_SIZE;
    let first = 0;
    let bit = 0;
    const totalBits = bytes.length * 8;
    // 종료 코드가 나올 때까지 가변 길이 코드를 하나씩 읽어 픽셀을 출력한다.
    while (bit + width <= totalBits) {
        let code = 0;
        // 코드 하나를 낮은 비트부터 모은다.
        for (let i = 0; i < width; i += 1, bit += 1) code |= ((bytes[bit >> 3] >> (bit & 7)) & 1) << i;
        if (code === clear) {
            next = end + 1;
            width = minimum + 1;
            previous = LZW_TABLE_SIZE;
            continue;
        }
        if (code === end) {
            if (written !== pixels) throw new Error('GIF 픽셀 수가 맞지 않습니다.');
            return output;
        }
        const input = code;
        let count = 0;
        if (code === next && previous !== LZW_TABLE_SIZE) {
            stack[count++] = first;
            code = previous;
        } else if (code >= next) {
            throw new Error('GIF LZW 코드가 잘못됐습니다.');
        }
        // 접두 사전을 거꾸로 따라가며 문자열을 스택에 쌓는다. 순환·범위 밖 참조는 거부한다.
        while (code >= clear) {
            if (code <= end || code >= next || count >= LZW_TABLE_SIZE - 1) throw new Error('GIF LZW 사전이 잘못됐습니다.');
            stack[count++] = suffix[code];
            code = prefix[code];
        }
        first = code;
        stack[count++] = code;
        if (count > pixels - written) throw new Error('GIF 출력이 이미지 크기를 넘었습니다.');
        // 거꾸로 쌓인 스택을 실제 픽셀 순서로 기록한다.
        while (count !== 0) output[written++] = stack[--count];
        if (previous !== LZW_TABLE_SIZE && next < LZW_TABLE_SIZE) {
            prefix[next] = previous;
            suffix[next] = first;
            next += 1;
            if (next === 1 << width && width < 12) width += 1;
        }
        previous = input;
    }
    throw new Error('GIF LZW 종료 코드가 없습니다.');
}

// GIF 파일의 첫 이미지를 해독해 { width, height, indices, opacity } 를 돌려준다.
// indices 는 논리 화면 크기의 색 번호(이미지가 덮지 않은 곳은 배경색 번호), opacity 는 0(투명)/255(불투명)이다.
// 그래픽 제어 확장의 투명 번호가 있으면 그 번호의 픽셀과 이미지 밖은 투명이다.
// 사용: decodeGif(bytes) 결과를 IndexedSurface.drawImage 나 tile 에 넘긴다. (2026-10-23 WB03-M1)
export function decodeGif(bytes) {
    const signature = String.fromCharCode(...bytes.subarray(0, 6));
    if (bytes.length < 13 || (signature !== 'GIF87a' && signature !== 'GIF89a')) throw new Error('GIF 서명이 아닙니다.');
    const reader = new GifReader(bytes);
    reader.skip(6);
    const width = reader.word();
    const height = reader.word();
    const packed = reader.byte();
    const background = reader.byte();
    reader.byte();
    if (width === 0 || height === 0 || width * height > MAX_GIF_PIXELS) throw new Error('GIF 크기가 잘못됐습니다.');
    if (packed & 0x80) reader.skip(3 << ((packed & 7) + 1));
    let transparent = -1;
    // 첫 이미지가 나올 때까지 확장 블록을 읽고, 그래픽 제어 확장의 투명 번호를 기억한다.
    for (;;) {
        const marker = reader.byte();
        if (marker === 0x21) {
            const kind = reader.byte();
            const data = reader.blocks();
            if (kind === 0xf9 && data.length === 4 && (data[0] & 1)) transparent = data[3];
            continue;
        }
        if (marker !== 0x2c) throw new Error('GIF 이미지가 없습니다.');
        const left = reader.word();
        const top = reader.word();
        const imageWidth = reader.word();
        const imageHeight = reader.word();
        const flags = reader.byte();
        if (imageWidth === 0 || imageHeight === 0 || left + imageWidth > width || top + imageHeight > height) {
            throw new Error('GIF 이미지 영역이 잘못됐습니다.');
        }
        if (flags & 0x80) reader.skip(3 << ((flags & 7) + 1));
        const minimum = reader.byte();
        const decoded = decodeLzw(reader.blocks(), minimum, imageWidth * imageHeight);
        const indices = new Uint8Array(width * height).fill(background);
        const opacity = new Uint8Array(width * height).fill(transparent < 0 ? 255 : 0);
        let source = 0;
        // 해독한 한 행을 논리 화면의 해당 행에 옮긴다.
        const copyRow = (y) => {
            // 이미지 사각형 안의 픽셀만 채운다.
            for (let x = 0; x < imageWidth; x += 1) {
                const index = (top + y) * width + left + x;
                const color = decoded[source++];
                indices[index] = color;
                opacity[index] = color === transparent ? 0 : 255;
            }
        };
        if (flags & 0x40) {
            // 인터레이스 GIF 는 네 패스로 나눠 저장된 행을 실제 행 위치에 배치한다.
            for (let pass = 0; pass < INTERLACE_STARTS.length; pass += 1) {
                // 현재 패스가 맡은 행들을 위에서 아래로 채운다.
                for (let y = INTERLACE_STARTS[pass]; y < imageHeight; y += INTERLACE_STEPS[pass]) copyRow(y);
            }
        } else {
            // 일반 GIF 는 위에서 아래로 행이 이어진다.
            for (let y = 0; y < imageHeight; y += 1) copyRow(y);
        }
        return Object.freeze({ width, height, indices, opacity });
    }
}
