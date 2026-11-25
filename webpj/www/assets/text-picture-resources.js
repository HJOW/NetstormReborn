// 안내·도움말·메뉴 그림에 한해 원본 10.78 자료에서 빠진 마나 아이콘을 보충하는 읽기 전용 UI 그림 경계이다(cpppj `client/TextPictureResources.h`, `HtmlTextParser.cpp` BindHtmlPictureText).
// 10.78 의 `_shapes.shp` 마나 블록은 8프레임뿐인데 공식 안내 HTML 은 `imana.8`~`imana.11`·`imana.17` 같은 번호를 쓴다. 두 가지를 맞춘다.
//  1. 별칭: 정확히 `~[imana.8]`·`9`·`10`·`11`·`17` 인 그림 명령만 `~[Imana.4]`·`5`·`6`·`7`·`9` 로 바꾼다(다른 타입·프레임 식은 그대로).
//  2. 보충 SHP: CD 원본 마나 25프레임(같은 팔레트·앞 8개 프레임 동일)을 UI 측정·출력에만 쓰고 월드 TYPE·SHP·게임 프레임 수는 바꾸지 않는다.
// 사용: repairManaPictureText('~[Imana.8]') → '~[Imana.4]', resolveTextPictureResource(shapes, block, clusterCount) → { shapes, block, frames }.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-25 WB04-E1336: cpppj 1335 판정 ②의 웹 반영)
import { ShapeDatabase } from './shapes.js';
import { TYPE_LOAD_ORDER } from './type-table.js';
import { MANA_PICTURE_BASE64 } from './mana-picture-data.js';

// 마나 타입의 SHP 블록 번호(원본 타입 로딩 순서)이다.
export const MANA_BLOCK = TYPE_LOAD_ORDER.indexOf('mana');

// 10.78 마나 블록의 프레임 수(클러스터 수)와 CD 보충본의 프레임 수이다.
export const MANA_ORIGINAL_FRAMES = 8;
export const MANA_SUPPLEMENT_FRAMES = 25;

// 공식 HTML 의 오래된 마나 그림 번호 → 현행 번호 별칭표이다(소문자 정확 일치).
const MANA_ALIASES = Object.freeze(new Map([['~[imana.8]', '~[Imana.4]'], ['~[imana.9]', '~[Imana.5]'], ['~[imana.10]', '~[Imana.6]'], ['~[imana.11]', '~[Imana.7]'], ['~[imana.17]', '~[Imana.9]']]));

// 그림 명령 문자열이 별칭 대상이면 현행 번호로 바꾸고 아니면 그대로 돌려준다. ASCII 대문자만 소문자로 비교한다(원본 AsciiLower).
export function repairManaPictureText(text) {
    const key = String(text).replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));
    return MANA_ALIASES.get(key) ?? text;
}

// 보충 ShapeDatabase 이다. 처음 쓸 때 한 번만 base64 를 풀어 만든다(앱 수명 동안 공유).
let supplement = null;
function supplementShapes() {
    if (supplement) return supplement;
    const text = atob(MANA_PICTURE_BASE64.replace(/\s+/g, ''));
    const bytes = new Uint8Array(text.length);
    // 각 문자를 바이트로 옮긴다.
    for (let index = 0; index < text.length; index += 1) bytes[index] = text.charCodeAt(index);
    supplement = new ShapeDatabase(bytes);
    return supplement;
}

// 같은 그림의 측정·제한 검사·출력에 공유할 저장소·블록·논리 프레임 수를 정한다.
// 10.78 마나(8클러스터·8프레임)만 보충본(블록 0, 25프레임)으로 바꾸고 다른 타입은 빌려 준 shapes 를 그대로 쓴다.
export function resolveTextPictureResource(shapes, block, clusterCount) {
    if (block === MANA_BLOCK && clusterCount === MANA_ORIGINAL_FRAMES && shapes.blocks[block]?.frames.length === MANA_ORIGINAL_FRAMES) {
        return { shapes: supplementShapes(), block: 0, frames: MANA_SUPPLEMENT_FRAMES };
    }
    return { shapes, block, frames: clusterCount };
}
