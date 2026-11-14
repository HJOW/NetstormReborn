// 원본 Image(0046d790/0046d900/0046d8a0)의 bitmap 소비·VFX 1.10 변환·깊은 복사·소멸 수명을 옮긴다(cpppj client/RawImage.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 동적 이미지는 자산 TYPE 그림과 별도 수명으로 소유한다. 행 우선 8비트 bitmap(0 은 투명)을 받아 불투명 사각형만 단일 프레임 VFX 1.10 shape(40바이트 머리 + 행별 skip/repeat/literal 토큰)로 인코딩하고 bitmap 은 소비한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 인코딩은 웹 ShapeDatabase 로 되풀이 해독해 원래 픽셀과 대조한다. 기준선 뒤 변경도 없다.
// 사용: const image = new RawImage(); image.convert({ width, height, pixels }, true); image.shape() → Uint8Array, image.width()/height().
// (2026-11-14 WB04-E440: RawMenuControls 선행 동적 Image 이식)

// 1.10 shape 머리의 표식('1.10')과 단일 프레임 표/프레임 머리 크기다.
const SHAPE_MAGIC = 0x30312e31; const HEADER_BYTES = 40; const FRAME_TABLE_OFFSET = 16;
// 빈 그림의 원문 sentinel(INT_MAX) 값이다.
const INT_MAX = 0x7fffffff;

// 원문 16비트 좌표 필드를 낮은 바이트부터 쓴다.
function writeWord(output, offset, value) { output[offset] = value & 0xff; output[offset + 1] = (value >> 8) & 0xff; }
// 원문 DWORD 의 감김도 같은 비트로(little endian, 비정렬 머리 포함) 쓴다.
function writeDword(output, offset, value) { for (let index = 0; index < 4; index += 1) output[offset + index] = (value >>> (index * 8)) & 0xff; }

// 0040463b/004047ab: 행 시작 pair 와 literal 내부 triple 을 구분하고 끝의 투명 run 은 버린다. 토큰: 1(skip,개수 ≤255)·개수×2(repeat,색 하나, ≤127)·개수×2+1(literal, 색 개수개, ≤127)·행 끝 0.
function encodeRow(output, row) {
    let cursor = 0; let pending = 0;
    // 한 행의 모든 source 픽셀을 원문 선택 순서로 소비한다.
    while (cursor < row.length) {
        if (!row[cursor]) { cursor += 1; pending += 1; continue; }
        // 다음 불투명 run 이 있을 때만 255 단위 skip 토큰을 쓴다.
        while (pending) { const count = Math.min(pending, 255); output.push(1, count); pending -= count; }
        let end = cursor + 1;
        // 시작 위치의 같은 색을 먼저 확인한다. 첫 pair 는 길이 2 부터 repeat 로 선택한다.
        while (end < row.length && row[end] === row[cursor]) end += 1;
        if (end - cursor >= 2) {
            // repeat 는 127픽셀마다 같은 색을 다시 쓴다.
            while (cursor < end) { const count = Math.min(end - cursor, 127); output.push(count * 2, row[cursor]); cursor += count; }
            continue;
        }
        // 단일 색은 literal 의 시작이며 중간 pair 는 literal 에 남긴다.
        const start = cursor; cursor += 1;
        // 세 번 같은 색이 나오는 위치 직전까지 literal 을 모은다.
        while (cursor < row.length && row[cursor]) {
            if (cursor + 2 < row.length && row[cursor] === row[cursor + 1] && row[cursor] === row[cursor + 2]) break;
            cursor += 1;
        }
        let first = start;
        // literal 도 127단위로 나누되 payload 의 255/0 의미를 바꾸지 않는다.
        while (first < cursor) { const count = Math.min(cursor - first, 127); output.push(count * 2 + 1); for (let index = 0; index < count; index += 1) output.push(row[first + index]); first += count; }
    }
    // 행 끝의 pending skip 은 flush 하지 않고 terminator 만 쓴다.
    output.push(0);
}

// 004dad20→004042cd: 실제 nonzero 사각형만 인코딩하되 상자/기준점은 crop 크기를 보존한다(머리 24바이트의 좌표는 DWORD 로 감긴다).
function encodeImage(bits, stride, width, height) {
    let left = INT_MAX; let top = INT_MAX; let right = -INT_MAX; let bottom = -INT_MAX;
    // crop 영역의 실제 불투명 경계를 먼저 찾는다(투명 0 과 불투명 255 를 구별).
    for (let y = 0; y < height; y += 1) {
        for (let x = 0; x < width; x += 1) {
            if (bits[y * stride + x]) { left = Math.min(left, x); right = Math.max(right, x); top = Math.min(top, y); bottom = Math.max(bottom, y); }
        }
    }
    // 1.10 의 단일 프레임 표 16바이트와 실제 프레임 헤더 24바이트다.
    const result = new Array(HEADER_BYTES).fill(0);
    writeDword(result, 0, SHAPE_MAGIC); writeDword(result, 4, 1); writeDword(result, 8, FRAME_TABLE_OFFSET);
    writeWord(result, 16, height - 1); writeWord(result, 18, width - 1); writeWord(result, 20, height - 1); writeWord(result, 22, width - 1);
    writeDword(result, 24, (left - (width - 1)) >>> 0); writeDword(result, 28, (top - (height - 1)) >>> 0); writeDword(result, 32, (right - (width - 1)) >>> 0); writeDword(result, 36, (bottom - (height - 1)) >>> 0);
    // 빈 sentinel 은 반복을 시작하지 않으며 40바이트 shape 를 그대로 유지한다.
    for (let y = top; y <= bottom; y += 1) { const first = y * stride + left; encodeRow(result, bits.subarray(first, first + (right - left + 1))); }
    return Uint8Array.from(result);
}

// 동적 이미지 하나다. bitmap(+0)과 shape(+4)을 독립 버퍼로 소유하고 폭/높이(+8/+c)와 shape 바이트 수(+10)를 가진다.
export class RawImage {
    // 원본 +0 bitmap 과 +4 shape 의 독립 소유 버퍼다. 성공한 convert 뒤 bitmap 은 비어 있다.
    #bits = new Uint8Array(0); #shape = new Uint8Array(0);
    // 원본 +8/+c 의 crop 전/후 크기와 +10 shape 바이트 수다(+10 은 destroy 가 쓰지 않는 DWORD 다).
    #width = 0; #height = 0; #shapeSize = 0;

    // 0046d790: 입력을 소비하고 오른쪽/아래 여백만 줄여(crop) 실제 VFX shape 를 만든다. bitmap = { width, height, pixels }(행 우선). 이미 bitmap 이 있으면 Error(원문 imageBits == NULL 진단),
    // 크기가 맞지 않으면 TypeError 다. crop 은 오른쪽/아래 마지막 nonzero 만 쓰며 전부 0 이면 원문과 같은 최소 1×1 이다. 왼쪽/위쪽 여백은 자르지 않는다.
    convert(bitmap, crop) {
        if (this.#bits.length) throw new Error('Image imageBits == NULL 원문 진단');
        if (!(bitmap.width > 0) || !(bitmap.height > 0) || bitmap.width * bitmap.height !== bitmap.pixels.length) throw new TypeError('Image bitmap 크기 오류');
        const stride = bitmap.width; const pixels = Uint8Array.from(bitmap.pixels);
        let width = bitmap.width; let height = bitmap.height;
        if (crop) {
            let lastX = 0; let lastY = 0;
            // 각 행의 마지막 nonzero 가 crop 경계를 갱신한다.
            for (let y = 0; y < height; y += 1) {
                for (let x = 0; x < width; x += 1) { if (pixels[y * stride + x]) { lastX = Math.max(lastX, x); lastY = y; } }
            }
            width = lastX + 1; height = lastY + 1;
        }
        this.#width = width; this.#height = height;
        this.#shape = encodeImage(pixels, stride, width, height); this.#shapeSize = this.#shape.length;
        // 0046d790 마지막 free(imageBits)와 NULL 저장이다.
        this.#bits = new Uint8Array(0);
    }

    // 0046d900: 먼저 현재 버퍼를 파괴한 뒤 source 를 깊게 복사한다. 자기 복사도 이 순서라 빈 이미지가 된다.
    copyFrom(source) {
        // 자기 복사도 먼저 파괴하므로 아래 source 읽기는 이미 비워진 값이다(원문 순서).
        this.destroy(); this.#width = source.width(); this.#height = source.height();
        const bits = source.bits(); if (bits.length) this.#bits = Uint8Array.from(bits);
        const shape = source.shape(); if (shape.length) { this.#shapeSize = source.shapeSize(); if (this.#shapeSize) this.#shape = Uint8Array.from(shape); }
    }

    // 0046d8a0: 두 버퍼/폭/높이를 지우며 shapeSize 는 원문대로 보존한다. 중복 호출이 가능하다.
    destroy() { this.#bits = new Uint8Array(0); this.#shape = new Uint8Array(0); this.#height = 0; this.#width = 0; }

    // 변환 뒤 crop 0/1 이 확정한 실제 폭/높이다(원문 +8/+c).
    width() { return this.#width; }
    height() { return this.#height; }
    // 마지막 shape 바이트 수이며 소멸 뒤에도 남는다(원문 +10).
    shapeSize() { return this.#shapeSize; }
    // 원본 1.10 블록/프레임/RLE 전체를 빌린다(전용 MetaImage 가 읽는다).
    shape() { return this.#shape; }
    // 변환 성공 뒤에는 소비되어 빈 bitmap 버퍼다.
    bits() { return this.#bits; }
}
