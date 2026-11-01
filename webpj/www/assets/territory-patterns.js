// 영역(Territory) 패턴 표이다. 원본 `Territory` 레코드의 모양 번호(첫 바이트 하위 6비트)가 고르는 패턴이며, 한 패턴은 폭 × 높이 칸의 청크 배치 틀이다.
// 원본 실행 파일 0x5300f0 의 68개(각 72바이트)를 cpppj `CanonDecoderTables.inc` 에서 옮긴 것이다. 모양 번호는 6비트라 앞의 64개만 고를 수 있으나 원본 표를 그대로 둔다.
// 이 파일은 webpj/tools/make-territory-patterns.py 가 만든다. 직접 고치지 않는다(고치면 `--check` 와 tests/territory.test.mjs 가 알려 준다).
// 칸은 줄 우선 순서(왼쪽 위에서 오른쪽, 아래 줄)이다. 칸마다 세 글자가 한 위치에 대응한다:
//  - sides: 청크의 방향(모서리) 글자 'A'~'P', 빈 칸은 '.'  - variations: 변형 숫자 글자('1' 등, 빈 칸은 '.')  - labels: 번호 글자('a'~), 없으면 '-'.
// (2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계 영역 배치를 위해 추가)

// 원본 패턴 표의 항목 수이다(모양 번호로 고를 수 있는 것은 앞의 64개). (2026-11-01 WB03-V)
export const TERRITORY_PATTERN_COUNT = 68;

// 영역 패턴 표이다. [폭, 높이, sides, variations, labels] 를 모양 번호 순서로 늘어놓고 아래에서 객체로 바꾼다. (2026-11-01 WB03-V)
const RAW_PATTERNS = [
    [3, 3, '.L.FAGINH', '.1.111111', '---------'], // 0
    [3, 3, 'FG.BAGINH', '11.111111', '---------'], // 1
    [3, 3, 'FCGBADINH', '111111111', '---------'], // 2
    [2, 2, 'OG.N', '11.1', '----'], // 3
    [3, 1, 'OKM', '111', '---'], // 4
    [3, 2, 'OKG..N', '111..1', '------'], // 5
    [3, 2, 'OCM.N.', '111.1.', '------'], // 6
    [2, 2, 'FGIH', '1111', '----'], // 7
    [3, 2, 'OG..IM', '11..11', '------'], // 8
    [3, 2, 'OCG.IH', '111.11', '------'], // 9
    [3, 2, 'FKGN.N', '1111.1', '------'], // 10
    [3, 3, '.L.OAM.N.', '.1.111.1.', '---------'], // 11
    [3, 3, 'L..IG..IM', '1..11..11', '---------'], // 12
    [3, 3, 'FKGJ.JIKH', '1111.1111', '---------'], // 13
    [3, 3, '.L.OAG.IH', '.1.111.11', '---------'], // 14
    [3, 3, '..LOCD.IH', '..1111.11', '---------'], // 15
    [3, 2, 'OCG.IH', '111.11', '------'], // 16
    [3, 3, 'FKGN.J..N', '1111.1..1', '---------'], // 17
    [3, 2, '.FMOH.', '.1111.', '------'], // 18
    [3, 3, 'L..BKGN.N', '1..1111.1', '---------'], // 19
    [3, 3, '.L.FEGN.N', '.1.1111.1', '---------'], // 20
    [4, 1, 'PPPP', '1111', 'abcd'], // 21
    [2, 3, 'PP.P.P', '11.1.1', 'abcdef'], // 22
    [2, 3, '.PPP.P', '.111.1', 'abcdef'], // 23
    [3, 2, 'PP..PP', '11..11', 'abcdef'], // 24
    [2, 2, 'PPPP', '1111', 'abcd'], // 25
    [5, 1, 'PPPPP', '11111', 'abcde'], // 26
    [4, 2, 'PPPP...P', '1111...1', 'abcdefgh'], // 27
    [4, 2, 'PPPP..P.', '1111..1.', 'abcdefgh'], // 28
    [3, 2, 'PPP.PP', '111.11', 'abcdef'], // 29
    [3, 2, 'PPPP.P', '1111.1', 'abcdef'], // 30
    [3, 3, 'P..PPP..P', '1..111..1', 'abcdefghi'], // 31
    [3, 3, '.P.PP..PP', '.1.11..11', 'abcdefghi'], // 32
    [3, 3, '..PPPP..P', '..1111..1', 'abcdefghi'], // 33
    [4, 2, 'PPP...PP', '111...11', 'abcdefgh'], // 34
    [3, 3, '.P.PPP.P.', '.1.111.1.', 'abcdefghi'], // 35
    [3, 3, 'P..PP..PP', '1..11..11', 'abcdefghi'], // 36
    [6, 1, 'PPPPPP', '111111', 'abcdef'], // 37
    [5, 2, 'PPPPP....P', '11111....1', 'abcdefghij'], // 38
    [5, 2, 'PPPPP...P.', '11111...1.', 'abcdefghij'], // 39
    [5, 2, 'PPPPP..P..', '11111..1..', 'abcdefghij'], // 40
    [5, 2, 'PPPP....PP', '1111....11', 'abcdefghij'], // 41
    [4, 3, 'PPPP...P...P', '1111...1...1', 'abcdefghijkl'], // 42
    [4, 2, 'PPPP..PP', '1111..11', 'abcdefgh'], // 43
    [4, 2, 'PPPP.P.P', '1111.1.1', 'abcdefgh'], // 44
    [4, 2, 'PPPPP..P', '11111..1', 'abcdefgh'], // 45
    [4, 3, 'P...PPPP...P', '1...1111...1', 'abcdefghijkl'], // 46
    [4, 3, '.P..PPPP...P', '.1..1111...1', 'abcdefghijkl'], // 47
    [4, 3, '..P.PPPP..P.', '..1.1111..1.', 'abcdefghijkl'], // 48
    [4, 3, '...PPPPP..P.', '...11111..1.', 'abcdefghijkl'], // 49
    [4, 3, 'PPPP..P...P.', '1111..1...1.', 'abcdefghijkl'], // 50
    [4, 2, 'PPPP.PP.', '1111.11.', 'abcdefgh'], // 51
    [3, 2, 'PPPPPP', '111111', 'abcdef'], // 52
    [3, 3, 'P..PPP.PP', '1..111.11', 'abcdefghi'], // 53
    [3, 3, '.P.PPP.PP', '.1.111.11', 'abcdefghi'], // 54
    [3, 3, '..PPPP.PP', '..1111.11', 'abcdefghi'], // 55
    [4, 2, 'PPP..PPP', '111..111', 'abcdefgh'], // 56
    [3, 3, 'P..PP.PPP', '1..11.111', 'abcdefghi'], // 57
    [3, 3, 'PPPP.P..P', '1111.1..1', 'abcdefghi'], // 58
    [4, 2, '.PPPPP.P', '.11111.1', 'abcdefgh'], // 59
    [3, 3, 'P..PPPP.P', '1..1111.1', 'abcdefghi'], // 60
    [3, 3, '.P.PPPP.P', '.1.1111.1', 'abcdefghi'], // 61
    [4, 3, '..PP..P.PPP.', '..11..1.111.', 'abcdefghijkl'], // 62
    [4, 3, 'PPP...PP..P.', '111...11..1.', 'abcdefghijkl'], // 63
    [4, 3, '...PPPPP...P', '...11111...1', 'abcdefghijkl'], // 64
    [4, 3, '.P..PPP...PP', '.1..111...11', 'abcdefghijkl'], // 65
    [4, 3, 'PP...PPP...P', '11...111...1', 'abcdefghijkl'], // 66
    [4, 3, 'PP...PP...PP', '11...11...11', 'abcdefghijkl'], // 67
];

// 영역 패턴 객체의 목록이다: { width, height, sides, variations, labels }. 모양 번호로 바로 고른다(예: TERRITORY_PATTERNS[7]). (2026-11-01 WB03-V)
export const TERRITORY_PATTERNS = Object.freeze(RAW_PATTERNS.map(([width, height, sides, variations, labels]) => Object.freeze({ width, height, sides, variations, labels })));
