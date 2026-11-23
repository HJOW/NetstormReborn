# cpppj1332 — 추가 요구 세 건의 배포 확인

2026-11-23, HJOW-Athlon. 제품1330·검사 장치1331 기준이며 마지막 디컴파일 PC도 HJOW-Athlon이다. 기존1327의 전체34미션 관문을 반복하거나 전수 동일성을 새로 판정한 단위는 아니다.

새 ZIP을 기존 `package-cpppj.ps1 -OutputPath … -NoBuild`로 생성했다.1,437항목 중복/CRC 오류0, 설정·전체화면 상태 파일 제외를 확인했고 최상위 Netstorm.exe·R.exe·한국어 번역표의 바이트가 현재 빌드/자산과 일치했다. 압축을 풀고 실제 실행한 뒤1,437파일을 ZIP 원시 바이트와 다시 비교해 모두 불변이었다.

압축 해제한 실행 파일로 한국어 버전 창과 The War Begins의 방향키·반복·뗌·채팅·다리 회전·메뉴/정지/대화창 차단2경로를 확인했다. 둘 다 종료0이며 카메라 경로의 기존 기준 보고와 실제 필드도 같았다. 최초 캠페인 탐침2회는 배포본에 동봉된 인트로의 자동 재생 중 Play Mission 버튼을 찾으려 해 실패했다. 자동 인트로를 끄는 기존 옵션 `intro=1`을 명시해 검사 준비를 교정했고 실패 로그도 보존했다. 인트로/미션 상태를 강제로 성공 처리하지 않았다.

원본1,392파일·CD425파일·실행 설치 사본1,393파일·AGENTS/README2파일, 총3,212경로/내용 SHA-256이 최초 지문과 같다. 기존 fixture와 보호 경로도 `git diff --quiet HEAD -- originals originalCD cpppj/tests/fixtures AGENTS.md README.md`로 변경 없음을 확인했다. 무관한 사용자 Saved Games는 사용하지 않았다.

ZIP: `extracted/cpp-followups-1332/NetstormReborn-11.01-cpppj.zip`,205,884,306바이트, SHA-256 `ac0ed4d34dad0c663f726a6623bd13657dc81c270f82ade1bef3c426bfc5f11c`. EXE SHA-256 `99514ed7c3b13ada9a155518880ce02421e21ba2d875805b13591f287b69da45`.

[검증 JSON](../../cpppj/followups-gate-1332-20261123.json). 원시 자료는 `extracted/cpp-followups-1328/`의 보호 전후 JSON·package 입력/보고/실행 결과, ZIP과 압축 해제본은 `extracted/cpp-followups-1332/`에 있다. 전체 스크린·청취·MP·사용자 지정 확인은 이번 검증에서도 제외했다.

1332 완료. 사용자 후속 답변에 따라 CD10.72·QuickJS 추가 연동·프레임 보간은 보류하고 일본어 번역은1333부터 이어간다. 이 ZIP은 일본어 본문 번역 전 중간 배포본이다.
