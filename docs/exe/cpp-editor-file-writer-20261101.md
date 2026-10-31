# Edit Template 파일의 기존 끝 보존 (단위669)

실제 Template 저장 경로의 잘라쓰기를 원본 BaseFile 쓰기로 교체했다. 기존 `EditorFiles::Writer`는 `Write`의 `w+b`를 사용해 짧아진 Fort 이미지를 저장하면 파일 끝도 줄였다. 원본 `0044d8d0 → 0041bb90 → 0041b4c0 → 0041a5b0`은 mode1의 `r+b`를 먼저 시도하고 열기 실패 때만 `w+b`로 생성한다. 따라서 정상 쓰기도 기존 파일의 뒤쪽 바이트를 보존한다.

새 private Template writer는 같은 보호 경로 검사를 통과한 작업본에서 기존 `ConstructBaseFile`, `OpenBaseFileDisk(1,1)`, `WriteBaseFile(-1)`, `DestructBaseFile`을 실행한다. 초기화/열기/쓰기/닫기 알고리즘은 기존 raw 몸체를 재사용하며, 실제 CRT 경계만 기존 Unicode open 장치에 연결한다. Windows10/11의 한글 경로는 원본 ANSI 대신 W CRT를 사용한다고 주석에 명시했다.

`0041a910`의 `fwrite(data,size,1)`과 `0041bb90`의 fclose 반환은 새 예외로 바꾸지 않는다. 열기 실패까지 무시하는 것은 아니며 두 open이 실패하면 기존 오류를 유지한다. 실제 부분 쓰기나 fclose 오류를 새로 재현한 것으로 주장하지 않는다. 일반 `Write`·준비·복사의 잘라쓰기는 그대로다. 부모 폴더를 임의 생성하거나 보호 원본·작업본 밖 쓰기를 허용하지 않는다.

## 검증

Release 게임/Editor 파일/BaseFileIO 집중의 빌드 명령1개가 경고/오류0으로 끝났다. Editor36등록과 BaseFile7등록 중 관련28개/실패0이다(새5·기존23, 총5회/28함수 호출). Writer4·EditorFiles8·파일장치9·Unicode4·BaseFileIO3이며 최초 검사 실패는 없다. 두 등록 전체나 주 검사 전체 통과로 세지 않는다.

Unicode 실제 임시 파일에서 tail/빈 쓰기/확장/새 파일/보호/쓰기 공유 거부와 실제 SaveFile의 현재 이름/서명을 확인했다. 기존 TEST01의 작업 사본에72BYTE 빈 Template를 저장하고 뒤쪽 원문이 남으며35섹션으로 재파싱되는 것도 확인했다. 기존 BaseFile322행은 C++ 재생이며 새 Native 실행이 아니다. 실제 부분 쓰기/닫기 반환 실패는 이번 새 실제 관측 범위가 아니다.

제품 SHA-256은 `a8bfcf707fc69e833692dc99df3e403158895803def6b6e918903924a45584a6`다. 관련 보호5개(AGENTS/양PE/TAFF/TEST01)는 전후 불변이며 앞4개는668 기준과도 일치한다. 전체2632/fixture773 해시 재감사는 하지 않았다. 상세와 원시 경로는 [보고서](../../cpppj/editor-file-writer-20261101.json)를 따른다. 게임 실행·전체34·새ZIP·신규MP 검사0이다. 최신 전체34/ZIP은645의644제품이며 이번 제품의 GUI 근거로 확대하지 않는다.

원문 근거는 현행 `extracted/refined/originals/Netstorm.c`의 위 함수와 기존651/652 BaseFile 복원이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon /2026-10-23**이며 이번 디컴파일 산출물 변경은 없다. 새 Native 실행/fixture 생성·신규MP 검사는 하지 않는다.10.72의 일반/부분 open 대응은652의 의미 대조를 유지하며 이번 추가 판본 검증은 아니다.

## 물리 입력 환경 재확인

이전637/640/641 캡처 실패 이후 새 활성 RDP 세션(2026-11-01 00:37 로그인, 세션2)을 확인했다. Computer Use로 현재 Visual Studio Code 창을 반환 객체에서 선택해 캡처를 시도했으나 `FrameArrived timed out: timed out waiting on channel`이었다. 창 목록/대상을 다시 선택한 한 번의 복구 시도도 `window capture timed out: timed out waiting on channel`로 실패했다. 키·마우스 입력0, 게임/별도 검사용 창 실행0이다. 게임 고유 결함으로 판정하지 않고 새 환경 변화 전 같은 시도를 반복하지 않는다.
