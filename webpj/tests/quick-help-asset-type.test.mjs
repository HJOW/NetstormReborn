// 미션 월드가 없을 때(메인 메뉴 Help) 전용 도움말의 TYPE 정보 readAssetQuickHelpType 를 실제 게임 자산의 188개 TYPE 으로 검사한다. (2026-11-23 WB04-E1240)
// 원본 QuickHelpHtml.cpp ReadAssetQuickHelpType: 표의 theme·flags1·flags2·cost 와 AST 요구 에너지(mana)는 모든 번호에서, 자산 TYPE(70~185)은 정의의 range·hpPerSec(float32)·class(23바이트 이하)를 더 읽는다.
// 실제 게임·브라우저를 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameFiles } from '../www/assets/game-files.js';
import { TaffArchive } from '../www/assets/taff.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { typeNumber, typeString } from '../www/assets/type-table.js';
import { loadClientWorldAssets } from '../www/core/client-world-assets.js';
import { bindAiTypeRequiredEnergy } from '../www/core/raw-type-required-energy.js';
import { currentRiftTypeNumber, readRiftHitPoints } from '../www/core/squid-factory.js';
import { buildQuickHelpInfoHtml, readAssetQuickHelpType } from '../www/ui/quick-help-info.js';
import { readData, readDataMap } from './helpers.mjs';

// 실제 아카이브·SHP·팔레트에서 월드 자산과 현재 TYPE 188칸을 만든다. (2026-11-23 WB04-E1240)
function realAssets() {
    const files = new GameFiles({ loose: readDataMap(['d/_shapes.shp', 'd/GIFCLOUD.COL']), archives: [new TaffArchive(readData('netstorm.tarc'))] });
    const game = loadClientWorldAssets(files), types = buildTypeRecords(game.typeTable.definitions);
    return { game, types };
}

test('월드 없는 TYPE 정보: 188개 번호 모두 표 필드·요구 에너지와 정의의 range·hpPerSec·class 를 그대로 읽는다', () => {
    const { game, types } = realAssets(), mana = bindAiTypeRequiredEnergy(game.typeTable.definitions, types);
    assert.equal(types.length, 188); assert.equal(game.types().length, 116);
    let assets = 0;
    for (let type = 0; type < types.length; type += 1) {
        const info = readAssetQuickHelpType(game, types, type), record = types[type];
        assert.deepEqual([info.theme, info.flags1, info.flags2, info.cost], [record.theme, record.flags1, record.flags2, record.cost], `표 필드 ${type}`);
        assert.equal(info.mana, mana(type), `요구 에너지 ${type}`);
        if (type >= 70 && type < 186) {
            const definition = game.types()[type - 70].definition; assets += 1;
            assert.ok(Object.is(info.range, Math.fround(typeNumber(definition, 'range') ?? 0)), `range ${type}`);
            assert.ok(Object.is(info.damage, Math.fround(typeNumber(definition, 'hpPerSec') ?? 0)), `damage ${type}`);
            assert.equal(info.classification, (typeString(definition, 'class') ?? '').split('\0', 1)[0], `class ${type}`);
            assert.ok(info.classification.length <= 23);
        } else assert.deepEqual([info.range, info.damage, info.classification], [0, 0, ''], `내장 TYPE ${type}`);
    }
    assert.equal(assets, 116);
});

test('월드 없는 TYPE 정보: 실제 대표 TYPE 값(사제·선캐논·다리·제단)과 float32 저장', () => {
    const { game, types } = realAssets(), find = (name) => types.findIndex((record) => record.name.toLowerCase() === name.toLowerCase());
    const priest = readAssetQuickHelpType(game, types, find('priest')), cannon = readAssetQuickHelpType(game, types, find('sunCannon'));
    assert.deepEqual([priest.range, priest.damage, priest.classification, priest.cost], [30, 0, 'High Priest', 0]);
    assert.deepEqual([cannon.range, cannon.damage, cannon.classification, cannon.cost, cannon.mana, cannon.theme], [20, 16, 'Shooter', 400, 's', 3]);
    assert.equal(readAssetQuickHelpType(game, types, find('bridge')).classification, 'Bridge');
    // 모든 range·damage 는 float32 로 표현되는 값이다.
    for (let type = 70; type < 186; type += 1) { const info = readAssetQuickHelpType(game, types, type); assert.equal(Math.fround(info.range), info.range); assert.equal(Math.fround(info.damage), info.damage); }
});

test('월드 없는 TYPE 정보: 범위 밖 번호와 23바이트를 넘는 class 는 조용히 넘기지 않는다', () => {
    const { game, types } = realAssets();
    for (const bad of [-1, 188, 1.5, Number.NaN]) assert.throws(() => readAssetQuickHelpType(game, types, bad), /TYPE 범위/u);
    assert.throws(() => readAssetQuickHelpType(null, types, 82), /TYPE 범위/u);
    // class 속성이 24바이트인 가짜 자산 정의 하나로 상한을 확인한다(NUL 뒤는 읽지 않는다).
    const long = { typeTable: game.typeTable, types: () => [{ definition: { properties: [{ name: 'class', value: 'x'.repeat(24) }] } }] };
    assert.throws(() => readAssetQuickHelpType(long, types, 70), /class 23바이트/u);
    const nul = { typeTable: game.typeTable, types: () => [{ definition: { properties: [{ name: 'class', value: `${'x'.repeat(5)}\0${'y'.repeat(40)}` }] } }] };
    assert.equal(readAssetQuickHelpType(nul, types, 70).classification, 'xxxxx');
});

test('월드 없는 TYPE 정보 → 도움말 HTML: 선캐논의 Hits·Range·Damage·Cost·요구 에너지가 표와 같다', () => {
    const { game, types } = realAssets(), type = types.findIndex((record) => record.name === 'sunCannon'), record = types[type];
    // 메인 메뉴 도움말의 체력 장치(원본 MakeCurrentTypeHitPoints): 진단·1/4 HP 가 꺼진 현재 전역에서 레코드의 최대 HP 를 읽는다.
    const hitPoints = () => readRiftHitPoints(record, { checking: () => 0, quarter: () => 0, priest: () => 158, dais: () => 165 }, { number: () => currentRiftTypeNumber(record, types, types), assertion: () => assert.fail('진단 없음') });
    const identity = (value) => value;
    const html = buildQuickHelpInfoHtml({ type: () => readAssetQuickHelpType(game, types, type), description: () => 'Sun Cannon', hitPoints, translate: identity, substitute: identity, config: () => 'Energy' });
    assert.match(html, /<h2>Sun Cannon<\/h2>/u); assert.match(html, /Alignment: <c>Sun<\/c>/u); assert.match(html, /Class: <c>Shooter<\/c>/u);
    assert.match(html, /Hits: <c>600<\/c>/u); assert.match(html, /Range: <c>20<\/c>/u); assert.match(html, /Damage: <c>16<\/c>/u); assert.match(html, /Cost in Storm Power: <c>400<\/c>/u);
    // 1/4 HP 가 켜지면 사제 번호와 같은 TYPE 만 4분의 1 이 된다(선캐논은 그대로).
    assert.equal(readRiftHitPoints(record, { checking: () => 0, quarter: () => 1, priest: () => 158, dais: () => 165 }, { number: () => type }), 600);
    const priest = types[158];
    assert.equal(readRiftHitPoints(priest, { checking: () => 0, quarter: () => 1, priest: () => 158, dais: () => 165 }, { number: () => 158 }), Math.trunc(priest.maxHitPoints / 4));
});
