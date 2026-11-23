// MissionBegin 단추의 미션 이름 별칭(원본 오타 `BC1Menu` → `BC1Menu76`)을 고정한다. (2026-11-23 WB04-E1245)
// 10.78 의 Priest Training 7(bc7sun_bars76) 완료 안내 "Training Menu" 단추만 접미 없는 `BC1Menu` 를 가리켜 웹에서 아무 일도 없었다.
// 이 시험은 별칭 해석 함수와, 이름이 실제 훈련 대본 파일들의 철자와 맞물려 있는지를 파일에서 직접 읽어 확인한다. 브라우저·게임은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, readdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { MISSION_NAME_ALIASES, resolveMissionBeginName } from '../www/core/mission-name-alias.js';

// 한국어 번역 대본이 모인 폴더의 절대 경로이다(번역 파일은 원본 대본의 `$Button=` 줄 구조를 그대로 둔다). (2026-11-23 WB04-E1245)
const TRANSLATIONS = fileURLToPath(new URL('../translations/', import.meta.url));

// 번역 폴더의 훈련 대본(bc*76.korean) 안 `MissionBegin,이름` 단추 줄을 (파일, 이름) 목록으로 모은다. (2026-11-23 WB04-E1245)
function trainingBeginTargets() {
    const targets = [];
    for (const file of readdirSync(TRANSLATIONS).filter((name) => /^bc\d.*76\.korean$/u.test(name)).sort()) {
        for (const line of readFileSync(`${TRANSLATIONS}${file}`, 'utf8').split(/\r?\n/u)) {
            const match = /^\$Button=.*,MissionBegin,([^,\s]+)\s*$/u.exec(line);
            if (match) targets.push({ file, name: match[1] });
        }
    }
    return targets;
}

test('BC1Menu 는 대소문자·앞뒤 공백과 무관하게 BC1Menu76 으로 풀린다', () => {
    for (const written of ['BC1Menu', 'bc1menu', 'BC1MENU', ' BC1Menu ']) assert.equal(resolveMissionBeginName(written), 'BC1Menu76', written);
});

test('별칭이 아닌 이름은 그대로 돌려주고 누락 인자는 빈 문자열이다', () => {
    for (const written of ['BC1Menu76', 'bc8fury76', 'Tutorial3', 'TheWarBegins', 'BC2Bridging', '']) assert.equal(resolveMissionBeginName(written), written, written);
    assert.equal(resolveMissionBeginName(undefined), '');
    assert.equal(resolveMissionBeginName(null), '');
});

test('별칭 표는 동결돼 있고 소문자 열쇠만 가진다', () => {
    assert.ok(Object.isFrozen(MISSION_NAME_ALIASES));
    assert.deepEqual(Object.keys(MISSION_NAME_ALIASES), ['bc1menu']);
    for (const key of Object.keys(MISSION_NAME_ALIASES)) assert.equal(key, key.toLowerCase());
});

test('훈련 대본의 BC1Menu 단추는 bc7sun_bars76 의 한 줄뿐이고 나머지는 이미 76 접미를 쓴다', () => {
    const targets = trainingBeginTargets(), bare = targets.filter((target) => target.name.toLowerCase() === 'bc1menu');
    assert.deepEqual(bare, [{ file: 'bc7sun_bars76.korean', name: 'BC1Menu' }], '접미 없는 BC1Menu 단추');
    // 별칭이 풀어 주는 이름과 다른 훈련 대본들이 쓰는 철자가 같아야 같은 미션으로 이어진다(대소문자까지 같게 둔다).
    const suffixed = targets.filter((target) => target.name.toLowerCase() === 'bc1menu76');
    assert.ok(suffixed.length >= 15, `BC1Menu76 단추 ${suffixed.length}개`);
    for (const target of suffixed) assert.equal(target.name, resolveMissionBeginName('BC1Menu'), target.file);
});

test('별칭을 푼 뒤의 모든 훈련 MissionBegin 이름은 연결된 훈련 미션 목록 안에 있다', () => {
    const connected = new Set(['bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76']);
    for (const target of trainingBeginTargets()) assert.ok(connected.has(resolveMissionBeginName(target.name).toLowerCase()), `${target.file} → ${target.name}`);
});
