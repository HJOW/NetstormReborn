# 소리 프로세스·이름 표와 보호막 수명 (WB04-E571)

`225553df`의 `client/SoundProcess.cpp/.h`361줄을 이식했다. Session 생성자1208이 이 시스템을
무조건 만들고 사제 보호막에 이름 조회와 Sound47 부착을 설치하므로8단계에서4-D C로 앞당겼다.
같은 입력이 요구하는 `Sound.cpp/.h`824줄 중 **SoundList 공개 API 전체**도 이식했다.
SoundProcess·Sound·RawGameAuxState의 기준선 뒤 소스 변경은0이다. cpppj·보호 원본·공통 인계와
dotnetpj는 수정하지 않았다. 새 게임/브라우저/오디오 장치나 원본 관측 도구는 실행하지 않았다.

## 이식 범위

- `www/core/sound-process.js`: 실제 SquidProcessHost/Kernel의 Form47 부착·실행·Form 삭제,
  SoundProcessState의 현재 frame/typeFrames256·공유TYPE getter·현재checking을 옮겼다.
  여섯 필수 소리/시계 효과를 구성 때 확인하고 C++ 값 인자처럼 함수 묶음을 복사한다.
  소리 선택→벽시계→flags/중복 프레임→진단→현재 좌표→재생 순서를 보존한다.
- 반복 위치 재생은 현재 소리가 있으면 같은 프레임에도 갱신한다. 위치 한 번은 priority8을
  원문대로 반복 인자 자리에 넘기고 결과를 버린다. 전역 재생은 결과0이어도 프레임을 기록한다.
  한 번 소리는 Form이 끝나도 정지하지 않고 반복 소리는 stop 뒤 현재 항목을 재조회한다.
  재생 후에는 현재 부모TYPE·frame을 다시 읽는다. double 벽시각을 float로 먼저 자르지 않는다.
- 보호막 소리·SoundPlayer 연결·보호막 안내·사제 낙하·건설 소리의 다섯 어댑터를 옮겼다.
  같은 SID 풀 정체성과 원문 인자 순서를 유지하고 signed volume/pan/limit는32비트로 해석한다.
  SoundPlayer는 공개 메서드 계약만 명시하며 실제 재생기 구현이 있다고 주장하지 않는다.
- `www/audio/sound-list.js`: 초기화/조회/null/표 안 포인터/이름/머리 필드/복제 API를 옮겼다.
  숫자 handle은 원본 관측과 같은0x15000000+오프셋이며 호스트 주소가 아니다. 원문32768바이트
  표·28바이트 머리·ASCII만 접는 비교·한도7fb0·대체 이름 `nonexistant.wav`·이름 포인터에서
  빼는1f·쓰지 않는+10 음량·복제의 이름→root→next→buffer→앞next→빈 위치 순서를 보존한다.
  API 이름은 인코딩된 byte 문자열이다. NUL/빈 이름과 표 초과는 cpppj처럼 거부하고, JS에서
  바이트로 표현할 수 없는 문자를 암묵 절단하지 않는다. CD/10.37 전용 보고/NaN 분기는 제외한다.
- GameAuxState에 같은 현재0054112c DWORD를 추가했다. PE 초기값47과TYPE 이름을 대조했으며
  일반 월드 reset/Session 교체로 되돌리지 않는다. 다른 미이식 장기 칸은 기존③후속 판정을 유지한다.

## 검증과 교정

`cpppj/tests/SoundProcessTests.cpp`의10.78 재생·시작 관문·오류·실제 보호막 연결을 옮겼다.
원본 기록은 `cpppj/tests/fixtures/soundprocess-x86.tsv`의총18,381관측 가운데 **originals6,127행**만
쓴다. 실행5,808·통지144·생성168·이름 연산7묶음(조회124회)이다. 실제 Host/Form/Kernel에서
사건과 살아 있는 필드·double 비트·TYPE 프레임을 대조하고, 이름 표는 반환/진단/빈 위치와
**표 전체32768바이트 Adler-32**를 저장된 원본 관측과 비교한다. 소멸한 프로세스는 읽지 않는다.

fixture의 정규화SHA-256은 `1687245d52763e0e26111e95c3984e9977a1504a882bd825e8f828f1b55b9203`,
파일18,386행·4,177,929바이트다. fixture 변경 커밋은 `99d66b0a6`이며 manifest가 기록한 실제
cpppjBasis는 SoundProcess.cpp/.h `d3767242f`, Sound.cpp/.h `d445a5d04`다. 기준선 표기는
이식 원본이며 이 과거 관측 생성 근거를 덮어쓰지 않는다. 새 manifest는802항목이다.

신규14검사는 두 파일에 있다. 주변 GameAux·ConstructionProcess·PriestShield까지 포함한
최종 집중36건이 통과했다(`extracted/wb-e571-focus3.log`). 두 새 모듈의 줄/분기/함수 coverage는
모두100%다. 실제 장치나 전체 캠페인 성공을 커버리지로 대신하지 않는다.

첫 집중13건 중10건이 통과했고 **원본6,127행은 처음부터 전부 일치**했다. 나머지3건은 시험이
`SidPool.slot()` 읽기 사본에 입력WORD/dead/TYPE를 쓴 탓이었다. 시험 쓰기를 `allocated()`로
교정했다. 함께 검토하며 SoundProcess의 원문 const span에 대응하는 부모 읽기를 물리 대여로
고쳤다. 이후 추가 검사는 벽시계 앞 소리 캡처·벽시계 뒤 현재TYPE·진단 뒤 현재 좌표와
캡처한 played 보존을 직접 바꿔 관찰하므로 사본으로 되돌리면 실패한다.

두 번째 집중20건 중19건이 통과했다. 새 장기TYPE를 포함해 기존 GameAux TYPE 이름 개수도
34에서35로 갱신한 뒤 최종36건이 통과했다. PE 필드 개수는58이고 장식4칸과 별도다.
보호 원본SHA-256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`을 확인해
0054112c의 실제47을 읽었다. 현재 환경에서는 PE 대조를 모두 수행했다.

첫 전체 검사에서 manifest 형식 검사가 두 버전 모두 실패했다. 같은 보호막 통합 검사를
소리 TSV의 직접 소비자로 적었으나 그 검사는 실제 모듈/자산의 관계 검사이고 TSV를 직접 읽지 않는다.
직접 읽는 `sound-process.test.mjs`만 소비자로 남겼다. 이 오류가 확인된 첫 실행은 중단했고
manifest를 고친 최종 목록으로 두 전체 검사를 다시 실행해 모두 통과했다. 중단된 실행을 전체 통과로 세지 않는다.
교정한 manifest의 형식/참조·정규화SHA·소비자 누락 감사3건은 별도 재검사에서도 모두 통과했다
(`extracted/wb-e571-manifest-final.log`).

최종 전체 검사는 Node v22.23.3·v26.13.0 각각 **5,414건 통과·실패/취소/건너뜀0**이다.
최종 로그는 `extracted/wb-e571-node22-final.log`·
`extracted/wb-e571-node26-final.log`다. 대응표1,160행 오류0, port839·partial33·todo132·제외156,
session/oracle0이다(`extracted/wb-e571-port-map.log`, 모두 Git 제외).

## 실제 연결과 남은 작업

보호막 시험은 E213의 실제 자산/공장/소유/Pop/두 Regular/프레임/표시/장부/삭제에 같은
SoundList·SoundProcessSystem을 더했다. 확보→이름 조회→세 프로세스→Kernel의 위치 반복,
같은/다음 프레임의 갱신→사제 Unpop→보호막 자기 삭제→stop/isPlaying→SID/장부/프로세스
복구가 이어진다. 소리 확보 실패에서는 두 Regular만 생기고 소리 효과 없이 반납된다.
재생 장치만 명시 기록 경계다. 브라우저 청취/실제 Session 프레임을 실행했다고 세지 않는다.

SoundProcess는port이고 Sound는**partial**을 유지한다. SoundPlayer의 위치 음향/중복 재생·실제
브라우저 오디오 버퍼·음량/음악과 장치 준비는8단계에서 이식/적응한다. 현재 모듈은 실제 장치
호출을 성공 함수로 채우지 않는다. Session의 소리 효과/벽시계는 공급자가 실제로 설치해야 한다.
다음은4-D D의 RawCampaignSession1 구성·소유 상태·Load다. 같은 장기 owner/실제 자료부터
구성하고 저장 Fort 적재와 L2를 이어 붙인다. 전체 미션 플레이는 아직 불가능하며 session/oracle0이다.
