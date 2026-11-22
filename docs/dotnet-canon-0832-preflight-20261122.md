# dotnetpj 0.832 사전 — 기존 명암 표 검사의 값 스냅샷 교정

작업일2026-11-22, PC HJOW-Athlon. 정기 전체 첫 관문82560은 Assets366·Core3557통과, Client1997통과/1실패로 종료1이었다. 총5920통과/1실패이며 기존 실장치 음향1건은 미실행이다. 실패 뒤65시작은 실행하지 않았다.

실패한 `GumpBackgroundTests.MapsRemainUntilNextPaletteFile`는 새 팔레트 파일에서 명암 값이 바뀌는지 확인하면서 이전 표의 객체 참조를 보관했다.0.830에서 Source `PaletteShade.h`의 두 std::array 값형과 `Screen.cpp`의 멤버 배열 값 대입/참조 반환에 맞춰 화면 내부 배열을 유지하도록 이식했으므로, 그 참조도 갱신된 현재 값을 가리킨다. 원문대로 두 행의 값을 복사하는 제품 동작을 유지했다.

검사는 이전 Bright/Dark BYTE를 별도 배열에 복사해 임시 팔레트 변경에서 값 보존, 새 파일 적재에서 값 변경을 비교하도록 고쳤다. 화면의 같은 표 객체도 함께 확인한다. 검사 이름과 입력·변경 기대를 유지했으며 제품 코드나 CPP 원문 자료/기대는 변경하지 않았다.

교정본의 기존 GumpBackground/Screen/ScreenFixture와 새 ClientColorSortInput 관련38건이 모두 통과했다. 검사 세션82927 종료0이며 증빙은 `extracted/dotnet-0832/preflight.log`와 `preflight/screen.trx`다. 전체5921건/65시작 완료는 아직 주장하지 않는다. 같은 교정 Source를 새 관문에서 고정해 정기 전체0.832를 다시 수행한다. M1 0/34·전체목표 미완료이며 원격 push는 하지 않았다.
