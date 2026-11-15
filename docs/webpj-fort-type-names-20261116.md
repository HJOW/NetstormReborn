# 캠페인 공용 TypeNames와 변환표 수명 (WB04-E573)

`225553df`의 `o/RawFortTypeNamesLoad.cpp/.h`112줄과
`client/FortTypeConversionStorage.cpp/.h`63줄을 이식했다. 두 파일은 기존 대응표에서
Editor 전용으로 제외되어 있었으나 실제 캠페인에서도 사용한다. 생성자 구현 준비 중 발견한
이 선행 분류를 교정했다. 전체 Session 생성자/Load는 아직 미구현이다.

## 제외 판정 교정

기준선의 실제 호출 사슬은 다음과 같다.

1. RawCampaignSession.cpp3484의 `data.campaignObjects`가 캠페인 저장 객체 콜백을 설치한다.
2. 같은 파일3211의 `ReadFortObjectStreams`가 실제 Objects stream을 읽고3218에서
   `PrepareFortTypeNames`를 호출한다. 내용물은 같은 현재 변환 토큰을 사용한다.
3. 3345의 `PrepareFortTypeNames`가 동일 CampaignTemplateStorage의 TypeNames stream과
   FortTypeConversionStorage를 실제 로더에 연결한다. 상위 scope에서만 예외 cleanup을 부른다.
4. 변환표 owner는445의 Impl 멤버 생성에서 조건 없이 생성/공유된다. 값0이나 별도 지역 표로
   대체하면 실제 캠페인 저장 타입 변환과 할당 수명이 사라진다.

따라서 규칙 `edit-existing-fort`에서 두 이름을 빼고 `campaign-type-names` port 규칙으로
분리했다. E275/E499 당시의 제외 기록은 이 판정으로 교정한다. Deck/Subscriber/지형 회복의
Editor 분류는 이번 근거만으로 변경하지 않는다. `port-map --write` 전후 총1,160행은 같으며
분류를 바꾼 것은 이 두 행뿐이다. 새 대상은1,006개, 제외154개, 이식 대상 분류955·adapt51이다.
두 행의 이식을 끝내 port841·partial33·todo132, session/oracle0이다. 총 대상 줄은146,232,
port111,642줄이다. 분류 복원이므로 남은165단위 수 자체는 줄지 않는다.

대상 소스의 기준선 이후 변경0을 확인했다. cpppj·보호 원본·공통 인계·dotnetpj는 수정하지
않았다. 게임/브라우저/원본 관측 도구도 실행하지 않았다. 현재 자료만 읽어 검증했다.

## 이식과 실제 입력

- 변환표 owner는 같은 현재DWORD 상자·발급 토큰·실제255BYTE 할당을 별도로 보관한다.
  할당은 current에 대입하거나 본문 zero를 앞당기지 않는다. 이전 포인터가 밀려나도 그 블록은
  명시 release까지 남는다. release(0)는 허용하고 현재값 변경은 원문 부모에 맡긴다.
  잘못된 크기·미발급/반납 토큰·토큰 소진·할당 중단은 기존 live 블록과 발급 접두를 보존한다.
- 웹의 새 Uint8Array는0이며 C++의 미초기화 힙을 읽을 수 없다. 기존 CurrentAllocationBytes
  적응 정책을 재사용하고 추가 zero는 하지 않는다. 시험은 비영 입력을 주입해 실제 로더의
  63DWORD→WORD→BYTE 쓰기와 부분 중단을 구분한다. 토큰은 실제 호스트 주소가 아니다.
- 로더는 깊이 증가→현재 manager/Select→같은 stream16/data→count 캡처→기존 conversion
  진단→할당/현재 토큰 대입/정확255BYTE zero→현재 hash 주소 캡처→signedchar hash/현재
  TYPE 수 반복→첫 저장 hash 일치 칸→그 순간 현재 변환표 쓰기→정상 깊이 감소를 유지한다.
  TYPE256 이상은 낮은BYTE로 감긴다. 빈 stream은 이전 블록을 해제하지 않고 current만0으로 쓴다.
- 단독 getter와 CampaignTemplateStorage.accessSection의 실제 동기 대여를 모두 지원한다.
  중간 예외에서는 깊이/pos/앞선 zero·매핑을 유지하고 자동 rollback/cleanup을 넣지 않는다.
  별도 unwind는 현재 깊이를 한 번 감소시킨다. 상위 Session은 같은 방식으로 예외 뒤 호출해야 한다.

## 검증

신규16건을 추가했다. 원본 fixture `fort-type-names-load-x86.tsv`는332입력×두x87제어의
664본문이며 중단38건에는 별도 cleanup 기록이 있다. getter 경로와 동기access 경로에서
각각 같은 현재 전역·원문 전체 raw·효과 순서·cleanup을 대조했다. 재생은1,328본문/76cleanup이며
새 원본 관측을 만들거나 관측 수를 두 배로 보고하지 않는다. fixture는665행·4,788,248바이트,
정규화SHA-256은 `4c824ec764b85b73ef1d2387c7e6043611b767e983f66abc3f1a98da2d0ef86c`다.
fixture/실제 CPP 근거 커밋은 `a9af29da7`이다. 새 manifest는803항목이다.

owner6검사는 CPP 공개 계약과 실제 마지막 소비자/GC 수명을 확인한다. 실제 Template 통합6건은
동일 manager의slot1/8·현재 노드 교체·같은 물리 fields/data·중첩/구조 변경 guard·callback의
end/count/hash 변경·부분 실패와 별도 unwind·빈 섹션의 과거 할당 보존을 확인한다. 마지막 검사는
같은 현재 표를 실제 RawFortContentsLoad가 소비해 저장 타입1을 현재2로 생성 요청하며, 명시
해제 뒤 같은 포인터 재조회가 실패함을 확인한다. 최종 create는 인자 기록 경계이며 전체
Session/Factory 생성이나 실제 게임 플레이를 대신하지 않는다.

첫 기본 집중3건과 확장10건은 모두 통과했다. 실제 Template 통합을 더한 첫15건 중5건은 실패했다.
4건은 서로 다른 대여의 Uint8Array 뷰 객체 자체를 비교한 시험 오류였다. 같은 실제 물리의
buffer/byteOffset/byteLength를 비교하도록 고쳤고 단일 대여 중 뷰 정체성 검사는 유지했다.
빈 섹션1건은 CPP 입력의 `fort.sections.resize(35)`를 생략한 시험 구성 때문이었다. 실제35칸
입력을 제공한 뒤 기존 제품 코드를 바꾸지 않고 모두 통과했다. 재생/로더 알고리즘 교정은 없었다.

최종 집중 검사는 Node26 신규16건과 Node22의 신규16건/대응표7건 총23건이 모두 통과했다.
owner의 줄/분기/함수는100%, 로더는 줄100%/분기97.83%/함수100%다. 로그는
`extracted/wb-e573-focus6-node26.log`·`extracted/wb-e573-focus5-node22.log`다.
전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,434건 통과, 실패·취소·건너뜀0으로 끝났다.
최종 전체 로그는 `extracted/wb-e573-node22.log`·
`extracted/wb-e573-node26.log`, 대응표 결과는 `extracted/wb-e573-port-map-final.log`다(모두 Git 제외).

## 남은 통합과 다음 시작점

생성자/소유 상태를 먼저 실제 장기 owner에 연결하고, 같은 Template의
TypeNames→Objects→Contents를 RawCampaignSession1에 설치한다. 생성자 본문만이 아니라
Impl 멤버 선언의 GameAux 별칭(FreeTechnologyType·TempleProductType·BombFlareType·
Fence/aviary 역할 등)과 기본 shared owner도 원문 순서로 구성해야 한다. E572는 생성자 본문의
입력 보완이며 모든 Impl 멤버나 전체 Session 완료를 뜻하지 않는다.
저장 Fort의 전체 후처리·로컬 로그인·L2/미션 화면·실제 플레이는 여전히 잔여다.
