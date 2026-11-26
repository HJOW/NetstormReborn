// 추가 안내의 실제8비트 출력·입력 투과·크기 변경과 부모/자기 수명을 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { FontSet, ALL_FONT_PATHS } from '../www/assets/font-set.js';
import { UnicodeFontProvider } from '../www/assets/unicode-font.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { TextGumpNode } from '../www/ui/text-gump.js';
import { TutorialObjectiveGump, wrapTutorialObjective } from '../www/ui/tutorial-objective-gump.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { readData } from './helpers.mjs';

// 실제 원본 글꼴과 필요 시 명시Unicode 픽셀 장치를 빌린다. 브라우저의 갈무리 실화면 검사는TUT-4에서 별도로 한다.
function fonts(unicode = false) {
    const provider = unicode ? new UnicodeFontProvider({ glyph: () => ({ advance: 12, width: 1, height: 1, left: 0, top: 0, coverage: Uint8Array.of(255) }) }) : undefined;
    return new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)])), { unicode: provider });
}
// 현재 등록표의 실제 TextGump만 읽으며 표시기 내부 기록을 복제하지 않는다.
function labels(root) { return [...root.createState().entries].filter(Boolean).filter((token) => root.node(token) instanceof TextGumpNode); }
// 기존 출력기와 원본 클립으로8비트 표면을 합성한다. sentinel은 배경을 덮은 픽셀을 구별한다.
function pixels(root, width, height) {
    const result = new Uint8Array(width * height).fill(157), state = new GumpDrawState();
    setGumpDrawClip(state, { left: 0, top: 0, right: width, bottom: height });
    const target = new GumpPixelTarget(result, width, height, state);
    for (const token of labels(root)) root.draw(token, state, target);
    return result;
}

// 기대 줄은 독립 문자 폭으로 고정한다. 한 글자가 폭을 넘더라도 그 글자 전체를 소비해야 한다.
test('튜토리얼 표시: 영문 단어·긴 단어·개행·혼합 문자의 완전한 글자 줄바꿈', () => {
    const measure = (value) => Array.from(value).length;
    assert.deepEqual(wrapTutorialObjective(' one two three ', 7, measure), ['one two', 'three']);
    assert.deepEqual(wrapTutorialObjective('abcdefgh', 3, measure), ['abc', 'def', 'gh']);
    assert.deepEqual(wrapTutorialObjective('a\n\n b', 10, measure), ['a', 'b']);
    assert.deepEqual(wrapTutorialObjective('사제🙂와 신전', 3, measure), ['사제🙂', '와', '신전']);
    assert.deepEqual(wrapTutorialObjective('🙂', 1, () => 2), ['🙂']);
    assert.throws(() => wrapTutorialObjective('a', 0, measure), /실제 글꼴/);
});

// 세 화면비의 배치와 실제 투명 글리프를 검사한다. 안내 영역의 클릭/도움말 후보는 생성 전후 같아야 한다.
test('튜토리얼 표시:4:3·16:9·16:10의 배경 없는 실제 글리프와 입력 투과', () => {
    for (const [width, height] of [[1024, 768], [1366, 768], [1280, 800]]) {
        const root = new ClientGumpRoot(width, height, () => 0, () => {}), view = { left: 85, top: 0, right: width, bottom: height }, font = fonts();
        const underlying = root.createBase(8, width - 85, height, 1 | 0x200 | 0x800, () => 1); root.attach(underlying, 85, 0);
        const owner = new TutorialObjectiveGump(root, font), text = 'Press F4 to return to your island, or place a bridge.';
        try {
            const point = { x: width - 60, y: height - 15 }, before = [root.events().pickTopLevel(point), root.events().pick(point, 0), root.events().pick(point, 0x200)];
            assert.deepEqual(before, [underlying, underlying, underlying]);
            assert.equal(owner.sync(text, view, 241, 9), true); assert.equal(owner.text(), text);
            const tokens = labels(root); assert.ok(tokens.length > 0);
            for (const token of tokens) {
                const node = root.node(token), bounds = root.collection().bounds(token);
                assert.deepEqual([...node.textContext], [2, 0, 241, 0, 0, 9, 1, 1]);
                assert.ok(bounds.left >= view.left + 8); assert.ok(bounds.right <= view.right - 7); assert.ok(bounds.bottom <= view.bottom - 7);
                // 그림자/패딩을 제외한 줄 폭은440을 넘지 않는다.
                assert.ok(node.width <= 443);
            }
            assert.deepEqual([root.events().pickTopLevel(point), root.events().pick(point, 0), root.events().pick(point, 0x200)], before);
            const output = pixels(root, width, height); let ink = 0, shadow = 0, untouched = 0;
            // 기본 배경색 채우기나 반투명 상자 없이 본문/그림자 두 색만 추가할 수 있다.
            for (let index = 0; index < output.length; index += 1) {
                const color = output[index]; if (color === 157) { untouched += 1; continue; }
                assert.ok(color === 241 || color === 9); const x = index % width, y = Math.floor(index / width);
                assert.ok(x >= view.left + 8 && x < width - 7 && y < height - 8 && y >= height - 60);
                if (color === 241) ink += 1; else shadow += 1;
            }
            assert.ok(ink > 100); assert.ok(shadow > 50); assert.ok(untouched > width * height - 6000);
            assert.equal(owner.sync(text, view, 241, 9), false); assert.deepEqual(labels(root), tokens);
            owner.clear(); assert.equal(owner.text(), ''); assert.deepEqual(labels(root), []); assert.equal(owner.sync('', view, 241, 9), false);
        } finally { owner.clear(); root.dispose(); }
    }
});

// 한국어·영문 혼합을 같은Unicode 경계로 출력하고 부모가 먼저 지운 줄은 다시 생성한다.
test('튜토리얼 표시: 혼합 글자·크기 변경·부모 삭제·숨김에서 이전 노드를 반납한다', () => {
    const root = new ClientGumpRoot(640, 480, () => 0, () => {}), owner = new TutorialObjectiveGump(root, fonts(true)), view = { left: 85, top: 0, right: 640, bottom: 480 };
    const text = 'F4를 눌러 내 섬으로 돌아오거나 다리를 놓으십시오.';
    try {
        owner.sync(text, view, 241, 9); const first = labels(root); assert.ok(pixels(root, 640, 480).includes(241));
        root.resize(1280, 800); owner.sync(text, { ...view, right: 1280, bottom: 800 }, 241, 9);
        for (const token of first) assert.equal(root.node(token), null);
        const second = labels(root); assert.ok(second.every((token) => root.collection().bounds(token).left > 700));
        for (const token of second) root.remove(token);
        assert.equal(owner.sync(text, { ...view, right: 1280, bottom: 800 }, 241, 9), true); assert.ok(labels(root).length > 0);
        owner.sync('', view, 241, 9); assert.deepEqual(labels(root), []);
        owner.sync(text, { left: 0, top: 0, right: 16, bottom: 16 }, 241, 9); assert.equal(owner.text(), ''); assert.deepEqual(labels(root), []);
        assert.throws(() => owner.sync('a\nb\nc\nd', { left: 0, top: 0, right: 50, bottom: 20 }, 241, 9), /표시 높이/); assert.deepEqual(labels(root), []);
    } finally { owner.clear(); root.dispose(); }
});
