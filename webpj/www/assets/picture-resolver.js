// 본문 속 그림 명령 `~[I타입.프레임식]`(도움말의 `<!"타입.프레임식">` 포함)을 셰이프 그림으로 해석하는 모듈이다.
// 원본 StyleTextMarkup::Picture(10.78 004bb430 안, cpppj `StyleTextMarkup.cpp`)의 규칙을 그대로 옮겼다.
//  1. 타입: 첫 캡처를 CRT 정수로 읽어 0 이 아니면 그 번호, 0 이면 같은 문자열을 타입 이름 표에서 대소문자 무시로 찾는다. 못 찾으면(-1) 그림이 없다.
//  2. 프레임 식: 첫 글자가 `*`(`#`·`&` 는 65×60 상자 맞춤)이면 gumpFrame(-1 이면 defaultFrame), 첫 글자가 `A ≤ 글자 < A + letterKinds` 이면
//     그 글자 구간의 첫 클러스터 번호에 뒤의 숫자를 더하고, 그 밖에는 식 전체를 숫자로 읽는다(`mana.2`·`fortGump.L6`·`93.113`).
//  3. 프레임 번호는 0 미만이면 0, 프레임 수(클러스터 수) 이상이면 프레임 수로 바로잡으며 그 번호에는 그림이 없어 크기 0 조각이 된다.
//  4. 그려지는 프레임은 셰이프 블록(타입 로딩 순서)의 해당 논리 프레임(레이어 0)이고, 기준점은 셰이프 추가 헤더의 기준점에서 타입 기준점을 하위 16비트로 뺀 값이다.
// 그림의 폭·높이·기준점은 줄 높이와 기준선(올림 높이 = 기준점 y)을 정하는 데 쓰인다(ui/style-text.js).
// 지원하지 않는 부분: `#`·`&`(상자 맞춤·색 치환) 그림은 null 로 돌려 호출자가 알린다(현재 도움말에는 쓰이지 않는다).
// (2026-11-01 WB03-T: 도움말 그림(에너지 기호·간헐천·건물·주문 아이콘) 지원을 위해 추가, 이전에는 fortGump 숫자 프레임만 해석했다)

// 캡처 하나에서 읽는 최대 글자 수이다(원본 Regexp::Get 의 80바이트 복사). 프레임 식이 이 길이 이상이면 원본이 지역 배열을 넘쳐 쓰므로 그림을 만들지 않는다. (2026-11-01 WB03-T)
const CAPTURE_LIMIT = 80;

// 그림이 없는 프레임의 빈 그림이다(폭·높이 0, 픽셀 없음). 원본은 이 경우에도 크기 0 인 그림 조각을 줄에 넣는다. (2026-11-01 WB03-T)
const EMPTY_IMAGE = Object.freeze({ width: 0, height: 0, left: 0, top: 0, indices: new Uint8Array(0), opacity: new Uint8Array(0) });

// 그림이 없는 프레임의 빈 그림 조각 정보이다. (2026-11-01 WB03-T)
export const EMPTY_PICTURE = Object.freeze({ image: EMPTY_IMAGE, width: 0, height: 0, hotspotX: 0, hotspotY: 0 });

// CRT 정수 변환(앞 공백·부호·숫자, 숫자가 없으면 0)이다. 32비트 부호 있는 정수로 감긴다(원본 `ConfigParseLong`/atol). 사용: parseCrtInteger(' -12ab') → -12. (2026-11-01 WB03-T)
export function parseCrtInteger(text) {
    const matched = /^[ \t\n\v\f\r]*([+-]?)(\d*)/.exec(text);
    if (!matched || matched[2] === '') return 0;
    let value = 0;
    // 숫자를 앞에서부터 읽으며 32비트로 감는다.
    for (const digit of matched[2]) value = (Math.imul(value, 10) + (digit.charCodeAt(0) - 48)) | 0;
    return matched[1] === '-' ? -value | 0 : value;
}

// 하위 16비트만 빼고 부호 있는 16비트로 다시 읽는다(원본 타입 기준점 보정 SubtractWord). (2026-11-01 WB03-T)
function subtractWord(value, offset) {
    return ((value - offset) << 16) >> 16;
}

// 그림 해석 함수를 만든다. 반환 함수 (이름, 프레임식) → { image, width, height, hotspotX, hotspotY } 또는 null 이다.
// shapes 는 ShapeDatabase, getTable 은 타입 표(TypeTable)를 돌려주는 함수이다(없으면 null, 처음 쓸 때 만들도록 지연한다).
// null 은 "지원하지 않는 그림"(타입 이름을 못 찾음·표 밖 번호·상자 맞춤·셰이프 오류)이며 호출자가 알린다. 그림이 없는 프레임은 EMPTY_PICTURE 이다.
// 사용: const resolve = createPictureResolver({ shapes, getTable }); resolve('fortGump', 'L6'). (2026-11-01 WB03-T)
export function createPictureResolver({ shapes, getTable }) {
    return (name, expression) => {
        const table = getTable();
        if (!table) return null;
        const nameText = String(name).slice(0, CAPTURE_LIMIT);
        const expressionText = String(expression).slice(0, CAPTURE_LIMIT);
        if (expressionText.length === 0 || String(expression).length >= CAPTURE_LIMIT) return null;
        // 타입 번호: 숫자 문자열이면 그 번호, 아니면 이름으로 찾는다.
        let type = parseCrtInteger(nameText);
        if (type === 0) type = table.find(nameText);
        if (type === -1) return null;
        const view = table.view(type);
        if (view === null) return null;
        const first = expressionText[0];
        let frame = 0;
        if (first === '*' || first === '#' || first === '&') {
            // 상자 맞춤(`#`)·색 치환(`&`)은 지원하지 않는다.
            if (first !== '*') return null;
            frame = view.gumpFrame === -1 ? view.defaultFrame : view.gumpFrame;
        } else {
            let suffix = expressionText;
            const letter = first.charCodeAt(0);
            // 첫 글자가 signed char 로 'A' 이상이고 글자 구간 수 안이면 그 글자 구간의 첫 번호에서 시작한다.
            if (letter >= 0x41 && letter < 0x80 && letter < 0x41 + view.letterKinds) {
                frame = view.first[letter - 0x41];
                suffix = suffix.slice(1);
            }
            frame = (frame + parseCrtInteger(suffix)) | 0;
        }
        const count = view.frameCount;
        if (frame < 0 || count < 0) frame = 0;
        else if (frame >= count) frame = count;
        // 무자산 타입이거나 프레임 수 이상이면 그림이 없는 크기 0 조각이다.
        if (view.block < 0 || frame >= count) return EMPTY_PICTURE;
        try {
            const metrics = shapes.squidMetrics(view.block, frame);
            const hotspotX = subtractWord(metrics.hotspotX, view.hotspotX);
            const hotspotY = subtractWord(metrics.hotspotY, view.hotspotY);
            // 특수 레코드(이미지 없음)는 크기만 가지고 그리지 않는다.
            const special = shapes.frame(view.block, frame).special;
            const image = special ? EMPTY_IMAGE : shapes.decode(view.block, frame);
            return Object.freeze({ image, width: metrics.width, height: metrics.height, hotspotX, hotspotY });
        } catch (error) {
            // 셰이프 범위 밖이나 추가 헤더 없음은 지원하지 않는 그림으로 본다.
            return null;
        }
    };
}
