// 실제 앱의 입자 색 표에 공급할 명시 메모리 입력이다. 원본의 보편 상수가 아니라 검증한 실행 환경의 관측값이다.
// CPP1370 x64 private heap의 건설17은11DWORD 바로 뒤에서0을 읽었다(E1380). 다른 부류는 기존 x86 FieryTail 관측 기본1을 유지한다.
export const CLIENT_PARTICLE_PALETTE_INPUT = Object.freeze({
    source: 'cpp1370-x64-construction-private-heap',
    adjacentByKind: Object.freeze({ construction: 0 }),
});
