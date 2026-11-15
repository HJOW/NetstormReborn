# dotnetpj 0.562 — 현재 자산 값 사본과 최초 로딩 비용 owner

2026-11-15, HJOW-Athlon. cpppj 제품1313의 RawSurfaceStorage 생성자 vector 값 소유와 Source 최초 비용 연결을 옮겼다. M1은0/34이며 새 경로는 플레이할 수 없다.

## 구현

기존 C# 저장소는 목록만 복사하고 mutable TYPE/셰이프/헤더 객체를 입력 자산과 공유했다. 원문 vector와 같이 TYPE 및 중첩 usage 배열, 프레임 코드, 셰이프의 loaded/count/물리 헤더 목록을 독립 소유하도록 고쳤다. 표시 헤더의12필드를 계산 없이 값 복사하며 NaN payload/부호0를 보존한다. 프레임 개수와 물리 헤더 목록을 서로 추정하거나 잘라 맞추지 않는다.

명시 최초 자산 정체성은 현재 mutable 사본과 분리해 보관한다. Core의 `IRawSurfaceAssetSource`는 실제 자산 owner의 최초 TYPE 표를 읽는 경계이며 전체 GameAssets 구현을 대신한다고 주장하지 않는다. Source 조회/저장소 생성/공장 getter에서 표를 미리 읽지 않고 첫 장기 PostPop 구성에서만 실제 Source.LoadedTypes를 빌린다. 현재 TYPE 비용 변경과 최초 로딩 비용을 구분하고 누락/잘못된 표는 실제 선택 위치에서 실패하며 실패한 실행기를 공개하지 않는다. Source가 없는 typed 경로는 기존 실행기 구성 시점 비용 사본 계약을 유지한다. Source 존재로 Native 정책을 추정하지 않는다.

일반/준비 World 재대여는 현재 자산 객체/값과 최초 Source 정체성을 교체하거나 입력 자산으로 덮어쓰지 않는다. 실제 Client GameAssets의 추가 자산뷰(배치/도형 등)와 Native 기본 장치/파생 설치·Host 전환은 잔여다. 이 단위는 상위 자산 계층 또는 NativeStorage 전체 조립 완료가 아니다.

## 검증

기존 관련 **408/408**, 신규7개 포함415/415, 계약 포함 최종 **Core421/421·Client120/120** 통과다. 신규 검사는 입력/현재 자산의 양방향 중첩 별도 소유, 전체 표시 헤더/NaN·부호0 복사, 명시 Source 정체성/지연 조회와 Native 정책 독립, 최초 비용17/현재 비용999의 실제 PostPop 분리와 두 번 누적34, Source 없는 typed 비용 사본33 유지, 잘못된 표의 선택 실패/미공개와 실제 표 교정 뒤 구성, 세 World의 자산 정체성/현재 값/최초 Source 보존을 확인했다. 기존 fixture 기대값은 변경하지 않았다.

TRX는 `extracted/dotnet-storage-assets-0562-initial/`·`-new/`·`-final/`·`-client/`다. Client 검사에서 Host Release까지 빌드했고 경고/오류는 없었다. 전체 회귀 기준은0.546을 유지한다.

L2는 cpp34 SHA 확인 재사용·dn일반31 신규 실행 종료0·수집 오류0·원본1,394파일 전후 동일이다. 0.561 대비 상태 변화는 없고 전체 파일 동일24·frame행만 차이7경로다. **전체 일치0/34·데모dn3 미연결**이다. 자료 `extracted/dotnet-l2-0562/`. 사용자 제외 플레이/MP 실행은 없다.

PORT_MAP945행(L1433·port512), cpppj 대응416단위·fixture313개다. 부분 대응3행과 재대조한 표시 헤더 소비자의 기준만 반영했다. 큰 Storage/World 전체 기준과 기존 fixture 해시/순서, floor/reflected는 유지했다. R4 누락137단위/13,873줄·R6 누락382단위/45,090줄은 이전 그대로다. 다음은 실제 Native 장치/파생과 Client 자산뷰를 이어간다.
