// 실제 Session의 EvilRay 생성·표시·반납을 cpppj CampaignObjectMenuTests의 EvilRayCurrent 세 검사와 대조한다.
// 전체 게임/AI Kernel은 실행하지 않고 선택한 시각 효과만 호출한다. (WB04-E1314)
import test from 'node:test';
import assert from 'node:assert/strict';
import { EvilRayVisualProcess } from '../www/core/evil-ray-visual-system.js';
import { buildEvilRayPalette } from '../www/core/raw-evil-ray-lifetime.js';
import { drawVfxEllipse } from '../www/render/vfx-ellipse.js';
import { put, floatToBits } from './raw-support.mjs';
import { game, visualSessionScene as scene } from './campaign-visual-session-support.mjs';

// 같은 실제 Kernel에서 부모에 붙은 EvilRay 파생 객체만 찾는다.
function currentRay(core, parent) {
    // 원문 CurrentEvilRay와 같이 슬롯 순서대로 조회한다.
    for (let pid = 1; pid <= core.kernel.capacity; pid += 1) {
        const effect = core.kernel.get(pid);
        if (effect instanceof EvilRayVisualProcess && effect.parent === parent) return effect;
    }
    return null;
}

test('EvilRayCurrent: 실제 Teleport가 현재 생성자 TYPE과 source를 선택한다', () => {
    const { session: s, gameplay } = scene();
    try {
        s.load(); const c = s.core(), aux = gameplay.gameAux(), particles = s.particles();
        c.types[57] = { ...c.types[10] }; c.types[58] = { ...c.types[10] };
        aux.streakProcessType = 57; aux.evilRayProcessType = 58; aux.particleSource = 0x12345678;
        const source = c.factory.create(74), raw = c.pool.allocated(source);
        put(raw, 0, 0x5070c0); raw[11] = raw[40] = 0; raw[34] = 1; c.types[74].flags1 = c.types[74].flags2 = 0;
        const before = c.kernel.size, rendered = particles.rendering.size;
        c.bookkeeping.depth += 1; c.postPop.postPopVirtual(source, 0x2000001);
        const ray = currentRay(c, source); assert.ok(ray);
        assert.equal(c.bookkeeping.depth, 0); assert.equal(c.kernel.size, before + 1); assert.equal(particles.rendering.size, rendered + 1);
        assert.equal(c.pool.slot(ray.form)[10], 58); assert.equal(ray.fields.source, 0x12345678);
        assert.equal(ray.fields.particles.length, 50); assert.equal(ray.fields.total, 0); assert.equal(ray.fields.first, false);
        aux.particleSource = 77; c.types[59] = { ...c.types[10] }; aux.evilRayProcessType = 59;
        const next = particles.addEvilRay(source);
        assert.equal(ray.fields.source, 0x12345678); assert.equal(next.fields.source, 77); assert.equal(c.pool.slot(next.form)[10], 59);
    } finally { s.dispose(); }
});

test('EvilRayCurrent: 실제 Host에서 프레임·픽셀·Stop 통지·자기 삭제가 이어진다', () => {
    const { session: s, gameplay, sounds, diagnoses } = scene();
    let shape, savedWidth;
    try {
        s.load(); sounds.length = diagnoses.length = 0;
        const c = s.core(), particles = s.particles(), source = c.factory.create(70), raw = c.pool.allocated(source);
        raw[11] = raw[40] = 0; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21)); put(raw, 36, 0);
        gameplay.gameAux().particleSource = source; const free = c.pool.freeCount, ray = particles.addEvilRay(source);
        const form = ray.form, pid = c.kernel.find(ray), token = ray.fields.registration.token;
        shape = c.shapes[70].frames[0]; savedWidth = shape.width; shape.width += 2;
        particles.prepareFrame(1, 1); ray.runFrame();
        assert.equal(ray.fields.total, 1); assert.equal(ray.fields.particles[0].x, Math.fround(320 - Math.trunc(shape.width / 2)));
        assert.equal(ray.fields.particles[0].y, 231); assert.equal(ray.fields.particles[0].life, 13); assert.equal(sounds.length, 1);
        shape.width = savedWidth;
        const center = { x: Math.trunc(ray.fields.particles[0].x), y: Math.trunc(ray.fields.particles[0].y) };
        const pixels = new Uint8Array(1024 * 768).fill(199), expected = pixels.slice(), colors = buildEvilRayPalette(Array.from(game.palette.logical));
        drawVfxEllipse(expected, 1024, 768, { left: 0, top: 0, right: 640, bottom: 480 }, center, -1, -2, colors[2]);
        drawVfxEllipse(expected, 1024, 768, { left: 0, top: 0, right: 640, bottom: 480 }, center, 0, -1, colors[1]);
        drawVfxEllipse(expected, 1024, 768, { left: 0, top: 0, right: 640, bottom: 480 }, center, 1, 0, colors[0]);
        particles.drawEntry(token, pixels, 1024, 768, { left: 0, top: 0, right: 1, bottom: 1 });
        assert.deepEqual(pixels, expected); assert.ok(pixels.some((value) => value !== 199));
        ray.notify(3, 0x11223344, 0x55667788); ray.notify(3, 7, 8);
        assert.equal(ray.fields.stopping, 1); assert.deepEqual(diagnoses, ['stopping == false']);
        particles.prepareFrame(3, 2); ray.runFrame(); assert.ok(ray.fields.particles[0].life < 0); assert.equal(sounds.length, 2);
        ray.runFrame(); assert.equal(c.kernel.get(pid), null); assert.equal(particles.rendering.lookup(token), null);
        assert.equal(c.processes.fromForm(form), null); assert.equal(c.pool.freeCount, free);
    } finally { if (shape) shape.width = savedWidth; s.dispose(); }
});

test('EvilRayCurrent: 부모 0은 Kernel 없이 표시 등록되고 source를 읽지 않는다', () => {
    const { session: s, gameplay } = scene();
    try {
        const c = s.core(), particles = s.particles(), before = c.kernel.size, rendered = particles.rendering.size;
        gameplay.gameAux().particleSource = 0xffffffff;
        const ray = particles.addEvilRay(0);
        assert.equal(c.kernel.size, before); assert.equal(particles.rendering.size, rendered + 1);
        assert.equal(ray.form, 0); assert.equal(ray.parent, 0); assert.equal(ray.fields.source, 0xffffffff); assert.equal(ray.fields.particles.length, 50);
        const pixels = new Uint8Array(256).fill(199);
        particles.drawEntry(ray.fields.registration.token, pixels, 16, 16, { left: 0, top: 0, right: 16, bottom: 16 });
        assert.ok(pixels.every((value) => value === 199));
    } finally { s.dispose(); }
});
