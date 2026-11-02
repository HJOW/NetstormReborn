# 777 — TYPE 시작의 실제 FILE·키 읽기

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물은 수정하지 않았다. 776 커밋 `7f903eba` 위에 준비본의 최소 변경을 합쳤다. **단위 완료·전체 목표 진행 중이다.**

원문 `0049c160`은 같은 `00459c60` 고리에서 `-2,"\\" / -2,d / -3,typeName / -4,.type`를 작성하고 `0041baa0`에 check0·현재 길이 주소·extra0을 전달한다. 실패 전 길이를 0으로 쓰지 않으며 반환 버퍼/길이가 비면 `0x51c` 진단이다. 원문 `540cf8`의 `d`를 Startup의 절대 dataPrefix로 바꾸지 않았다.

실제 Patch1078 Client의 기존 init types 자리에서만 동기 `GameAssets::TypeDefinitionRead`를 공급한다. 각 TYPE은 같은 VFS의 현재 기본/보조 root·TAFF metadata와 backing FILE/키로 읽는다. 선택한 archive의 실패를 다른 정상 파일로 숨기지 않는다. 완료된 FILE/키를 반납한 뒤 기존 DecodeOriginalText/Parse를 실행하며 원시 할당과 해독 문자열은 Parse의 정상/예외 이탈까지 보관한다. 실제 부분 fread와 빈 필수 파일은 오류로 남긴다.

기본 GameAssets 생성자·독립 CLI·Cd1072는 기존 snapshot Read/Decode/Parse를 유지한다. TYPE 순서, SHP/팔레트 읽기, 타입 표 생성은 바꾸지 않았다. callback은 동기 생성자 인수이며 장기 멤버에 저장하지 않는다. 원문 `0049c0f0` scanner/read-ahead의 정확한 EOF·offset·line·해제 시점을 복원한 것은 아니며 가짜 scanner API를 추가하지 않는다. 원문32비트 FILE/SEH ABI·미정 padding은 현대 호스트 수명 경계와 구별한다.

원문 부모의 독립47행/두 정밀도94회와 현재 재관측을 대조한다. 새 C++5개는 고정 호출/고리·실패 수명, 등록 후 현재 암호 TAFF와 snapshot 분리, 현재 디스크/보조 Unicode root, 필수 누락·빈 파일·문법 오류 뒤 재시도, 잘린 archive/typed 실패를 검사한다. QuickHelp와 기본 읽기 정책 회귀도 같은 파일 집중 대상에서 확인한다. 결과는 [보고서](../../cpppj/type-live-read-20261103.json)와 `extracted/cpp-resume-777/`에 남긴다.

새 게임·GUI·독립 CLI·멀티플레이·공중3종 전용 검사·포획/희생·특정 적 공격·승리/자연 완주·전체 C++/34경로/ZIP 검사는 실행하지 않는다. 실제 Client 접합은 소스/빌드 근거이고 새로운 GUI 관측으로 합산하지 않는다. 다음은779 공유 checking 소비자와783의 별도 부모이며 [재개 준비 인계](cpp-pause-handover-20261103.md)의 필수 교정을 따른다.

최종 Release 경고/오류0·집중 **13개(새5개)/실패0**이다. 기존 QuickHelp6개와 기본 읽기 정책2개도 통과했다. 원문47입력/94회 재관측 일치·기존 보호2712개 불변·새fixture2 포함2714개/fixture894다. EXE SHA-256은 `94e79d5aabbfdb8a98fb6e889b8ad3d8d5e5da4c8bdf10dc0223692947ea192d`, 다음 보호 기준은 `extracted/cpp-resume-777/protected-after.json`이다. 새 게임/MP/전체 C++/34/ZIP 검사는0이다.

최초 실행과 항목별 진단 실행은 각각5개 중 Native 대조1개/4단언이 실패했다. 행36~39의 allocator 반환0에서 새 검사 장치가 반환되지 않은 ALLOCATION 예약 블록을 실제 읽기 대상 span으로 넘긴 것이 원인이었다. FILE 필드·호출열·출력 길이·반환값은 이미 같았으며 예약 블록의 원문 seed는 그대로였다. 제품 코어와 기존47행 fixture를 바꾸지 않고 반환0의 물리 span을 별도로 제공했다.

같은47행을 두 차례 더 재관측해 기존 기대가 계속 같음을 확인했다(재개 후 합계282관측). 추가 fixture4행은 마지막94관측 중 반환0의4입력/8관측이며 `0041bb53`의 Read 반환 직후·File 소멸 전 바이트다. 최초 반환 후 probe는 격리 환경의 FS0 복구와 겹쳐 첫 DWORD가0으로 보였고, 이를 `native-null-destination.json`에 남긴 뒤 `native-null-after-read.json`에서 관측 지점을 교정했다. 호스트의 실제 NULL 메모리에 쓰거나 SEH의 FS0 쓰기를 합성하지 않는다. 최종 검사에서는 원래 예약 블록과 실제 읽기 대상 바이트를 모두 대조한다. 전체 검사 실행 수는 최초10회와 최종13회를 합쳐23회이며 최종 고유13개/실패0과 구별한다.
