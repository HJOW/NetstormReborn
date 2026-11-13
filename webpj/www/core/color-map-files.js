// 색표 캐시 파일(`!color.dat`)의 웹 적응 저장소이다(cpppj `client/ClientColorMapFile.cpp` 의 FILE/BaseFileSystem 대응, 분류 adapt-file).
// 웹에는 쓸 수 있는 설치 폴더가 없으므로 이 저장소는 페이지 수명 동안의 메모리 파일 목록이다. 첫 실행은 캐시 miss(열 수 없음)로 9표를 계산해 쓰고, 같은 페이지의 다음 소유자는 hit 로 읽는다.
//  - read(path): 파일의 현재 전체 내용 사본(Uint8Array)이거나 없으면 null. 읽는 쪽이 부분만 소비해도 저장 내용은 바뀌지 않는다.
//  - createWriter(path): 쓰기용으로 열고(생성/비우기) { write(bytes), close() } 를 돌려준다. 쓰기는 즉시 파일에 이어 붙여 도중 실패해도 부분 파일이 남고(원문 FILE 과 같음), 쓸 수 없는 저장소(writable false)이거나
//    상위 폴더가 없는 경로(directories 목록에 없는 상위)는 ColorCacheUnableToOpen 으로 거부한다.
//  - 영구 저장(localStorage 등)은 이 저장소를 교체해 연결한다(5차 요구, 후속). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-13 WB04-E375: 5-A ClientColorMapFile 웹 적응으로 추가)
import { ColorCacheUnableToOpen } from './raw-color-map-cache.js';

// 경로의 상위 폴더(마지막 `/` 앞)이다. 상위가 없으면 빈 문자열이다. (2026-11-13 WB04-E375)
const parentOf = (path) => {
    const slash = path.lastIndexOf('/');
    return slash < 0 ? '' : path.slice(0, slash);
};

// 메모리 색표 캐시 파일 저장소이다. 사용: const files = new MemoryColorMapFiles({ directories: ['cache'] }); input.initializeCache(files, 'cache/!color.dat', red, initializeOwners). (2026-11-13 WB04-E375)
export class MemoryColorMapFiles {
    // 경로 → 파일 내용, 쓰기 가능 여부, 존재하는 상위 폴더 목록(생략하면 상위 폴더 검사를 하지 않는다)이다.
    #files = new Map();
    #writable;
    #directories;

    // options = { writable = true, directories = null(검사 없음) }.
    constructor({ writable = true, directories = null } = {}) {
        this.#writable = Boolean(writable);
        this.#directories = directories ? new Set(directories) : null;
    }

    // 파일이 있는지 본다.
    has(path) { return this.#files.has(path); }

    // 파일 전체 내용의 사본이다(없으면 null).
    read(path) {
        const file = this.#files.get(path);
        return file ? Uint8Array.from(file) : null;
    }

    // 쓰기용으로 열어(생성/비우기) 즉시 이어 붙이는 쓰기기를 돌려준다. 열 수 없으면 ColorCacheUnableToOpen 이다.
    createWriter(path) {
        if (!this.#writable) throw new ColorCacheUnableToOpen();
        const parent = parentOf(path);
        if (this.#directories && parent !== '' && !this.#directories.has(parent)) throw new ColorCacheUnableToOpen();
        const chunks = [];
        const publish = () => {
            const bytes = new Uint8Array(chunks.reduce((sum, chunk) => sum + chunk.length, 0));
            let at = 0;
            // 이어 붙인 조각을 순서대로 합쳐 파일 내용으로 둔다.
            for (const chunk of chunks) {
                bytes.set(chunk, at);
                at += chunk.length;
            }
            this.#files.set(path, bytes);
        };
        publish();
        return {
            write: (bytes) => { chunks.push(Uint8Array.from(bytes)); publish(); },
            close: () => {},
        };
    }

    // 시험/진단용으로 파일 내용을 직접 둔다(원본 캐시 파일을 미리 놓는 장면).
    put(path, bytes) { this.#files.set(path, Uint8Array.from(bytes)); }
}
