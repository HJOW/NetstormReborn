// 시각 효과 통합 검사가 같은 실제 Session·보호 자산·명시 장치를 공유한다. 게임/AI Kernel은 실행하지 않는다.
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { parseFort } from '../www/assets/fort.js';
import { TaffArchive } from '../www/assets/taff.js';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { RawCampaignSession } from '../www/core/raw-campaign-session.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { SoundList } from '../www/audio/sound-list.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData, loadOriginalForts } from './helpers.mjs';

// 보호 자료는 읽기만 하고 TYPE·셰이프 쓰기는 Session 소유 메타데이터에 한정한다.
const data = realCaptureAssets();
export const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
const fort = parseFort(entry.bytes, createFortTypes(data.table));

// 선택하지 않은 장치는 호출 시 실패한다.
function unexpected() { throw new Error('선택하지 않은 외부 장치'); }

// 실제 논리는 그대로 실행하고 음향·진단만 수집한다. assertion은 진단 정상 복귀의 외부 입력을 줄 때 사용한다.
export function visualSessionScene({ assertion, sound = {} } = {}) {
    const gameplay = new ClientGameplayGlobals(), sounds = [], diagnoses = [];
    gameplay.camera().initializeNativeRenderer(640, 480, 256, { value: 0 }, { value: 0 });
    const session = new RawCampaignSession(game, { read: unexpected }, fort, 'thewarbegins', {
        gameplayGlobals: gameplay, unixSeconds: () => 1700000000, soundNames: new SoundList(),
        install: { translateYou: () => Uint8Array.of(89, 111, 117, 0), setConfig: () => {}, refreshWindowTitle: () => {}, resetCamera: () => {} },
        ownership: { refreshProduction: () => {}, themeMode: () => 0 }, deletion: { selected: () => 0 },
        assets: { postPop: { playImpactSound: () => {} }, queueCommand: () => {} }, player: { log: () => {} },
        sound: { wallSeconds: () => 0, play: () => 1, playOnceAt: (...args) => { sounds.push(args); return args[2]; }, playLoopAt: unexpected, stop: unexpected, isPlaying: () => false, ...sound },
        ai: { assertion: (expression) => { diagnoses.push(expression); assertion?.(expression); } },
    });
    return { session, gameplay, sounds, diagnoses };
}
