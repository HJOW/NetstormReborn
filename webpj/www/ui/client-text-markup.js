// 실제 한 줄 글자 Gump에 원본 대괄호 그림·팔레트·소리 장치를 연결한다.
import { Markup, MarkupAssets, MarkupPixelTarget, MarkupState, bindMarkup, bindMarkupAssets } from './markup-picture.js';
import { StyleTextMarkup, StyleTextMarkupState, bindStyleTextMarkupAssets } from './style-text-markup.js';
import { bindStyleTextPalette } from './style-text-fragments.js';

// ClientTextGump의 스타일 훅 공장을 만든다. palette와 pictureColors는 현재 화면의 표를 반환한다.
export function makeClientTextMarkup(game, palette, pictureColors, sound, options = new StyleTextMarkupState()) {
    if (!game || [palette, pictureColors, sound].some((device) => typeof device !== 'function')) throw new TypeError('Client 글자 마크업의 실제 장치 누락');
    return (fragments, target) => {
        const assets = new MarkupAssets(game), state = new MarkupState(), pictureHooks = {}, colors = new Uint8Array(256);
        bindMarkupAssets(state, pictureHooks, assets);
        // 화면과 그림 색표는 실제 출력 순간에 조회한다.
        const raster = () => {
            const surface = target(); if (!surface) throw new Error('Client 마크업의 현재 출력 표면 누락');
            return new MarkupPixelTarget(surface, assets, colors, pictureColors()).hooks({});
        };
        pictureHooks.color = () => raster().color(); pictureHooks.shape = (request) => raster().shape(request);
        const pictures = new Markup(fragments, state, pictureHooks), markupHooks = { black: () => palette().basic[0], sound };
        bindStyleTextMarkupAssets(markupHooks, game);
        const markup = new StyleTextMarkup(fragments, pictures, options, markupHooks), hooks = {};
        bindMarkup(hooks, pictures);
        // 짧은 색 명령도 팔레트 변경 뒤의 현재 기본·명명 색을 사용한다.
        hooks.color = (command) => { const current = {}; bindStyleTextPalette(current, palette()); return current.color(command); };
        return markup.hooks(hooks);
    };
}
