// cpppj 의 가상 표 진입 자료(.inc)에서 웹 자료 모듈 두 개를 만들고 검사하는 도구이다.
//  - cpppj/src/o/EnergyProviderVtables.inc  → webpj/www/core/energy-provider-vtables.js (실제 보호 PE 83개 가상 표의 +a0 에너지 공급 진입 [가상 표, +a0])
//  - cpppj/src/o/CampaignDamageVtables.inc → webpj/www/core/campaign-damage-vtables.js (실제 가상 표 83개의 +98 피격 진입과 +9c 방어 진입 { vtable, address })
// 사용(저장소 루트 또는 webpj 에서):
//   node webpj/tools/make-vtable-tables.mjs --write   → 두 모듈을 다시 쓴다.
//   node webpj/tools/make-vtable-tables.mjs --check   → 현재 모듈이 .inc 의 재생성 결과와 같은지 본다(다르면 종료 코드 1). tests/vtable-tables.test.mjs 가 같은 파싱 함수로 검사한다.
// .inc 는 cpppj 가 현재 보호 PE 와 다시 확인하는 자료이므로 이 도구는 값을 계산하지 않고 옮기기만 한다. 로컬 파일만 읽고 쓰며 네트워크를 쓰지 않는다.
// (2026-11-10 WB04-E250: 3-C EnergyProviderVtables·CampaignDamageVtables 이식으로 추가)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// 저장소 루트와 입력/출력 경로이다. (2026-11-10 WB04-E250)
const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
export const ENERGY_INC = path.join(ROOT, 'cpppj', 'src', 'o', 'EnergyProviderVtables.inc');
export const DAMAGE_INC = path.join(ROOT, 'cpppj', 'src', 'o', 'CampaignDamageVtables.inc');
export const ENERGY_MODULE = path.join(ROOT, 'webpj', 'www', 'core', 'energy-provider-vtables.js');
export const DAMAGE_MODULE = path.join(ROOT, 'webpj', 'www', 'core', 'campaign-damage-vtables.js');

// 이름이 name 인 constexpr 배열 본문에서 `{0x…U,0x…U}` 쌍을 순서대로 읽는다. 배열이 없거나 쌍이 아닌 항목이 섞이면 거부한다. (2026-11-10 WB04-E250)
export function parsePairs(text, name) {
    const start = text.indexOf(name);
    if (start < 0) throw new Error(`배열 ${name} 없음`);
    const end = text.indexOf('}};', start);
    if (end < 0) throw new Error(`배열 ${name} 끝 없음`);
    const body = text.slice(text.indexOf('{{', start) + 2, end);
    const pairs = [];
    // 주석을 뺀 줄마다 하나의 [가상 표, 주소] 쌍이어야 한다.
    for (const line of body.split(/\r?\n/)) {
        const code = line.replace(/\/\/.*$/, '').trim();
        if (code === '') continue;
        const match = /^\{0x([0-9a-fA-F]{8})U,\s*0x([0-9a-fA-F]{8})U\},?$/.exec(code);
        if (!match) throw new Error(`쌍이 아닌 항목: ${code}`);
        pairs.push([parseInt(match[1], 16), parseInt(match[2], 16)]);
    }
    return pairs;
}

// 에너지 공급 진입(83쌍) 모듈 본문이다. (2026-11-10 WB04-E250)
export function renderEnergyModule(pairs) {
    const hex = (value) => `0x${value.toString(16).padStart(8, '0')}`;
    const rows = [];
    // 네 쌍씩 한 줄로 묶는다.
    for (let index = 0; index < pairs.length; index += 4) rows.push(`    ${pairs.slice(index, index + 4).map(([vtable, entry]) => `[${hex(vtable)}, ${hex(entry)}]`).join(', ')},`);
    return `// 에너지 공급원 가상 표의 +a0 진입 자료이다(cpppj \`o/EnergyProviderVtables.inc\`, 실제 보호 PE 10.78 의 83개 가상 표). 이 파일은 tools/make-vtable-tables.mjs 가 만든다(손으로 고치지 않는다).
//  - ENERGY_PROVIDER_ENTRIES: [가상 표 주소, +a0 함수 주소] 83쌍. 기본 공급 0x0041c0f0 과 공급형 0x0041c170 두 함수가 .inc 의 전부이다.
//    호스트 함수 주소로 호출하지 않으며, raw-energy-supply.js 의 energyProviderEntry 가 현재 raw 슬롯의 가상 표를 이 표에서 찾아 공급 몸체를 분배한다.
// 검사: tests/vtable-tables.test.mjs 와 \`node webpj/tools/make-vtable-tables.mjs --check\`. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E250: 3-C EnergyProviderVtables 이식으로 추가)
export const ENERGY_PROVIDER_ENTRIES = Object.freeze([
${rows.join('\n')}
].map((pair) => Object.freeze(pair)));
`;
}

// 피격/방어 진입(각 83쌍) 모듈 본문이다. (2026-11-10 WB04-E250)
export function renderDamageModule(hit, defense) {
    const hex = (value) => `0x${value.toString(16).padStart(8, '0')}`;
    const rows = (pairs) => pairs.map(([vtable, address]) => `    { vtable: ${hex(vtable)}, address: ${hex(address)} },`).join('\n');
    return `// 캠페인 피해 가상 표 진입 자료이다(cpppj \`o/CampaignDamageVtables.inc\`, 실제 보호 PE 10.78 의 +98 피격·+9c 방어 주소 83쌍씩). 이 파일은 tools/make-vtable-tables.mjs 가 만든다(손으로 고치지 않는다).
//  - CAMPAIGN_DAMAGE_HIT_ENTRIES: { vtable, address } 83개. 가상 +98 이 일반 Base 피격 0x004add10, Damageable 피격 0x0044c8e0, 그 밖의 사제·신전·특수 피해(주소로 구별, 엄격한 후속 경계)이다.
//    raw-damageable-hit.js 의 makeDamageableGunHitHooks(damage, entries, hooks) 가 현재 raw 가상 표와 같은 진입만 Base/Damageable 몸체로 분배한다.
//  - CAMPAIGN_DAMAGE_DEFENSE_ENTRIES: { vtable, address } 83개. 가상 +9c 방어가 기본 false 와 TYPE 142 의 특별 방향 방어를 원본 주소 그대로 구별한다(미복원 진입은 성공으로 바꾸지 않는다).
// 호스트 함수 주소로 호출하지 않는다. 검사: tests/vtable-tables.test.mjs 와 \`node webpj/tools/make-vtable-tables.mjs --check\`. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E250: 3-C CampaignDamageVtables 이식으로 추가)
export const CAMPAIGN_DAMAGE_HIT_ENTRIES = Object.freeze([
${rows(hit)}
].map((entry) => Object.freeze(entry)));

export const CAMPAIGN_DAMAGE_DEFENSE_ENTRIES = Object.freeze([
${rows(defense)}
].map((entry) => Object.freeze(entry)));
`;
}

// 두 .inc 를 읽어 모듈 본문 둘을 만든다. (2026-11-10 WB04-E250)
export function renderAll() {
    const energy = parsePairs(fs.readFileSync(ENERGY_INC, 'utf8'), 'kEnergyProviderEntries');
    const damage = fs.readFileSync(DAMAGE_INC, 'utf8');
    return {
        energy: renderEnergyModule(energy),
        damage: renderDamageModule(parsePairs(damage, 'kCampaignDamageHitEntries'), parsePairs(damage, 'kCampaignDamageDefenseEntries')),
    };
}

// 줄바꿈 차이는 무시하고 비교한다(작업 트리 CRLF 변환). (2026-11-10 WB04-E250)
const normal = (text) => text.replace(/\r\n/g, '\n');

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    const mode = process.argv[2];
    const generated = renderAll();
    if (mode === '--write') {
        fs.writeFileSync(ENERGY_MODULE, generated.energy);
        fs.writeFileSync(DAMAGE_MODULE, generated.damage);
        console.log('가상 표 자료 모듈 두 개를 다시 만들었다.');
    } else if (mode === '--check') {
        const same = [[ENERGY_MODULE, generated.energy], [DAMAGE_MODULE, generated.damage]].every(([file, text]) => fs.existsSync(file) && normal(fs.readFileSync(file, 'utf8')) === text);
        console.log(same ? '가상 표 자료 모듈이 .inc 와 같다.' : '가상 표 자료 모듈이 .inc 와 다르다(--write 로 다시 만든다).');
        process.exit(same ? 0 : 1);
    } else {
        console.log('사용: node webpj/tools/make-vtable-tables.mjs --write | --check');
        process.exit(2);
    }
}
