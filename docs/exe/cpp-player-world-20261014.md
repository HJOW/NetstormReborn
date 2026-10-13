# 실제 Player152 생성·후처리·현재 Technology 조회

## 범위와 순서

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 단위13의 전체1066 통과 기준 뒤 단위14 신규 파일이며 보호 두 PE/원본/AGENTS/dotnetpj를 변경하지 않는다. CMake·공유 `SquidPostPop` 분배·공유 빌드·상세 LEFT·커밋은 root가 맡는다.

| 몸체 | 패치10.78 | CD10.72 | 실제 효과 |
| --- | --- | --- | --- |
| Player 생성 wrapper | `00490250` | `00407400` | PlayerKnown 진단→현재 playerType 단일검색→Factory(flags2)→가상Owner→직접frame0→가상Pop(x=owner+10,y=1/10,flags0) |
| Player PostPop | `004905d0` | `00407cc0` | 현재owner의 Player Technology DWORD에SID 기록→최초local/비Fort일 때 new40/Regular(event1,payload5.0f)→special 첫frame1→같은 비가상 공통Post |
| 현재 Technology | `0048fc60` | `00406d90` | PlayerKnown0이면0 자연 반환, 그렇지 않으면 현재battleOwner의DWORD 전체 조회 |
| 명시owner Technology | `0048fc90` | `00406dc0` | PlayerKnown 진단 후 명시owner의DWORD 전체 조회 |

기존 실제152 생성자 `00491660/CD00407950`의 Recipe를 재사용한다. 새 생성자 복원 개수에 다시 합산하지 않는다. 실제 VT는 `0050ee08/CD005002a8`이며 가상+20은 위 Player 전용 PostPop이다. 생성/특수frame 쓰기는 패치DWORD+36, CD BYTE+34이다. CD 디컴파일의 `unaff_ESI` Pop 인자는 PE 명령에서 실제(x=owner+10,y=1/10)로 확인했으며 잘못된 매개변수 순서를 구현하지 않는다.

## 현재 저장소와 초기 예약

`RawPlayerWorld`는 Factory/Owner/Pop/공통Post/ProcessHost와 같은 Pool을 사용한다. Player9개 기록은 패치9×0xb4/CD9×0xac BYTE를 직접 참조하고 Technology SID는 각각+0x9c/+0x94 DWORD다. `State()`와 `Records()`는 같은 현재 전역/기록을 상위 실제 캠페인 부모에 제공하는 읽기 연결이며 vector 사본을 만들지 않는다.

현재battleOwner(`00540c70/CD0050f6c8`)와 Template owner(`00540cac/CD0050f6d4`)를 합치지 않는다. 이 네 몸체에서 Template owner를 쓰는 효과는0이다. owner0..8의 현재 물리 기록 범위를 확인하며 범위 밖 원본 메모리 실행을 완료로 주장하지 않는다.

최초 Regular는 **현재 game time 그대로**, event1, payload**5.0f**, count0다. payload를 최초5초 지연으로 바꾸지 않는다. 힙 예약 실패이면 Regular 생성/부착이 없지만 SID 기록·special frame·공통Post는 계속 수행한다. 실제 이후 Player 이벤트 처리기/경제·UI는 미복원이다.

보호 `player.type`의 두 blank 클러스터/실제 SHP 블록, foot1×1, zOrder−20, HP0을 사용한다. 프레임/HP/Owner/Pop의 기본값 대체로 실제 하위효과를 닫지 않는다. 일반 finder는 기존 `RawBridgeConnect::FindTypeAt`를 읽기 재사용한다.

## 독립 원본 및 보존

독립 고유 입력은 **1680=정상1452/assert228**, 각판본840=정상726/assert114이다. wrapper/Create192·직접Post576·두getter72를 판본마다 실행하며 owner0/1/8·두owner·PlayerKnown·Fort/special·힙성공/실패·flags·유한currenttime을 교차한다. 두x87 반복3360 반환을 추가 고유수에 더하지 않는다.

최초 관찰은 실제 Factory/152ctor/Owner/Pop/표시/공통Post/Regularctor를 실행하고 FormAttach만 명시 하위경계였다. 최초 tool/spec/hcpp/report/1680fixture 전체6파일과 manifest를 `cpppj/hist/player-world-f6101`에 보존했다.

같은1680 입력의 후속 관찰은 **실제 BaseProcess/Kernel/FormFactory/부모부착/FormPop**까지 실행한다. 1600개 비부착 행의 기존 전체 출력이 불변이며 부착80행에서는 기존 Player/공간/장부/시각/event/payload/count/진단이 불변이고 부모WORD6만 실제 Form6으로 바뀐다. 현재 fixture는 Form SID/전체raw·Kernel PID·패치rotation을 추가 보존한다. 기존 관찰/재실행·smoke·중단된 미완료 실행을 신규 고유수에 합산하지 않는다.

힙 new40과 assert 보고가 명시 외부경계다. actual attach를 빈 콜백으로 공급하지 않는다. 불변 PE의 반복(addr,size) 범위판정만 캐시해 실제Kernel 순회의 검사를 빠르게 했고 처음 범위검사·CRT/assert·쓰기보호를 유지한다.

원본 자료는 `decomp_player_world_oracle.py`/`player-world.json`/`player-world-x86.tsv`다. 최초 전체 출처/동일입력/1600행/80행/Records·State 정확 역변환은 `analyze_player_world_attach_lineage.py`의 별도 successor로 연결한다. **최종 재관찰1680/전체출처SHA/정확역변환 감사가 통과**했으며 현재자료는동결했다. fixture SHA는 `c53c5f89f6734cbc89535199a69d0348f5f051192ce9e7c339c55565caa5c53b`다.

## C++ 검사와 남은 경계

새 `PlayerWorldTests.cpp`는 양판본 원본 전수2검사 및 실제 `RawSurfaceWorld.Core`의 같은 Factory/Pool/Hash/Player9/ProcessHost/Kernel 소비자1검사를 준비했다. 현재 **공유 빌드/전수 실행 전**이다. fixture 매크로는 `NETSTORM_PLAYER_WORLD_FIXTURE`다. 모듈 해제 전에 실제 Kernel/Form 프로세스를 정리한다.

공유 `SquidPostPop`의 Player 분배는 root가 단위13 전체 파일을 보존하고 별도 정확역변환 successor로 추가한다. Native 보고서의 당시 source SHA를 현재 shared source SHA로 덮어쓰지 않는다.

실제 미션 부모는 `004b5300/CD004322e0`에서 이 Player152를 먼저 생성한 뒤 지형과 저장172개를 로드한다. 173은 입력 대상 수이며 지형/edgeFarm/Form/Regular가 포함된 최종 Pool수가 아니다. 이번 네몸체/Host 성공을 실제 캠페인 전체 설치·PacketReady·Guide·AI·SP/myTech·게임 플레이 완료로 확대하지 않는다. 실제 부모와172개 같은 저장소 설치는 별도 단위15다.
