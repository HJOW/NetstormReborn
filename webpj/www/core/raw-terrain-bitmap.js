// 원본 지형 미리보기의 8비트 bitmap(004c1860)과 최초 문자 분류 캐시를 옮긴 모듈이다(cpppj `o/RawTerrainBitmap.cpp/.h`, 10.78 전용).
// 사용: const body = new RawTerrainBitmap(state, islandTypeBox, { green: () => 색번호, texture: (type) => ({ width, height, pixels(40000, 200 stride) }) });
//   body.render(codes(65,536BYTE, X 열 우선 문자), width, height, random(GameRandom)) → { width, height, pixels(행 우선 width × height) }.
// 순서: ① 최초 한 번만 분류 캐시 255BYTE 를 0 으로 지우고 'x' 칸만 현재 녹색 하위 BYTE 로 채운다(이후 팔레트/세션이 바뀌어도 같은 값 유지) → ② 현재 fringe TYPE(기본 102)의 texture 로
//   X 열마다 아래에서 위로 '=' 칸을 훑는다(위 칸이 'x' 이면 로컬 난수 `next(3) + 3` 길이만큼 fringe 를 아래로 칠하고, 위 칸이 '=' 가 아니면 120) → ③ 현재 지면 TYPE 의 texture 로 행 먼저
//   분류된 칸을 지면 질감으로 덮는다. texture 는 200 stride 40,000BYTE 이고 반복 주기는 각각 폭-1/높이-1 이다. 출력의 빈 0 은 texture 의 투명 255 와 별개로 보존한다.
// 이 모듈은 월드 객체나 공유 게임 난수를 건드리지 않으며(난수는 호출자가 준 `random` 하나) DOM 을 쓰지 않는다. 비정상 texture 와 문자 저장소 범위는 정상 이미지로 가장하지 않고 거부한다.
// (2026-11-11 WB04-E293: 4-B RawTerrainBitmap 이식)

// 지도 한 변 칸 수(문자 저장소의 X 열 간격)와 texture 의 가로 stride, 출력 폭/높이 상한이다. (2026-11-11 WB04-E293)
const WORLD_SIDE = 256;
const TEXTURE_STRIDE = 200;
const TEXTURE_MAX = 200;

// 지도에서 쓰는 문자: 본섬 'x', 경계 '='. (2026-11-11 WB04-E293)
const CODE_ISLAND = 0x78;
const CODE_FRINGE = 0x3d;

// fringe 의 아래쪽 끝 색(위가 'x' 도 '=' 도 아닌 '=' 칸의 원문 고정 색 번호)과 분류 캐시의 원문 BYTE 수, 문자 분류의 signed 상한이다. (2026-11-11 WB04-E293)
const COAST_COLOR = 120;
const CLASSIFIED_BYTES = 255;
const SIGNED_LIMIT = 128;

// 첫 요청에서 만들어 팔레트/세션 교체 뒤에도 같은 Client 수명으로 유지하는 분류 상태이다. initialized 는 005c8bb8(최초 BSS 0, 정상 초기화 1),
// classified 는 005c8ab0 의 원문 255BYTE(code 'x' 만 최초 녹색 하위 BYTE), fringeType 은 005411f0 의 현재 TYPE(기본 102)이며 지면 TYPE 5411dc 와 별개이다. (2026-11-11 WB04-E293)
export class TerrainBitmapState {
    initialized = 0;
    classified = new Uint8Array(CLASSIFIED_BYTES);
    fringeType = 102;
}

// 실제 texture 헤더(폭/높이 1 초과 200 이하, 40,000BYTE)를 검사한다. 비정상 texture 의 원문 나눗셈/잘못된 주소를 정상 이미지로 가장하지 않는다. (2026-11-11 WB04-E293)
function validateTexture(texture) {
    if (!texture || !Number.isInteger(texture.width) || !Number.isInteger(texture.height) || !(texture.pixels instanceof Uint8Array) || texture.pixels.length !== TEXTURE_STRIDE * TEXTURE_MAX
        || texture.width <= 1 || texture.height <= 1 || texture.width > TEXTURE_MAX || texture.height > TEXTURE_MAX) {
        throw new RangeError('지형 bitmap의 실제 texture 헤더 범위');
    }
}

// 원문 004c1860 의 수직 fringe → 지면 texture 순서와 로컬 난수 소비를 유지한다. state·island 상자는 이 본체보다 오래 살아야 한다.
// islandType 은 현재 지면 TYPE 의 `{ value }` 상자이다(호출마다 읽음). (2026-11-11 WB04-E293)
export class RawTerrainBitmap {
    // Client 최초 분류 상태, 현재 지면 TYPE 의 상자, 선택되는 실제 조회 장치이다.
    #state;
    #islandType;
    #hooks;

    // 생성자는 분류 캐시나 난수를 변경하지 않는다. 조회 장치는 render 때 필요한 쪽만 요구한다(누락은 거기서 명시 오류). (2026-11-11 WB04-E293)
    constructor(state, islandType, hooks) {
        if (!(state instanceof TerrainBitmapState) || typeof islandType?.value !== 'number' || hooks === null || typeof hooks !== 'object'
            || (hooks.green !== undefined && typeof hooks.green !== 'function') || (hooks.texture !== undefined && typeof hooks.texture !== 'function')) {
            throw new TypeError('지형 bitmap의 상태/지면 TYPE 상자/조회 장치 종류 오류');
        }
        this.#state = state;
        this.#islandType = islandType;
        this.#hooks = hooks;
    }

    // 입력은 물리 x × 256 + y 의 65,536BYTE 문자 지도이다. 출력은 width × height 행 우선 8비트 색 번호이고 투명 판정은 Draw 부모가 한다. (2026-11-11 WB04-E293)
    render(codes, width, height, random) {
        if (!(codes instanceof Uint8Array) || codes.length !== WORLD_SIDE * WORLD_SIDE || !random || typeof random.next !== 'function') {
            throw new TypeError('지형 bitmap의 문자 저장소/난수 종류 오류');
        }
        if (!Number.isInteger(width) || !Number.isInteger(height) || width < 0 || height < 0 || width > WORLD_SIDE || height > WORLD_SIDE) {
            throw new RangeError('지형 bitmap의 실제 문자 저장소 범위');
        }
        const state = this.#state;
        // 최초 한 번만 분류 캐시를 지우고 'x' 만 현재 녹색 하위 BYTE 로 채운다(정상 초기화 표식 1).
        if (!state.initialized) {
            state.classified.fill(0);
            if (!this.#hooks.green) throw new Error('지형 bitmap 현재 녹색 조회 미연결');
            state.classified[CODE_ISLAND] = this.#hooks.green() & 0xff;
            state.initialized = 1;
        }
        if (!this.#hooks.texture) throw new Error('지형 bitmap 실제 texture 조회 미연결');
        const fringe = this.#hooks.texture(state.fringeType);
        validateTexture(fringe);
        const pixels = new Uint8Array(width * height);
        // 원문은 각 X 열의 Y 를 아래에서 위로 진행한다.
        for (let x = 0; x < width; x += 1) {
            // 위 칸을 함께 읽으므로 실제 Y 0 은 첫 fringe 순회에서 제외한다.
            for (let y = height - 1; y > 0; y -= 1) {
                if (codes[x * WORLD_SIDE + y] !== CODE_FRINGE) continue;
                const before = codes[x * WORLD_SIDE + y - 1];
                if (before === CODE_ISLAND) {
                    const length = random.next(3) + 3;
                    // 첫 샘플은 texture 의 1 행이며 아래 끝은 출력 높이로 제한한다.
                    for (let step = 0; step < length && y + step < height; step += 1) {
                        const sample = ((step + 1) % (fringe.height - 1)) * TEXTURE_STRIDE + (x % (fringe.width - 1));
                        pixels[(y + step) * width + x] = fringe.pixels[sample];
                    }
                } else if (before !== CODE_FRINGE) {
                    pixels[y * width + x] = COAST_COLOR;
                }
            }
        }
        const ground = this.#hooks.texture(this.#islandType.value >>> 0);
        validateTexture(ground);
        // 원본 두 번째 순회는 출력 행을 먼저 진행한다.
        for (let y = 0; y < height; y += 1) {
            // 각 행의 X 를 처음부터 다시 읽어 지면 텍스처를 채운다.
            for (let x = 0; x < width; x += 1) {
                const code = codes[x * WORLD_SIDE + y];
                if (code >= SIGNED_LIMIT) throw new RangeError('지형 bitmap의 원문 signed 문자 분류 범위');
                if (state.classified[code]) pixels[y * width + x] = ground.pixels[(y % (ground.height - 1)) * TEXTURE_STRIDE + (x % (ground.width - 1))];
            }
        }
        return { width, height, pixels };
    }
}
