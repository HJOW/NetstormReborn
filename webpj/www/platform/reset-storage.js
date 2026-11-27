// 게임이 소유한 설정과 캠페인 진행 항목만 지운다. 부분 실패 시 이미 지운 항목을 가능한 범위에서 복구한다. (2026-11-19 WB04-E863)
import { SETTINGS_STORAGE_KEY } from './settings.js';
import { PROGRESS_STORAGE_KEY } from './progress.js';
import { MOD_STORAGE_KEY } from '../modding/mod-store.js';

// 저장소 접근이 불가능하거나 삭제에 실패하면 오류를 호출자에게 전달한다. 다른 웹 앱의 항목은 건드리지 않는다.
export function resetGameStorage(storage) {
    if (!storage) throw new Error('게임 저장소에 접근할 수 없습니다.');
    const keys = [SETTINGS_STORAGE_KEY, PROGRESS_STORAGE_KEY, MOD_STORAGE_KEY];
    const previous = keys.map((key) => storage.getItem(key));
    let removed = 0;
    try {
        // 먼저 읽은 항목만 차례로 지우며 완료한 개수를 보존한다.
        for (const key of keys) {
            storage.removeItem(key);
            removed += 1;
        }
    } catch (error) {
        // 앞서 지워진 값의 복원을 시도하되 복원 오류가 최초 오류를 가리지 않게 한다.
        for (let index = 0; index < removed; index += 1) {
            try {
                if (previous[index] !== null) storage.setItem(keys[index], previous[index]);
            } catch {
                // 저장소가 계속 차단되면 호출자가 최초 오류를 보고한다.
            }
        }
        throw error;
    }
}
