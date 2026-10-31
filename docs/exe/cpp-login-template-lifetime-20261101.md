# 최초 로그인 Template 선택·읽기·보존 (단위667)

실제10.78의 State7에서 Fort를 미리 읽고 전체 파싱하던 경로를 제거했다. 최초 전역 Template 설치는 서버 로그인 수신으로 옮겼으며, State9 송신은 별도 readonly 지역 Template에서 다섯 섹션을 추린 이미지를 만든다. 이미 보유한 전역 manager는 원본 Load 부모의 이름 비교 뒤 그대로 보존한다.

## 원문과 연결

- `00460680`은 호출 순간의 현재 Fort 이름과 `fortSpec`으로 경로만 선택한다. `MissionScript::FortFileSelector`는 선택된 경로/source를 읽기 closure에 보유하며, 실제 Client는 매번 같은 장기 `FortGlobals::Current()`를 조회한다.
- `004b63d0`은 지역 Template20의 `+8=1`을 설정해 권한 조회를 생략하고 `004bf550`으로 읽는다. 전역 owner의 이미지로 대체하지 않는다.
- `004bf3e0`은 공유 직렬화 version과 이름표를 사용하는 `NONAME` manager를 생성·초기화한다. `Subscriber`, `Territory`, `TypeNames`, `CoreData`, 현재 `Terr%02d`만 기존 `CloneDataManagerStream`으로 복제하고 현재 노드/서명으로 저장한다. target/source 임시는 반환·예외에서 반납한다.
- 보호PE의 `004bf4a0`은 `005c85d0` 전체DWORD를 읽는다. `BattleTerritory()`와 기존 `BattleByte()`는 같은4BYTE를 빌리며 BYTE 쓰기는 상위24비트를 보존한다.
- `004a6c50`의 서버 로컬 설치는 실제 `LoadFile`을 사용한다. `004bf550`은 기존 manager가 있으면 CRT 대소문자 비교만 수행하고 읽기·권한·새 생성·제목 갱신을 생략한다. 다른 이름은 진단 뒤 기존 manager를 보존한다.
- `0041af80`/`0041ad10`의 권한 조회는 내용 읽기를 사용하지 않는다. 기본 disk→기본TAFF→보조disk→보조TAFF 출처와 직접 장치 경로를 구분하며 TAFF2/4는 readonly, disk는 CRT 쓰기 비트를 확인한다. Windows10/11 Unicode 경계는 원본 ANSI 대신 W CRT를 사용한다고 코드에 명시했다.

`UberGump`의 최초 `MissionPlayers::Load`는 실제 Player/SP/기술 저장소가 아닌 구형 표시/진단 사본이었다. 실제10.78에서 이 선행 설정 조회도 제거했다. 실제 값은 기존 raw Start/Player/기술 경로가 계속 소유한다. 진단 `mission` 행의 객체 수는 설치 완료 시점의 실제 월드 객체 수로 바꿨으며, 선행 저장 레코드 수를 뜻하지 않는다.10.72의 기존 부분 어댑터는 유지했고 이번 추가 복원/검증 대상은 아니다.

## 검증 상태

Release 빌드 명령2개가 경고/오류0으로 끝났다. 앞선 직렬 빌드1개는 병렬 컴파일로 재시작하려고 중단했으며 컴파일 오류로 완료된 실행과 구별한다. 최종 명령에서 변경된 헤더 의존성을 다시 확인해 게임/미션집중/세계집중을 빌드했다.

미션 집중82등록과 세계 집중261등록 중 관련26개가 실패0으로 통과했다(새10개·기존16개, 총10회/26함수 호출). 기존7개는 강제 재설치/선행 이미지 기대를 원문 최초 읽기/보존 수명에 맞춰 변경했고 나머지9개는 비동기/부모/객체 스트림 회귀다. 최초 검사 실패는 없다. 선택3·이미지2·권한3·로그인4·기존비동기2·부모2·Campaign Objects2·기존 FortObjectStream3·작업본 source3·경로2다. 등록 전체 실행이나 주 검사 전체 통과로 세지 않는다.

새 Native 실행/fixture는0이며, 기존 Patch609 Objects/부모609행을 C++에서 재생했다. 메모리 source와 실제 임시 파일 검사를 구별한다. AGENTS/양PE/TAFF의 관련4개는645·666 기준과 불변이다. 전체2632/fixture773 해시 재감사는 하지 않았다. 전체34·새ZIP·게임 실행·신규MP 검사0이며 포획/희생·특정 적 공격·승리를 재현하지 않았다.

제품 SHA-256은 `61c7bf147eee6401dd4421603b801413cb19c2efda458a9f6d8d9d673f574a6d`다. 최신 전체34/ZIP은645의644제품이므로 이번 제품의 GUI 근거로 확대하지 않는다. 상세 로그와 범위는 [보고서](../../cpppj/login-template-lifetime-20261101.json)에 있다.

## 남은 경계

후속 [단위668](cpp-datamanager-dump-20261101.md)에서 아래0044cf20 진단을 실제 Client에 연결하고 관련20개/실패0을 확인했다. 아래 문단은667 완료 시점의 경계다.

`004bf3e0`의 `0044cf20("Stripped Image:")`는 별도의 실제 DataManager 진단이다. level29 로그, 공유 formatter/FILE/flush/Chat과 검색 중 `current` 변경을 발생시킨다. 이번 strip 이미지 내용/할당 연결만으로 이 진단까지 완료했다고 보지 않는다. 다음 단위에서 실제 Client 진단 장치와 연결한다. 구형 명시 이미지/BYTE 입력은 독립 모듈 검사의 부분 어댑터로 남으며 실제10.78 Client는 지연 selector 경로를 사용한다.

다른 Template 소비자·전체 FILE/SEH ABI·손상/할당/부분I/O·Edit 도구와 시나리오·연속 미션/UI·호환성은 현행 인수인계 잔여를 따른다. 자연 완주와 전체 동일성은 사용자 확인 대상이다.

정적 근거는 `extracted/refined/originals/Netstorm.c`의 위 함수와 보호PE 정적 disassembly `extracted/cpp-login-template-lifetime-667-strip-static.txt`다. 새 Native 실행/fixture 생성은 없다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon /2026-10-23**이며 이번 디컴파일 산출물 변경은 없다.
