# Session의 자산 효과·실제 후처리 등록 (WB04-E590)

`225553df`의 `RawCampaignSession.cpp`1538..1610을 같은 실제 owner에
연결했다. typed VirtualCount967..985·PrepareConstruction2708..2720·
PrepareBuilding3654..3657도 이 구간의 실제 선택 하위다. Session·
RawCampaignAssetBindings.cpp/.h·CampaignDamageableRegularTables.inc는
기준선 뒤 변경0이다. cpppj/보호 파일 수정·디컴파일·게임/브라우저 실행은 없다.

## 실제 같은 World에 연결한 것

- 거주 구역은 실제 Dude/공급원 목록의 현재 count와 Fort 모드를 읽고
  hits.Dudes의 입구를 쓴다. 늦게 설치할 Dude 생성/작업기는 선택 시 요구한다.
- 신전은 E589의 동일 상승/이름/WORD/이탈/Descend 부모를 사용한다.
  억제는 missionPost가 아니라 현재 bridgeState.mission이다.
- 예약 피해는 현재 VT의 공통/일반/Base 범위, 실제 불워크 관문, 사제의
  TYPE+VT 조건, 신전 네 VT만 사용한다. 넓은 다른 가상98 분배기로 우회하지
  않는다. 공격자 DWORD 폭을 첫 조회 전에 검사하며 HP/현재 건설 Form과
  선행 효과를 보존한다. developerDamage와 decorationHit는 같은 함수다.
- 실제 GunQuery→GunTarget→Archer 훅, 현재 TYPE impactSound, Graph/
  청크 BYTE, Factory 타이머 세 참조, 현재 소유권/레벨을 연결한다.
  서비스의 postPop/regular/factory/gun 네 함수 표는 값으로 복사한다.
- 가상80은 현재 TYPE/VT·활성 SID 집합·제단/다이스 선택·공통 레벨을
  사용하고 예외에도 자기 재귀 표식을 반납한다. 보상은 같은 결과의 signed
  DWORD다. 건설 준비도 기존 실제 상태/동맹/패턴 배열 주소를 공유한다.
- 마지막에 실제 RawCampaignAssetBindings를 생성해 같은 공통 PostPop
  분배기에 asset/factory를 등록한다. Kernel 정리 뒤 등록을 해제한다.

## 원본 정적 자료와 검사

필수 부모인 CampaignDamageableRegularTables.inc7줄을 함께 옮겼다.
`damageable-regular-tables-x86.tsv`83행은 보호 PE의 +5c/+98 주소와
공통 여부를 읽은 **정적 자료**다. 공통13·일반Hit31·BaseHit45의 포함/
미포함을 전부 대조했으며 새 함수 실행 관측 수를 늘리지 않는다.
manifest는 이를 original-binary로 분류했다. C++ 직접 대응은
CampaignSessionTests의 CampaignTempleShrapnel_Native83RegularTables다.

신규14검사는 실제 자산/World/Pool/HP/목록/Host와 등록된 PostPop을 쓴다.
첫13건 중9건이 통과했다. 제품 결함1건은 Factory 참조 getter 함수를 값으로
읽어0이 된 것이며 고정 상자의 현재 value를 읽도록 수정했다. 검사 입력의
나머지3건은 반납 중 뒤이어 정리되는 사제 등록을 자산 등록으로 오인한 것,
TYPE 문자열 자료 구조/프레임 번호를 잘못 가정한 것,8칸 패턴의 보존 꼬리4칸을
누락한 것이다. 실제 계약과 기대값을 바로잡고 신규13건 전부 통과했으며
등록된 실제 자산 몸체/장부 깊이 복귀 검사1건을 더해 최종14건이 통과했다.

첫 전체 검사는22/26 각각5,625건을 통과했다. 이후 정적 검토에서
RawConstructionPlace/Command의 chargeBlocked가 bool임을 확인해
PrepareConstruction의 값형 관측을 숫자1 대신 실제 true/false로 맞췄다.
현재 전역을 읽는 실행 관문은 기존부터 Boolean으로 판정했지만 값형 계약도
보존해야 하므로 집중14건과 전체 두 버전을 다시 확인했다.

새 단위 표7줄을 완료했으므로 남은162단위/34,373줄이다. 7단계의 더 넓은
전투 통합 항목은 저장 미션/전체 Session 재대조가 남아 체크하지 않는다.
manifest806항목(original-binary2), 웹 JS130,120줄·검사760파일이다.
최종 전체는 Node22(v22.23.3)·Node26(v26.13.0) 각각5,625건 통과,
실패·취소·건너뜀0이다. 전체 glob과 `--test-concurrency=4`를 사용했다.
대응표1,160행·오류0, port845·partial36·todo126·제외153이다.
4-D 체크는 선행 표를 더해32/47이며 7단계 전체 통합 체크는0/6이다.
로그는 `extracted/wb-e590-focused.log`(관련55건)·`wb-e590-focused-final.log`
(최종 집중14건)·`wb-e590-node22.log`/`wb-e590-node26.log`(첫 전체 통과)·
`wb-e590-node22-final.log`/`wb-e590-node26-final.log`(bool 수정 뒤 최종)·
`wb-e590-port-map.log`다(Git 제외).
Node24는 사용자 지시에 따라 생략하며22/26 통과 시 호환을 가정한다.

## 남은 경계와 다음 시작점

현재 생성자1610의 독립 typed 자산 등록까지다. Native 장기 Storage/가상80
대여·전체 Runtime/Kernel6699 경로·첫 저장 객체 Pop·전체 Load/로그인과
실제 미션 L2/L3는 남았다. CampaignSessionTests의 실제6699→HP/연소와
불워크 전체 사격/예약 시험은 이 후속 통합 뒤 재대조한다. 의식/건설/업그레이드/
내용물/너겟/성장 아이스 등 늦은 모듈은 원문 선택 지점에서 명시 오류이며
임시 성공 하위를 붙이지 않는다. 공중3종 전용 새 검사도 추가하지 않았다.

다음은1611 `RawWorldRuntimeHooks`의 현재 신전/공격/Dude 후단을 전체
Runtime 수명에 빌려 주는 구간이다. 이후 이동·건물·전체 모듈 생성과
외곽 공개·같은 Template의 TypeNames→Objects/Contents→Load로 이어간다.
