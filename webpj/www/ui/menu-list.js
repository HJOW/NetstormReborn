// 원본 세로 메뉴 목록(MenuGump, HTML 대화상자의 `$Menu=` 목록)을 복원하는 모듈이다.
// 안쪽으로 눌린 질감 상자 안에 항목(스타일 글자) 행이 세로로 쌓이고, 행 사이에 구분선이 들어갈 수 있다.
// 포인터가 행 위에 있으면 그 행이 어둡게 강조되고, 행을 누르는 즉시 명령이 실행된다(돌 버튼과 달리 뗄 때까지 기다리지 않음).
// 원본 주소: 생성 MenuGumpWidgets::Construct(전역 배경 질감 프레임 1, 글자 문맥), 항목 추가 MenuGumpControls::AddText, 배치 MenuGumpLayout::Reflow,
//            그리기 MenuGumpDraw::Draw, 입력 MenuGumpInput::Broadcast/Handle. 복원 기준: cpppj MenuGump*.cpp(10.78 경로).
// 하위 메뉴(자식)·가로 메뉴·장식 제목은 이 단위에서 옮기지 않는다. (2026-10-28 WB03-D: Tell 대화상자 메뉴 목록을 그리기 위해 추가)
import { createTextContext } from '../render/glyph-text.js';
import { GumpBackgroundFlags, drawGumpBackground, planGumpBackground } from '../render/gump-visual.js';
import { clipRect } from '../render/indexed-surface.js';
import { drawChain, measureChain, parseStyleText } from './style-text.js';

// 세로 메뉴의 항목 가로 여백(왼쪽·오른쪽)이다(원본 margins[0..1] = 15). (2026-10-28 WB03-D)
const VERTICAL_MARGIN_X = 15;

// 항목 세로 여백(위·아래)이다(원본 margins[2..3] = 1). (2026-10-28 WB03-D)
const MARGIN_Y = 1;

// 메뉴 항목 최대 수이다(원본 40칸). 넘는 항목은 추가하지 않는다. (2026-10-28 WB03-D)
export const MENU_ITEM_CAPACITY = 40;

// HTML 메뉴의 배경 플래그이다: 질감 + 네 테두리(0x03e00000) + 눌림(0x40000, 테두리 명암 반전 → 안쪽으로 눌린 상자). (2026-10-28 WB03-D)
const MENU_BACKGROUND_FLAGS = GumpBackgroundFlags.BUTTON | GumpBackgroundFlags.PRESSED;

// 선택 행 강조에 쓰는 배경 플래그이다: 질감 + 어둡게(0x2100000). (2026-10-28 WB03-D)
const HIGHLIGHT_FLAGS = GumpBackgroundFlags.TEXTURE | GumpBackgroundFlags.DARK;

// 구분선 두 줄의 배경 플래그이다: 첫 줄은 어둡게, 둘째 줄(1픽셀 아래)은 밝게 그려 홈이 파인 선처럼 보인다. (2026-10-28 WB03-D)
const SEPARATOR_FIRST_FLAGS = GumpBackgroundFlags.TEXTURE | GumpBackgroundFlags.DARK;
const SEPARATOR_SECOND_FLAGS = GumpBackgroundFlags.TEXTURE | GumpBackgroundFlags.BRIGHT;

// 체크 표시 그림을 항목 왼쪽 위에서 옮기는 값(x +5, y +7)이다(원본 004766e0 부근, 체크 SHP 프레임 J02). (2026-10-28 WB03-D)
const CHECK_MARKER_OFFSET = Object.freeze({ x: 5, y: 7 });

// 원본 CRT _ftol 처럼 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-28 WB03-D)
function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// 세로 메뉴 목록 하나이다. 항목을 addText/addSeparator/addChecked 로 추가하면 reflow 로 크기가 정해진다.
// 사용: const list = new MenuList({ fonts, colors }); list.addText('Credits', command, true); list.reflow(); list.x = ...; list.draw(surface, resources).
// 위치(x, y)는 화면 좌표이며 호출자가 정한다. 크기는 baseWidth/baseHeight(배율 전), width/height(배율 후)다. (2026-10-28 WB03-D)
export class MenuList {
    // fonts 는 FontSet, colors 는 StyleColors 이다. 글자 문맥은 원본 메뉴 생성자처럼 흰 글자 + 그림자(어두운 갈색, 오른쪽 1)로 시작한다.
    constructor({ fonts, colors }) {
        // 글꼴 묶음이다.
        this.fonts = fonts;
        // 색 번호 표다.
        this.colors = colors;
        // 메뉴 기본 글자 문맥이다(원본 +0x78: flags 2, 슬롯 0, 흰색, 그림자 어두운 갈색 (1,0)). 항목 글자는 슬롯 0 문맥으로 파싱한다.
        this.textContext = createTextContext({ flags: 2, font: 0, color: colors.white, style: 0, extra: 0, shadowColor: colors.dim, shadowX: 1, shadowY: 0 });
        // 항목 배열이다. 각 항목은 { kind('text'|'separator'|'checked'), fragments, command, disabled, checked, selected } 이다.
        this.items = [];
        // 항목별 높이(배율 전)이다.
        this.heights = [];
        // 항목별 위쪽 위치(메뉴 안 세로 위치)이다.
        this.offsets = [];
        // 배율 적용 전 폭(원본 +0x3c)이다.
        this.baseWidth = 0;
        // 배율 적용 전 높이(원본 +0x40)다.
        this.baseHeight = 0;
        // 배율이다.
        this.scale = 1;
        // 배율 적용 폭(float32)이다.
        this.width = 0;
        // 배율 적용 높이(float32)다.
        this.height = 0;
        // 화면 x 위치(float32)다.
        this.x = 0;
        // 화면 y 위치(float32)다.
        this.y = 0;
        // 현재 선택(포인터가 올라 있는) 항목 번호이다. 없으면 -1.
        this.selection = -1;
    }

    // 메뉴 항목 하나를 스타일 문자열로 만든다. 활성 항목은 원본처럼 `~[C.0]~[B.0]~E` 접두(색 설정은 꺼져 있어 소비만 하고 그림자만 켬)를 붙이고,
    // 비활성 항목은 `~*~x`(그림자 끄고 어두운 색) 접두에 본문을 그대로 쓴다. 글꼴 슬롯은 0 으로 시작한다.
    _styled(label, disabled) {
        const source = disabled ? `~*~x${label}` : `~[C.0]~[B.0]~E${label}`;
        const context = { ...this.textContext, font: 0 };
        return parseStyleText(source, context, this.colors);
    }

    // 글자 항목을 추가한다(원본 AddText). command 는 실행할 명령(없으면 null), enabled 가 false 면 비활성(어두운 글자, 선택 불가)이다.
    // 항목은 MENU_ITEM_CAPACITY(40)개까지다. 추가 후 크기를 다시 계산한다.
    addText(label, command, enabled = true) {
        if (this.items.length >= MENU_ITEM_CAPACITY) return;
        this.items.push({ kind: 'text', label, fragments: this._styled(label, !enabled), command, disabled: !enabled, checked: false, selected: false });
        this.reflow();
    }

    // 체크 항목을 추가한다(원본 AddChecked). checked 가 참이면 항목 왼쪽에 체크 표시를 그린다. 비활성이면 선택·실행할 수 없다.
    addChecked(label, command, checked, enabled = true) {
        if (this.items.length >= MENU_ITEM_CAPACITY) return;
        this.items.push({ kind: 'checked', label, fragments: this._styled(label, !enabled), command, disabled: !enabled, checked: Boolean(checked), selected: false });
        this.reflow();
    }

    // 구분선을 추가한다(원본 AddSeparator). 맨 앞이나 구분선 바로 뒤에는 추가하지 않는다. 구분선 행은 빈 글자 한 줄 높이를 가진다.
    addSeparator() {
        if (this.items.length >= MENU_ITEM_CAPACITY) return;
        if (this.items.length < 1 || this.items[this.items.length - 1].kind === 'separator') return;
        this.items.push({ kind: 'separator', label: '', fragments: [], command: null, disabled: false, checked: false, selected: false });
        this.reflow();
    }

    // 항목 크기로 메뉴 전체 크기와 행 위치를 정한다(원본 MenuGumpLayout::Reflow, 세로 메뉴).
    // 항목 폭 = 글자 폭 + 좌우 여백 30, 항목 높이 = 글자 높이 + 위아래 여백 2 이고, 메뉴 폭은 가장 넓은 항목, 높이는 모든 항목의 합이다.
    // 구분선·빈 항목은 슬롯 0 글꼴의 한 줄 높이(그림자 오프셋 포함)로 잰다.
    reflow() {
        let width = 0;
        let total = 0;
        this.heights = [];
        // 각 항목의 크기를 재고 최대 폭과 높이 합을 구한다.
        for (const item of this.items) {
            const measured = item.fragments.length > 0 ? measureChain(this.fonts, item.fragments) : this.fonts.measure(new Uint8Array(0), this.textContext);
            const itemWidth = ftol(measured.width + VERTICAL_MARGIN_X * 2);
            const itemHeight = ftol(measured.height + MARGIN_Y * 2);
            width = Math.max(width, itemWidth);
            total = (total + itemHeight) | 0;
            this.heights.push(Math.fround(itemHeight));
        }
        this.baseWidth = Math.fround(width);
        this.width = Math.fround(width * this.scale);
        this.baseHeight = Math.fround(total);
        this.height = Math.fround(total * this.scale);
        this.offsets = [];
        let offset = 0;
        // 각 행의 위쪽 위치를 앞 행들의 높이 합으로 정한다.
        for (let index = 0; index < this.items.length; index += 1) {
            this.offsets.push(Math.fround(offset));
            offset = ftol(offset + this.heights[index]);
        }
    }

    // 화면 정수 경계(원본 GumpCollection::Bounds)다.
    bounds() {
        const left = ftol(this.x);
        const top = ftol(this.y);
        return { left, top, right: ftol(left + this.width), bottom: ftol(top + this.height) };
    }

    // 화면 정수 좌표를 메뉴 안 위치(float32)로 바꾼다(원본 ScreenToLocal).
    _local(screenX, screenY) {
        return { x: Math.fround(screenX - this.x), y: Math.fround(screenY - this.y) };
    }

    // 지역 좌표가 메뉴 안인지 검사한다(원본 MenuGumpInput::Contains: [0, 폭) × [0, 높이)).
    _contains(local) {
        return local.x >= 0 && local.x < this.width && local.y >= 0 && local.y < this.height;
    }

    // 포인터 위치에서 선택될 항목 번호를 구한다. 메뉴 밖이면 -1, 안이면 행 범위에 맞는 첫 행이며 찾지 못하면 마지막 행이다(원본 Broadcast).
    // 비활성 항목은 선택되지 않는다(-1).
    hitIndex(screenX, screenY) {
        const local = this._local(screenX, screenY);
        if (!this._contains(local) || this.items.length === 0) return -1;
        let selected = 0;
        // 마지막 항목 전까지 행 범위를 훑어 포인터가 속한 행을 찾는다. 못 찾으면 마지막 행이 남는다.
        while (selected < this.items.length - 1) {
            if (local.y >= this.offsets[selected] && local.y < this.heights[selected] + this.offsets[selected]) break;
            selected += 1;
        }
        return this.items[selected].disabled ? -1 : selected;
    }

    // 포인터 이동에 맞춰 선택(강조) 행을 바꾼다(원본 Broadcast). 구분선은 선택될 수 있어도 강조하지 않는다. 바뀌면 true.
    pointerMove(screenX, screenY) {
        const selected = this.hitIndex(screenX, screenY);
        if (selected === this.selection) return false;
        if (this.selection >= 0) this.items[this.selection].selected = false;
        this.selection = selected;
        if (selected >= 0) this.items[selected].selected = true;
        return true;
    }

    // 포인터가 메뉴를 떠났을 때 강조를 지운다. 바뀌면 true.
    pointerLeave() {
        if (this.selection < 0) return false;
        this.items[this.selection].selected = false;
        this.selection = -1;
        return true;
    }

    // 왼쪽 버튼을 누른다(원본 MenuGumpInput::Handle). 선택 행을 갱신하고, 명령이 있는 행이면 그 명령을 돌려준다. 누른 곳이 행이 아니면 null 이다.
    // 돌려준 명령은 호출자가 즉시 실행한다(원본은 명령 큐에 넣고 효과음 openSubGump.wav 를 낸다).
    pointerDown(screenX, screenY) {
        this.pointerMove(screenX, screenY);
        if (this.selection < 0) return null;
        const item = this.items[this.selection];
        return item.kind !== 'separator' && item.command ? item.command : null;
    }

    // 메뉴를 그린다(원본 MenuGumpDraw::Draw): 경계로 클립 → 눌린 질감 배경(명암 테두리) → 행마다 [강조 | 구분선 | 체크 표시 + 글자].
    // resources: { texture(프레임 1), textureMetrics, shadeMaps, marker(체크 그림) }.
    draw(surface, resources) {
        const bounds = this.bounds();
        const clip = clipRect(bounds, surface.bounds);
        const background = (rect, flags, passClip) => {
            const passes = planGumpBackground(rect, passClip, resources.textureMetrics.width, resources.textureMetrics.height, flags);
            drawGumpBackground(surface, resources.texture, passes, resources.shadeMaps);
        };
        background(bounds, MENU_BACKGROUND_FLAGS, clip);
        const point = { x: bounds.left, y: bounds.top };
        // 각 항목 행을 위에서 아래로 그린다.
        for (let index = 0; index < this.items.length; index += 1) {
            const item = this.items[index];
            if (item.selected && item.kind !== 'separator') this._drawHighlight(surface, resources, index, point, clip);
            if (item.kind === 'separator') {
                this._drawSeparator(surface, resources, index, point, clip);
                continue;
            }
            const textY = ftol(point.y + this.offsets[index] + MARGIN_Y);
            if (item.kind === 'checked' && item.checked && resources.marker) {
                const markerX = ftol(VERTICAL_MARGIN_X * 0.25);
                const markerY = ftol(this.offsets[index] + MARGIN_Y);
                const image = resources.marker;
                surface.drawImage(image, (point.x + markerX + CHECK_MARKER_OFFSET.x + image.left) | 0, (point.y + markerY + CHECK_MARKER_OFFSET.y + image.top) | 0, clip);
            }
            drawChain(surface, this.fonts, item.fragments, ftol(point.x + VERTICAL_MARGIN_X), textY, clip);
        }
    }

    // 선택 행 강조를 그린다: 행 사각형(안쪽 2픽셀)에 어두운 질감을 덮는다(원본 MenuGumpDraw::Highlight).
    _drawHighlight(surface, resources, index, point, clip) {
        const rect = {
            left: ftol(point.x + 2),
            top: ftol(point.y + this.offsets[index] + 2),
            right: ftol(point.x + (this.width - 2)),
            bottom: ftol(point.y + (this.heights[index] + this.offsets[index] - 2)),
        };
        const passes = planGumpBackground(rect, clipRect(rect, clip), resources.textureMetrics.width, resources.textureMetrics.height, HIGHLIGHT_FLAGS);
        drawGumpBackground(surface, resources.texture, passes, resources.shadeMaps);
    }

    // 구분선을 그린다: 행 가운데에 가로 1픽셀 선을 어두운 질감 → 1픽셀 아래 밝은 질감 순서로 두 번 그린다(원본 MenuGumpDraw::Separator).
    _drawSeparator(surface, resources, index, point, clip) {
        const y = ftol(this.offsets[index] + MARGIN_Y - (this.baseHeight - this.height) + this.heights[index] * 0.5 - 1);
        const right = ftol(this.width + 2 - 4);
        const rect = { left: ftol(point.x + 2), top: ftol(point.y + y), right: ftol(point.x + right), bottom: ftol(point.y + y + 1) };
        // 두 번째 패스는 첫 패스 사각형을 1픽셀 아래로 옮겨 그린다(위·아래 모두 +1 누적).
        for (let pass = 0; pass < 2; pass += 1) {
            rect.top = (rect.top + pass) | 0;
            rect.bottom = (rect.bottom + pass) | 0;
            const passClip = clipRect(rect, clip);
            const passes = planGumpBackground(rect, passClip, resources.textureMetrics.width, resources.textureMetrics.height, pass ? SEPARATOR_SECOND_FLAGS : SEPARATOR_FIRST_FLAGS);
            drawGumpBackground(surface, resources.texture, passes, resources.shadeMaps);
        }
    }
}
