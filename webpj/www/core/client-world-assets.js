// 앱이 받은 같은 가상 파일에서 캠페인 TYPE·SHP·팔레트 집계를 구성한다.
// 사용: loadClientWorldAssets(gameData.files, palettePath). 독립 Session이 아닌 Client 수명에서 한 번 보유한다.
import { GameAssets, DEFAULT_GAME_PALETTE_PATH } from '../assets/game-assets.js';
import { GamePalette } from '../assets/palette.js';
import { ShapeDatabase } from '../assets/shapes.js';
import { decodeOriginalText } from '../assets/text-encoding.js';
import { TypeTable } from '../assets/type-table.js';

// 실제 파일 조회를 사용하며 빠진 필수 자산이나 잘못된 TYPE은 즉시 거부한다.
export function loadClientWorldAssets(files, palettePath = DEFAULT_GAME_PALETTE_PATH) {
    if (!files || typeof files.read !== 'function') throw new TypeError('Client 월드 자산의 실제 파일 조회 누락');
    const shapes = files.read('d/_shapes.shp'), palette = files.read(palettePath);
    if (!shapes || !palette) throw new Error('Client 월드의 SHP 또는 팔레트 자산 누락');
    const table = TypeTable.fromArchive(files, decodeOriginalText, (name, error) => { throw new Error(`Client 월드 타입 ${name}: ${error.message}`); });
    return new GameAssets(table, new ShapeDatabase(shapes), GamePalette.parse(palette));
}
