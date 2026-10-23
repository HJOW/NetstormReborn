// 원본 Miles VFX "1.10" 셰이프 데이터베이스(_shapes.shp, 글꼴 글리프)를 읽고 프레임을 색 번호 그림으로 푸는 모듈이다.
// 행 단위 RLE(0=행 끝, 1=투명 건너뛰기, 홀수=직접 복사, 짝수=반복 복사)를 풀며 투명 여부를 색 번호와 따로 보존한다.
// _shapes.shp 의 프레임 앞 36바이트에는 Squid 추가 헤더(셀 크기·출력 크기·기준점)가 있다.
// 복원 기준: cpppj VFXDraw.cpp(ShapeDatabase), 형식 문서 docs/formats/shp.md.
// (2026-10-23 WB03-M1: 돌 버튼 질감(fortGump A00)과 원본 글꼴 글리프를 그리기 위해 추가)

// VFX 1.10 셰이프 블록의 리틀 엔디언 매직("1.10")이다. (2026-10-23 WB03-M1)
const SHAPE_MAGIC = 0x30312e31;

// 프레임 머리(bounds·origin·사각형)의 크기(바이트)이다. 압축 픽셀은 이 뒤부터 시작한다. (2026-10-23 WB03-M1)
const FRAME_HEADER_SIZE = 24;

// 그림 대신 메타데이터가 들어 있는 특수 레코드를 가르는 좌표 시작 값이다. (2026-10-23 WB03-M1)
const SPECIAL_COORDINATE = 0x7fff0000;

// _shapes.shp 프레임 앞에 붙는 Squid 추가 헤더의 크기(바이트)이다. 순수 VFX(글꼴)에는 없다. (2026-10-23 WB03-M1)
const SQUID_PREFIX_SIZE = 36;

// 손상된 파일로 거대한 버퍼를 만들지 않도록 정한 해독 픽셀 수 상한이다. (2026-10-23 WB03-M1)
const MAX_DECODED_PIXELS = 64 * 1024 * 1024;

// 프레임 하나의 위치·크기 정보이다. rect 는 기준점 기준의 압축 픽셀 영역(양 끝 포함)이다.
// 사용: ShapeDatabase.frame(block, frame) 이 돌려주며 decode·squidMetrics 의 입력이 된다. (2026-10-23 WB03-M1)
class ShapeFrame {
    // 파일 위치와 머리 값으로 프레임 정보를 만든다.
    constructor(offset, colorMapOffset, bounds, origin, rect) {
        // 프레임 머리의 파일 위치(바이트)다.
        this.offset = offset;
        // 블록 기준 색 변환표 위치다. 기본 VFX 그리기는 쓰지 않는다.
        this.colorMapOffset = colorMapOffset;
        // 원본 머리 순서의 그림 상자 높이·폭이다.
        this.bounds = bounds;
        // 원본 머리 순서의 기준점 y·x 다.
        this.origin = origin;
        // 기준점 기준 압축 픽셀 영역 { left, top, right, bottom } (양 끝 포함)이다.
        this.rect = rect;
        // 다음 고유 프레임의 시작 위치(이 프레임 압축 데이터의 끝 경계)다. 파싱 뒤 채운다.
        this.end = 0;
    }

    // 좌표 대신 메타데이터가 들어 있는 특수 레코드이거나 영역이 뒤집혀 그림이 없는지 검사한다.
    get special() {
        return this.rect.left >= SPECIAL_COORDINATE || this.rect.right < this.rect.left || this.rect.bottom < this.rect.top;
    }
}

// 셰이프 데이터베이스(블록 목록 + 프레임 목록)이다. 파일 앞부분의 연속 블록 머리를 읽고 프레임 경계를 검증한다.
// 사용: const db = new ShapeDatabase(bytes); db.decode(block, frame) 으로 그림을 얻는다.
// _shapes.shp 의 블록 번호는 원본 타입 로딩 순서이며, 프레임 번호는 '레이어 × 클러스터 수 + 클러스터 순번'이다.
// (2026-10-23 WB03-M1)
export class ShapeDatabase {
    // 파일 바이트(Uint8Array)를 읽어 블록·프레임 표를 만든다. 형식이 맞지 않으면 오류를 던진다.
    constructor(bytes) {
        // 셰이프 파일 바이트다.
        this.bytes = bytes;
        // 정수 읽기용 DataView 다.
        this.view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        // 블록 목록이다. 각 블록은 { offset, frames: ShapeFrame[] } 이다.
        this.blocks = [];
        // 해독한 프레임 그림 캐시다. 키는 '블록:프레임' 이다.
        this.cache = new Map();
        let position = 0;
        const frameOffsets = [];
        // 매직이 이어지는 동안 블록 머리를 차례로 읽어 블록 순서(타입 순서)를 보존한다.
        while (position + 8 <= bytes.length && this.view.getUint32(position, true) === SHAPE_MAGIC) {
            const count = this.view.getUint32(position + 4, true);
            if (count > (bytes.length - position - 8) / 8) throw new Error('셰이프 프레임 수가 잘못됐습니다.');
            const block = { offset: position, frames: [] };
            // 프레임 표의 각 항목(블록 기준 오프셋·색 변환표 오프셋)을 읽는다. 같은 오프셋이 여러 번 나와도 각각 유지한다.
            for (let i = 0; i < count; i += 1) {
                const relative = this.view.getUint32(position + 8 + i * 8, true);
                if (relative > bytes.length - position) throw new Error('셰이프 프레임 오프셋이 잘못됐습니다.');
                const offset = position + relative;
                if (offset + FRAME_HEADER_SIZE > bytes.length) throw new Error('셰이프 파일이 잘렸습니다.');
                const colorMapOffset = this.view.getUint32(position + 12 + i * 8, true);
                const bounds = [this.view.getUint16(offset, true), this.view.getUint16(offset + 2, true)];
                const origin = [this.view.getUint16(offset + 4, true), this.view.getUint16(offset + 6, true)];
                const rect = {
                    left: this.view.getInt32(offset + 8, true),
                    top: this.view.getInt32(offset + 12, true),
                    right: this.view.getInt32(offset + 16, true),
                    bottom: this.view.getInt32(offset + 20, true),
                };
                block.frames.push(new ShapeFrame(offset, colorMapOffset, bounds, origin, rect));
                frameOffsets.push(offset);
            }
            position += 8 + count * 8;
            this.blocks.push(block);
        }
        if (this.blocks.length === 0) throw new Error('VFX 1.10 셰이프 머리가 없습니다.');
        // 블록 머리 영역의 끝 위치다. Squid 추가 헤더가 블록 표를 읽지 않도록 막는 경계다.
        this.headerEnd = position;
        const unique = [...new Set(frameOffsets)].sort((a, b) => a - b);
        // 모든 프레임에 다음 고유 레코드까지의 끝 경계를 정하고, 블록 머리와 겹치는 프레임은 거부한다.
        for (const block of this.blocks) {
            // 블록 안의 각 프레임 경계를 정한다.
            for (const frame of block.frames) {
                if (frame.offset < position) throw new Error('셰이프 프레임이 블록 머리와 겹칩니다.');
                const nextIndex = upperBound(unique, frame.offset);
                frame.end = nextIndex < unique.length ? unique[nextIndex] : bytes.length;
                if (frame.end - frame.offset < FRAME_HEADER_SIZE) throw new Error('셰이프 프레임이 겹칩니다.');
            }
        }
    }

    // 블록·프레임 번호로 프레임 정보를 돌려준다. 범위 밖이면 RangeError 다.
    frame(blockIndex, frameIndex) {
        const block = this.blocks[blockIndex];
        const frame = block?.frames[frameIndex];
        if (!frame) throw new RangeError(`셰이프 프레임 범위 밖입니다: ${blockIndex}:${frameIndex}`);
        return frame;
    }

    // _shapes.shp 프레임 앞 36바이트 Squid 추가 헤더의 치수를 읽는다.
    // width/height 는 출력 크기(signed short), hotspotX/Y 는 기준점, cell* 은 float 셀 값이다.
    // 사용: 돌 버튼 질감의 반복 주기(width-1, height-1)를 구할 때 쓴다. 추가 헤더가 없으면 오류다.
    squidMetrics(blockIndex, frameIndex) {
        const offset = this.frame(blockIndex, frameIndex).offset;
        if (offset < this.headerEnd || offset - this.headerEnd < SQUID_PREFIX_SIZE) throw new Error('Squid SHP 추가 헤더가 없습니다.');
        const base = offset - SQUID_PREFIX_SIZE;
        return Object.freeze({
            cellWidth: this.view.getFloat32(base, true),
            cellHeight: this.view.getFloat32(base + 4, true),
            cellHotspotX: this.view.getFloat32(base + 8, true),
            cellHotspotY: this.view.getFloat32(base + 12, true),
            walkStepX: this.view.getFloat32(base + 16, true),
            walkStepY: this.view.getFloat32(base + 20, true),
            width: this.view.getInt16(offset - 12, true),
            height: this.view.getInt16(offset - 10, true),
            hotspotX: this.view.getInt16(offset - 8, true),
            hotspotY: this.view.getInt16(offset - 6, true),
        });
    }

    // 프레임의 RLE 를 풀어 { width, height, left, top, indices, opacity } 를 돌려준다(캐시됨).
    // left/top 은 기준점 기준 그림 왼쪽 위 좌표(rect.left/top)이며, 화면의 기준점 위치에 더해 그린다.
    // opacity 는 실제로 기록된 픽셀만 255 이고 건너뛴 픽셀은 0 이다. 특수 레코드는 오류다.
    decode(blockIndex, frameIndex) {
        const key = `${blockIndex}:${frameIndex}`;
        const cached = this.cache.get(key);
        if (cached) return cached;
        const frame = this.frame(blockIndex, frameIndex);
        if (frame.special) throw new Error('특수 셰이프 레코드에는 그림이 없습니다.');
        const width = frame.rect.right - frame.rect.left + 1;
        const height = frame.rect.bottom - frame.rect.top + 1;
        if (width > MAX_DECODED_PIXELS || height > MAX_DECODED_PIXELS || width * height > MAX_DECODED_PIXELS) {
            throw new Error('셰이프 그림이 해독 픽셀 상한을 넘었습니다.');
        }
        const indices = new Uint8Array(width * height);
        const opacity = new Uint8Array(width * height);
        const bytes = this.bytes;
        let position = frame.offset + FRAME_HEADER_SIZE;
        // 머리가 선언한 모든 행을 행 끝 토큰까지 해석한다.
        for (let row = 0; row < height; row += 1) {
            let column = 0;
            // 픽셀 폭을 다 채워도 행 끝 0 은 반드시 소비한다.
            for (;;) {
                if (position >= frame.end) throw new Error('셰이프 행 끝 표시가 없습니다.');
                const token = bytes[position++];
                if (token === 0) break;
                if (position >= frame.end) throw new Error('셰이프 run 이 잘렸습니다.');
                const count = token === 1 ? bytes[position++] : token >> 1;
                if (count > width - column) throw new Error('셰이프 run 이 행 폭을 넘었습니다.');
                if (token !== 1) {
                    const literal = (token & 1) !== 0;
                    const inputSize = literal ? count : 1;
                    if (inputSize > frame.end - position) throw new Error('셰이프 run 픽셀이 잘렸습니다.');
                    // 색 번호 0 을 포함해 실제 기록된 픽셀만 불투명으로 표시한다.
                    for (let i = 0; i < count; i += 1) {
                        const output = row * width + column + i;
                        indices[output] = bytes[position + (literal ? i : 0)];
                        opacity[output] = 255;
                    }
                    position += inputSize;
                }
                column += count;
            }
        }
        const image = Object.freeze({ width, height, left: frame.rect.left, top: frame.rect.top, indices, opacity });
        this.cache.set(key, image);
        return image;
    }
}

// 정렬된 배열에서 value 보다 큰 첫 원소의 위치를 이진 탐색으로 찾는다(C++ upper_bound). (2026-10-23 WB03-M1)
function upperBound(sorted, value) {
    let low = 0;
    let high = sorted.length;
    // 탐색 구간을 절반씩 줄인다.
    while (low < high) {
        const middle = (low + high) >> 1;
        if (sorted[middle] <= value) low = middle + 1;
        else high = middle;
    }
    return low;
}
