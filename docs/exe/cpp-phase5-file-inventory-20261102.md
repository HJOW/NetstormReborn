# cpppj 단위733 — 5차 전체 자동 파일·읽기 전용 설치 요구 반영

2026-11-02 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이다. 이번은 사용자 변경 반영과 정적 경로 점검이며 **게임 제품은732 / 11.01 그대로**다. 전체 목표와 단계 순서는 유지한다.

## 새 요구와 현재 판정

사용자 `aaafbbc4`는5차에 **관리자 권한이 필요한 설치 위치에서도 일반 권한으로 정상 실행**하는 목적을 명시했다. 설정/진행만 옮기는 것으로 끝내지 않고 **게임이 자동 생성하는 모든 파일·디렉터리**를 `%USERPROFILE%\Saved Games\Netstorm\` 하위에 두고, 해당 파일을 읽는 코드도 함께 바꿔야 한다. `Saves`·`CustomMaps`, 원래/사용자 맵 병행 읽기, 신규 맵의 사용자 위치 저장, Edit 스크롤/무제한도 그대로 요구다.

사용자 `61067075`는 cpppj 대상을 **Windows10/11 x86-64(AMD64)**로 명확히 하고 **Windows11 ARM 지원도 고려**하도록 했다. 현재 실행 파일의 PE Machine은0x8664/AMD64다. ARM 빌드/실기는 미검증이다. 이후 파일 경로/OS 장치와 원본32비트 자료 형식·호스트 포인터 폭을 구별하고 신규 네이티브 의존성을 검토한다. ARM 지원 완료로 세지 않는다.

**5차 구현은 미착수다.** 이번 요청을 단계 순서 변경으로 해석하지 않으며 Startup·UI·호환성과 그 뒤의3/4차 잔여를 유지한다. 준비 목록을 만들었다는 이유로 현재 제품의 읽기 전용 설치 동작을 완료 처리하지 않는다.

## 확인한 생성물과 결합할 reader

[정적 목록 도구](../../tools/cpp_phase5_file_inventory.py)는 제품 소스/third_party의1709개 C/C++/헤더/inc를 점검해50개 파일 장치 후보를 기록했다. 주석·읽기 전용 열기·동적 모드도 포함하므로50개 모두가 writer라는 뜻은 아니다. 수동 분류12개 파일군과 실제 소스 줄/해시를 [목록](../../cpppj/phase5-file-inventory-20261102.json)에 고정했으며 재실행 결과가 일치했다.

| 파일군 | 현재 경로/장치 | 5차에서 함께 전환할 범위 |
| --- | --- | --- |
| 설정·진행 | `<game>/d/options.cfg` | 시작 읽기/초기 저장/변경 저장/다음 실행을`UserRoot/Saves`로 연결. 기존 설치 설정의 이관/우선순위 결정 |
| 옵션 교체·복구 | 같은 부모의`options.cfg.save-PID-N/next.cfg`, `previous.cfg` | 임시 폴더·실패 복구·지연 삭제도 UserRoot 내부의 같은 볼륨 |
| 상태 체크포인트 | `<game>/d/!state.dat` | 현재 reader/delete와 추후 전체 writer를 같은`Saves`에 연결. writer 완료로 세지 않음 |
| 전체화면 표식 | `fullscreenStateFile.dat` | 현재 reader/delete도 사용자 경로로 전환. 호환판이 생성하지 않는 파일을 새로 만들지는 않음 |
| Edit 맵·작업본 | `<game>-editor` | 신규맵`CustomMaps`, 작업 디렉터리도UserRoot. 원래 맵은 읽기 참조, 재적재/삭제/시나리오 짝 파일도 함께 처리 |
| 편집 장부·잠금 | `<game>-editor-state`의`catalog-v1.dat`, `catalog.lock`, `*.pending` | metadata도UserRoot. 설치 변경·이전 장부 identity·사용자 변경본/tombstone 보존·충돌 정책 검토 |
| GIF 스크린샷 | `GifOutName/GifOutSpec1`→기본 설치 위치 | 저장 위치와 이름 충돌 검사 reader를 같은 사용자 캡처 경로로 이동 |
| 연속 BMP | `GifSequenceName`→GameDirectory | 출력 부모·번호/이름·완료 안내·반납까지 사용자 캡처 정책에 연결 |
| 진단 덤프 | `<game>/squids.txt`, **CWD/procSumm.txt** | squids의텍스트→재읽기→바이너리 재쓰기, procSumm의CWD 의존을 모두UserRoot로 전환 |
| 명시 CLI/검사 출력 | screenshot/audio/UI report·실패BMP·이미지/설정 export | 자동 기본 생성물과 구별해 경로 정책 검토. 이 목록을 blanket 예외 허용으로 쓰지 않음 |
| 현재 읽기 전용 입력 | rootservers/기존 자산 | 자산은 설치 경로 읽기 전용, cache는분류. 비활성 온라인 검증/security writer를 되살리지 않음 |
| 추후 생성물 | MCP 기본 캡처·소리 포함 녹화·새cache/log/temp | 4~8차 구현에서 추가되는 writer/reader도 같은UserRoot 정책에 포함 |

`procSumm.txt`는현재작업폴더에직접생성된다. 설치폴더만바꿔도남는이경로를누락하지않아야한다. Edit의형제작업폴더와그형제-state도일반권한에서쓸수있다고가정하지않는다. stdout/stderr로쓰는현재trace와앱이이름을선택해생성하는파일로그는구별한다. third_party/libsmacker에서확인한fopen은rb읽기다.

## 5차 완료를 증명할 조건

- 새 테스트 설치 사본의 **실제 쓰기 거부 권한과 일반 권한 실행**에서 시작·옵션 저장·재실행이 성공해야 한다. 파일의 readonly 속성만 확인하는 것으로 대체하지 않는다.
- 자동 최종 파일뿐 아니라 임시/백업/잠금/장부/캐시/생성 디렉터리, 삭제·교체·reader도 같은UserRoot 정책을 써야 한다. 설치 경로·그 형제·임의CWD에 자동 파일이 생기지 않는지 확인한다.
- Unicode 홈/설치 경로, 다른CWD, 기존 설정 이관·두 위치 우선순위·두 설치의 공유 사용자 데이터, 예외/중단/권한 실패·1224 교체·잠금 정리·전체화면 종료 후 재실행을 점검한다.
- 원래 맵/사용자 맵 병행 읽기, 신규 CustomMaps 저장/재적재, 기존 사용자 변경본/삭제 상태 보존과 Edit 목록 스크롤/무제한을 확인한다.
- 자연 승리·포획/희생·특정 적 공격 재현, 공중3종 전용 및 신규MP 검사 제한은 그대로 유지한다. 원본과 다른 사용자 경로/일반권한 호환 이유는 구현 주석에 명시한다.

## 보호와 실행 범위

시작 보호2647개 중 **2646개는 불변**이다. AGENTS.md 한 파일은 진행 중 사용자가 커밋한`aaafbbc4`/`61067075`의 외부 변경이며 이 작업에서 수정하지 않았다. 현재 파일의 Git clean blob이61067075와 같음을 확인했다. 신규fixture0·현재fixture827개이며 이 작업의 보호파일 수정은0이다.

새 빌드/C++검사/게임/GUI/MP/통신 실행은0이다. 게임 EXE SHA-256은732의`d25df1aec10ae397355ec4f9e9842a1e2318e2890474712cadbe8721985821e6` 그대로다. 마지막제품검증25개/CLI와기존전체34/ZIP645·644근거를유지한다. 원시는`extracted/cpp-file-directory-733/`, 통합근거는[보고서](../../cpppj/phase5-requirements-20261102.json)다. 선행경로getter/탐색코드는이번에구현하지않았으며전체목표1~8차는진행중이다.
