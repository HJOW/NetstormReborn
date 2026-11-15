# dotnetpj 0.557 — 공유 상태와 장기 Graph·이웃·Owner

2026-11-15, HJOW-Athlon. cpppj 제품1313의 RawSurfaceStorage/World 공통 상태와 세 장기 실행기를 연결했다. M1은0/34, 새 경로는 플레이 불가다. 실제 개발 HostSession의 별도 풀은 이 단계에서 전환하지 않았다.

## 구현

Storage가 PostPop/삭제/표면 로딩/건물 공간의 같은 상태 본체를 보유하고 World가 직접 빌린다. PostPop의 세 통계표·정적 공급원/공장·GameAux 비용, 삭제의 손실 X/Y, 공장/Form에 빌릴 깊이 DWORD를 같은 물리에 연결했다. UIntBox의 값 속성과 내부 ref가 같은 backing DWORD를 사용한다. float 좌표의 NaN/부호0 비트도 그대로 보존한다.

일반 fresh는 상태 객체·목록 객체·깊이 owner를 교체하지 않고 원문 새 상태 대입의 값과 짧은 바인딩 반납을 수행한다. 준비 대여는 현재 값을 초기화하지 않는다. Template 직렬화 전역의 실제 로딩 깊이를 첫 대여 전에 결합하며 같은 owner 재결합은 허용하고 다른 owner/대여 중 최초 결합은 거부한다. World/표면의 깊이 조회는 이 현재 DWORD를 직접 읽는다. FortSerializationGlobals의 scope/저장 mask 등 나머지 현재 헤더 필드는 후속 저장/조립 대상으로 남겼고 파일 전체 기준을 올리지 않았다.

Graph·이웃·Owner는 Storage당 한 번 구성한다. 같은 Pool/Hash/spot·기록/stack·현재 TYPE·장부·모드·진단/시계/로그를 사용하며 World 반납/다음 대여에서 재할당하지 않는다. Graph는 최초 Storage 코드 표에서 독립 사본을 만들고 frames 교체를 초기 Graph 코드 변경으로 해석하지 않는다. 조회 자체는 대여·초기화·게임 효과를 만들지 않는다. 자산 표가 없는 기존 얇은 독립 검사 경로는 종전 지역 실행기 계약을 유지한다.

공장·표시·Pop/Unpop·프레임·연결/표면의 장기 실행기, 실제 Native 장치 설치/Runtime pin, 최초 로딩 비용/자산 소유·미이식 파생과 상위 피해/해방/삭제/배치·현재 Host 전환은 잔여다. 이 단위는 큰 Storage/World 전체 완료를 뜻하지 않는다.

## 검증

신규6검사는 스칼라/정적 목록/통계표와 ref 깊이 alias, 일반 fresh에서 상태 정체성과 이전 Player 목록 장치의 무호출 반납, 두 준비 World와 다음 일반 World의 장기 상태/세 실행기 재사용, Template 깊이의 현재 조회/대여 관문, 실행기 조회의 불변과 현재 Graph 활성, 최초 Graph 코드 사본을 검증한다. 기존 원본 기대값은 변경하지 않았다.

첫 빌드는 GraphRecovery 네임스페이스 누락으로3개 진단이 발생했고 같은 using 교정으로 해결했다. 이후 기존 관련388/388, 신규 포함394/394, 계약 포함 최종 **Core400/400**, **Client120/120** 통과다. Client는 사제/Player/요새 적재·세션 덤프와 CampaignTemplate을 확인했다. TRX는 `extracted/dotnet-storage-state-0557-initial2/`·`-new/`·`-final/`·`-client/`다. Host Release 경고0·오류0, 전체 회귀는0.546 기록을 유지한다.

L2는 동일 SHA cpp34 기준선을 재사용하고 dn일반31개를 새로 실행했다. 모두 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.556 대비 상태 변화는 없고 파일 전체 동일12·frame행만 차이19경로다. 전체 일치0/34·데모dn3 미연결이다. 자료 `extracted/dotnet-l2-0557/`에 보관했고 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP923행(L1 431·port492), cpppj 대응415단위·fixture312개다. 부분 대응6행만 추가하고 기존 행/해시/순서와 floor/reflected는 유지했다. R4 누락138단위/13,913줄·R6 누락382단위/45,090줄은 이전 단위 그대로다. 다음은 나머지 장기 생성/표시/공간/표면 조립과 실제 호출 수명, 현재 호스트 전환을 이어간다.
