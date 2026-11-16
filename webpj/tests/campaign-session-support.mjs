// 실제 RawCampaignSession 생성자 조립을 공유하는 검사 장면이다. 게임 진행/미션 Load는 실행하지 않는다(WB04-E621).
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 미선택 외부 장치를 빈 성공으로 바꾸지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 검사 프로세스 안에서 같은 보호 자산/팔레트 한 벌을 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 실제 기반 owner와 생성자의 원문 순서를 사용한다. 외곽 Session/Load는 아직 만들지 않는다.
export function campaignOwners(extra = {}) {
    const o = new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra,
        sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden, ...extra.sound } });
    try {
        o.initializePlayersAndSp();
        // 순환 참조의 설치 순서를 바꾸지 않고 현재 World 게시/지형까지 조립한다.
        for (const name of ['PlayerTablesAndPackets', 'IslandsAndPriests', 'PlayersAndOwnership', 'BodiesAndTypeTables', 'GunAndCursorViews', 'HitBodies', 'GunBodies', 'Particles', 'Teleport', 'Combat', 'Vortex', 'Assets', 'WorldRuntimeHooks', 'ContentNotices', 'ContentLifetime', 'Production', 'Factories', 'Flare', 'Construction', 'Damageable', 'IceTower', 'CarrierSteal', 'BombPostPop', 'AcidZap', 'AcidFence', 'CarrierRegular', 'SunAviary', 'WindAviary', 'RainAviary', 'ArcSpire', 'Flyer', 'Fence', 'GemReward', 'WorldDeletion', 'WorldRegular', 'WorldTransmission', 'Terrain']) o[`connect${name}`]();
        return o;
    } catch (error) { o.dispose(); throw error; }
}
