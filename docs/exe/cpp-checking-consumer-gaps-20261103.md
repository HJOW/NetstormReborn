# 미복원 checking 부모의 함수별 조건·현재 예외 한계

2026-11-03 단위779의 정적 분류를 보관한다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이다. 이 문서의51함수는779 당시 잔여이며 이후 완료 여부는 [현재 인계](../../LEFT_JOBS.md)와 [779 기록](cpp-checking-consumers-20261103.md)을 따른다. 주소 언급과 현재 gate 연결을 모든 진단 부모의 복원 완료로 합산하지 않는다.

모든 미복원/부분/MP 항목을 함수별로 남긴다. `현재 주소 언급`은 구현·연결 증거가 아니며 복원 후보를 찾기 위한 색인이다. 검사 hook이 이미 있어도 원문 checking 관문·호출 부모·복귀 뒤 효과를 확인하지 않았다면 완료로 표시하지 않는다. 기존 throw를 무조건 제거하거나 성공을 합성하지 않는다.

## FUN_0041cdb0 @ 0041cdb0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    fVar1 = fVar1 + (float10)_DAT_005022d0;
  }
  if ((float10)_DAT_005022d0 <= fVar1) {
    if ((DAT_005e4794 != 0) && ((float10)_DAT_005022cc <= fVar1)) {
      FUN_004e0620("angle < 361.0f","\\Ns\\O\\Boardcoord.cpp",0xb3);
      return (float10)_DAT_005022a0;
    }
    fVar1 = (float10)_DAT_005022a0;
  }
  return fVar1;
```

원문 assertion 경계:
- `"angle < 361.0f","\\Ns\\O\\Boardcoord.cpp",0xb3)`

## FUN_0041db70 @ 0041db70

**783 완료:** 현재 checking 뷰와 선택 진단을 두 실제 건설 소비자에 연결했다. 아래 미복원 서술은779 당시 상태다. [783 기록](cpp-board-checking-parent-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawConstructionNotice.h, cpppj\src\o\RawBoardApproach.h, cpppj\src\o\RawBoardTarget.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
            local_24 = local_24 + _DAT_005012d0;
          }
          else {
            if (DAT_005e4794 != 0) {
              FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Boardcoord.cpp",0x235);
            }
            local_28 = *(float *)this;
            local_24 = *(float *)((int)this + 4);
          }
        }
```

원문 assertion 경계:
- `&DAT_0050161c,"\\Ns\\O\\Boardcoord.cpp",0x235)`

## FUN_00421f90 @ 00421f90

**793 완료:** 같은 현재 checking/TYPE 진단 부모를 실제 낙하 예약과 Storage 뷰에 연결했다. 원본200입력/400회와 재관측400회 일치·집중7개 통과이며 기존 자식은 유지한다. 아래 미복원 서술은779 당시 상태다. [793 기록](cpp-bridge-fall-parent-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBridgeLifecycle.h, cpppj\src\o\RawIslandLifecycle.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int iVar2;

  iVar2 = param_1 * 0x32 + DAT_005c8464;
  if (DAT_005e4794 != 0) {
    uVar1 = FUN_004ac200(iVar2);
    if ((uVar1 & 4) == 0) {
      FUN_004e0620("s.getGenus() & gBRIDGE","\\Ns\\O\\Bridge.cpp",0x323);
    }
  }
  FUN_00421530(iVar2,param_2,param_3);
```

원문 assertion 경계:
- `"s.getGenus() & gBRIDGE","\\Ns\\O\\Bridge.cpp",0x323)`

## FUN_004227e0 @ 004227e0

**795 완료:** 현재 최소 수명 checking/진단 복귀와 같은 Storage 뷰를 연결했다. 원본272입력/544회와 재관측544회 일치·집중5개(새2개) 통과다. 아래 미복원 서술은779 당시 상태이며 [795 기록](cpp-bridge-decay-checking-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\Bridge.h, cpppj\src\o\RawBridgeDecayScan.h, cpppj\src\o\RawBridgeDecay.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_005453c8);
    if (uVar7 == 0x7fffffff) goto LAB_00422983;
    if (DAT_005e4794 == 0) {
LAB_004229b8:
      if (7 < uVar7) {
        uVar7 = 7;
      }
    }
    else if (7 < uVar7) {
```

원문 assertion 경계:
- `"0 <= lowestTimeLeft && lowestTimeLeft <= brMAX_TIME_LEFT","\\Ns\\O\\Bridge.cpp", 0x268)`

## FUN_00423d50 @ 00423d50

**798 완료:** 797의 현재 SID/진단 앞단을 실제 소비자와 같은 Client checking/상세 SID owner에 연결했다. 집중13개(새2개) 통과·기존 양판본 몸체 fixture를 유지했다. 아래 미복원 설명은779 당시이며 [798 기록](cpp-burning-victim-live-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBurningFrame.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    _free(*(void **)((int)this + 0x78));
  }
  FUN_00487ab0((int)this + 0x24);
  if (DAT_005e4794 != 0) {
    iVar3 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
    if (iVar3 == 0) {
      FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x6f);
    }
  }
  iVar3 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
```

원문 assertion 경계:
- `"isSidRobust(burningSid)",".\\Burning.cpp",0x6f)`
- `"*((long**)&s)!=0",".\\Burning.cpp",0x77)`
- `"s.hasHitPoints()",".\\Burning.cpp",0x7f)`

## FUN_00424110 @ 00424110

**798 완료:** 797의 현재 SID/진단 앞단을 실제 소비자와 같은 Client checking/상세 SID owner에 연결했다. 집중13개(새2개) 통과·기존 양판본 몸체 fixture를 유지했다. 아래 미복원 설명은779 당시이며 [798 기록](cpp-burning-victim-live-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBurningSources.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  float *local_1c;
  float *local_18;

  if ((DAT_005e4794 != 0) && (iVar2 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1), iVar2 == 0)
     ) {
    FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x91);
  }
  iVar2 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
  if (iVar2 == 0) {
    return (float *)0x0;
```

원문 assertion 경계:
- `"isSidRobust(burningSid)",".\\Burning.cpp",0x91)`
- `"burningVictim.hasHitPoints()",".\\Burning.cpp",0x98)`
- `"rowTaken[killRow] == 1",".\\Burning.cpp",0xf7)`

## FUN_00424530 @ 00424530

**798 완료:** 797의 현재 SID/진단 앞단을 실제 소비자와 같은 Client checking/상세 SID owner에 연결했다. 집중13개(새2개) 통과·기존 양판본 몸체 fixture를 유지했다. 아래 미복원 설명은779 당시이며 [798 기록](cpp-burning-victim-live-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBurningFrame.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      *(int *)((int)this + 0x54) = *(int *)((int)this + 0x54) + 1;
      return;
    }
    if ((DAT_005e4794 != 0) &&
       (iVar1 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1), iVar1 == 0)) {
      FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x11f);
    }
    iVar1 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
    if (iVar1 != 0) {
      iVar3 = *(int *)((int)this + 0x6c) * 0x32 + DAT_005c8464;
```

원문 assertion 경계:
- `"isSidRobust(burningSid)",".\\Burning.cpp",0x11f)`
- `"s.hasHitPoints()",".\\Burning.cpp",0x126)`

## FUN_00425860 @ 00425860

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\CanonDecoder.cpp, cpppj\src\o\CanonDecoder.h, cpppj\src\client\RawCursorPreviews.h, cpppj\src\client\InventoryItemGump.h, cpppj\src\client\RawCursorPlacement.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      }
    }
LAB_004259a2:
    if (((DAT_005e4794 != 0) && (*(int *)((int)this + 0x10) != -1)) &&
       ((*(byte *)(*(int *)(*(int *)((int)this + 0x28) + 0x124) + 3 + *(int *)((int)this + 0x10) * 4
                  ) & 0x20) != 0)) {
      FUN_004e0620("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)",
                   "\\Ns\\O\\Canondecoder.cpp",0x325);
    }
    local_c = *(int *)((int)this + 0x48) * *(int *)((int)this + 0x1c);
```

원문 assertion 경계:
- `"0 <= frame && frame < ::getType(type)->numFrames","\\Ns\\O\\Canondecoder.cpp", 0x305)`
- `"frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)", "\\Ns\\O\\Canondecoder.cpp",0x325)`

## FUN_0042bc10 @ 0042bc10

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    return;
  }
  iVar1 = param_2 * 0x1cc + 0xb70 + (int)this;
  if (DAT_005e4794 != 0) {
    if ((*(byte *)(iVar1 + 100) & 1) != 0) goto LAB_0042bc9b;
    FUN_004e0620("p->isValid()","\\Ns\\O\\Ch.cpp",0x4a1);
  }
  if ((*(byte *)(iVar1 + 100) & 1) == 0) {
    FUN_004c2630(9,"Cannot delete invalid player number %d\n",param_2);
    return;
```

원문 assertion 경계:
- `"0 < pId && pId < CL_NUM_CHAL_PLAYERS","\\Ns\\O\\Ch.cpp",0x499)`
- `"p->isValid()","\\Ns\\O\\Ch.cpp",0x4a1)`

## FUN_004428c0 @ 004428c0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawConstructionProcess.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int *piVar5;
  int iVar6;

  if (DAT_005e4794 != 0) {
    uVar1 = FUN_0041bfa0();
    iVar2 = FUN_004ae080(uVar1);
    if (iVar2 == 0) {
      FUN_004e0620("getForm().isRobust()","\\Ns\\O\\Construction.cpp",0x1c4);
    }
  }
```

원문 assertion 경계:
- `"getForm().isRobust()","\\Ns\\O\\Construction.cpp",0x1c4)`

## FUN_00446e70 @ 00446e70

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawStateWorldCleanup.h, cpppj\src\o\RawStateWorldCleanup.cpp, cpppj\src\o\RawProductionSync.h, cpppj\src\client\RawCampaignSession.h, cpppj\src\client\UberGump.cpp, cpppj\src\client\RawKnowledgeMenu.h, cpppj\src\client\RawCursorLifecycle.h, cpppj\src\client\InventoryGump.h, cpppj\src\client\GameMenuActions.h, cpppj\src\client\RawCampaignCursor.h, cpppj\src\client\ClientCursorStateCleanup.h, cpppj\src\client\ClientStateWorldCleanup.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
        (**(code **)(*(int *)(iVar1 * 0x32 + DAT_005c8464) + 0x24))(DAT_0055a2a8,4);
        if ((char)DAT_0055a2e0 < '\0') {
          FUN_004a9cb0("teleporterActive.wav",0,0,0,0,0);
          if (DAT_005e4794 != 0) {
            FUN_004e0620("false",".\\Cursor.cpp",0x3f1);
          }
          FUN_0040eec0(-*(float *)(DAT_0055a280 + 0xc4));
        }
      }
      else {
```

원문 assertion 경계:
- `"false",".\\Cursor.cpp",0x3f1)`

## FUN_00448bd0 @ 00448bd0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412ec);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  if (DAT_005e4794 != 0) {
    iVar1 = FUN_004ad710(this);
    if (iVar1 != 0xc) {
      iVar1 = FUN_004ad710(this);
      if (iVar1 != 0xb) goto LAB_00448c2d;
    }
    FUN_004e0620("!isInvisibleFrame() && !isSacrifice()",".\\Dais.cpp",0x578);
```

원문 assertion 경계:
- `"!isInvisibleFrame() && !isSacrifice()",".\\Dais.cpp",0x578)`

## FUN_00449710 @ 00449710

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawPriestSacrificeRequest.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  uStack_4 = 0;
  FUN_004b2270(appuStack_10,((int)this - DAT_005c8464) / 0x32);
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  if ((DAT_005e4794 != 0) && (uVar3 = FUN_004ac200(iVar4), (uVar3 & 0x200000) == 0)) {
    FUN_004e0620("priest.getGenus() & gPRIEST",".\\Dais.cpp",0x140);
  }
  uVar3 = FUN_004ac200(iVar4);
  if (((uVar3 & 0x200000) != 0) && (*(char *)(iVar4 + 0x22) == *(char *)((int)this + 0x22))) {
    while( true ) {
      uVar3 = iStack_8 * 0x32 + DAT_005c8464;
```

원문 assertion 경계:
- `"priest.getGenus() & gPRIEST",".\\Dais.cpp",0x140)`

## FUN_00449900 @ 00449900

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawPathRuntime.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
            if (iVar2 + 1 < 4) {
              piVar4 = (int *)FUN_00489080((int)(uVar5 - DAT_005c8464) / 0x32,iStack_10,iStack_c,1,0
                                           ,((int)this - DAT_005c8464) / 0x32,iVar1,0);
              if (DAT_005e4794 == 0) {
                if (piVar4 == (int *)0x0) {
                  return;
                }
              }
              else if (piVar4 == (int *)0x0) {
                FUN_004e0620(&DAT_005078ac,".\\Dais.cpp",0x18e);
```

원문 assertion 경계:
- `&DAT_005078ac,".\\Dais.cpp",0x18e)`

## FUN_0044c060 @ 0044c060

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawPriestRegen.h, cpppj\src\o\RawDamageableBurning.h, cpppj\src\o\RawPriestDamage.h, cpppj\src\client\RawPriestWorldBindings.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      iVar4 = FUN_0049fce0(uVar3);
      iVar1 = ((int)this - DAT_005c8464) / 0x32;
      iVar5 = FUN_004b1a90(iVar1,DAT_005410a8,0,0);
      if (DAT_005e4794 != 0) {
        iVar6 = FUN_004b1a90(iVar1,DAT_005410a8,1,0);
        if (iVar6 != 0) {
          FUN_004e0620("findProcess (mySid, burningSystemProcessType, 1 ) == 0",
                       "\\Ns\\O\\Damageable.cpp",0x210);
        }
      }
```

원문 assertion 경계:
- `"findProcess (mySid, burningSystemProcessType, 1 ) == 0", "\\Ns\\O\\Damageable.cpp",0x210)`

## FUN_00457890 @ 00457890

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBombEffectTimer.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int iVar1;

  iVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005e4794 != 0) && (*(byte *)(iVar1 + 10) != DAT_00541278)) {
    FUN_004e0620("fence.getType() == sunFenceType","\\Ns\\O\\Fence.cpp",0x60f);
  }
  if (*(byte *)(iVar1 + 10) == DAT_00541278) {
    FUN_00457460(iVar1);
    return;
  }
```

원문 assertion 경계:
- `"fence.getType() == sunFenceType","\\Ns\\O\\Fence.cpp",0x60f)`

## FUN_00457e20 @ 00457e20

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBombStatus.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int iVar1;

  iVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005e4794 != 0) && (*(byte *)(iVar1 + 10) != DAT_00541278)) {
    FUN_004e0620("fence.getType() == sunFenceType","\\Ns\\O\\Fence.cpp",0x602);
  }
  if (*(byte *)(iVar1 + 10) == DAT_00541278) {
    FUN_004578f0(iVar1);
    return;
  }
```

원문 assertion 경계:
- `"fence.getType() == sunFenceType","\\Ns\\O\\Fence.cpp",0x602)`

## FUN_0045c100 @ 0045c100

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawAiDamage.cpp, cpppj\src\o\RawAiDamage.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  }
  if ((*(byte *)(iVar2 + 0xb) & 3) == 0) {
    iVar2 = FUN_004b1a90(param_1,DAT_005410ec,0,0);
    if (DAT_005e4794 == 0) {
      if (iVar2 != 0) goto LAB_0045c17f;
    }
    else {
      if (iVar2 != 0) {
LAB_0045c17f:
        iVar2 = FUN_004ae2d0(iVar2 * 0x32 + DAT_005c8464);
```

원문 assertion 경계:
- `"flyer.getGenus() & gFLYER","\\Ns\\O\\Flyer.cpp",0x8ee)`
- `"processSid","\\Ns\\O\\Flyer.cpp",0x8f5)`

## FUN_00462a50 @ 00462a50

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\Graph.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
{
  short sVar1;

  if (DAT_005e4794 != 0) {
    if (*(short *)((int)this + 2) == 0) {
      FUN_004e0620("inUse","\\Ns\\O\\Graph.cpp",0x59);
    }
    if ((DAT_005e4794 != 0) && (*(short *)this < 1)) {
      FUN_004e0620("numSurfaces > 0","\\Ns\\O\\Graph.cpp",0x5a);
    }
  }
  if ((*(short *)((int)this + 2) != 0) && (0 < *(short *)this)) {
    sVar1 = *(short *)this + -1;
    *(short *)this = sVar1;
```

원문 assertion 경계:
- `"inUse","\\Ns\\O\\Graph.cpp",0x59)`
- `"numSurfaces > 0","\\Ns\\O\\Graph.cpp",0x5a)`

## FUN_00462ac0 @ 00462ac0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
void __cdecl FUN_00462ac0(void)

{
  if (DAT_005e4794 != 0) {
    if (*(short *)(DAT_00562c18 + 0x5f4) != 0x7dfd) {
      FUN_004e0620("graphs[INVALID_GRAPH].numSurfaces == 0x7DFD","\\Ns\\O\\Graph.cpp",0x68);
    }
    if ((DAT_005e4794 != 0) && (*(short *)(DAT_00562c18 + 0x5f6) != 0x7dfd)) {
      FUN_004e0620("graphs[INVALID_GRAPH].inUse == 0x7DFD","\\Ns\\O\\Graph.cpp",0x69);
    }
  }
  return;
}

```

원문 assertion 경계:
- `"graphs[INVALID_GRAPH].numSurfaces == 0x7DFD","\\Ns\\O\\Graph.cpp",0x68)`
- `"graphs[INVALID_GRAPH].inUse == 0x7DFD","\\Ns\\O\\Graph.cpp",0x69)`

## FUN_00462bc0 @ 00462bc0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawPlayerPlacementAnchor.h, cpppj\src\o\RawBridgeCarriers.h, cpppj\src\o\RawTutorialOne.h, cpppj\src\o\RawTutorialLater.h, cpppj\src\o\RawCarrierChoice.h, cpppj\src\o\RawAiPreparation.h, cpppj\src\o\RawAiResourceChoice.h, cpppj\src\o\RawAiCarrierActions.h, cpppj\src\o\RawAiBridgePlanning.h, cpppj\src\o\RawAiBridgeStrategies.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  uint uVar2;
  undefined4 uVar3;

  if (DAT_005e4794 == 0) {
    if (param_1 == 0) {
      return 0;
    }
  }
  else if (param_1 == 0) {
    FUN_004e0620("mySid","\\Ns\\O\\Graph.cpp",0x93);
```

원문 assertion 경계:
- `"mySid","\\Ns\\O\\Graph.cpp",0x93)`

## FUN_00462f10 @ 00462f10

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\Graph.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
             (uVar5 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))) {
        iVar1 = FUN_004ad390();
        if (iVar1 != param_2) {
          if ((DAT_005e4794 != 0) && ((local_cc * 0x32) / 0x32 == iVar7)) {
            FUN_004e0620("cFinder.s().getSid() != sid","\\Ns\\O\\Graph.cpp",0x176);
          }
          DAT_00562c20[iVar3] = (local_cc * 0x32) / 0x32;
          iVar3 = iVar3 + 1;
          if (0xfff < iVar3) {
            FUN_004e0620("d < FLOOD_STACK_SIZE","\\Ns\\O\\Graph.cpp",0x17a);
```

원문 assertion 경계:
- `"cFinder.s().getSid() != sid","\\Ns\\O\\Graph.cpp",0x176)`
- `"d < FLOOD_STACK_SIZE","\\Ns\\O\\Graph.cpp",0x17a)`

## FUN_004633c0 @ 004633c0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\Graph.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  local_208 = *(float *)(param_1 * 0x32 + 0xe + DAT_005c8464);
  iVar9 = param_1 * 0x32 + DAT_005c8464;
  local_204 = *(float *)(iVar9 + 0x12);
  if ((DAT_005e4794 != 0) && (iVar2 = FUN_0040e800(&local_208), iVar2 == 0)) {
    FUN_004e0620("pos.isValid()","\\Ns\\O\\Graph.cpp",0xbd);
  }
  if (local_208 <= _DAT_00500fc4) {
    return;
  }
  if ((float)DAT_00531928 <= local_208) {
// ...
  if ((float)DAT_00531928 <= local_204) {
    return;
  }
  if (DAT_005e4794 != 0) {
    if ((*(byte *)(iVar9 + 0xb) & 7) == 0) goto LAB_004634ab;
    FUN_004e0620("!s.isVoid() && !s.isDead() && !s.isFree()","\\Ns\\O\\Graph.cpp",0xc2);
  }
  if ((*(byte *)(iVar9 + 0xb) & 7) != 0) {
    return;
  }
// ...
                       "\\Ns\\O\\Graph.cpp",0xfb);
        }
        psVar5 = (short *)(DAT_00562c18 + iVar2 * 6);
        if ((DAT_005e4794 != 0) && (psVar5[1] != 1)) {
          FUN_004e0620("g->inUse == true","\\Ns\\O\\Graph.cpp",0x100);
        }
        if (psVar5[1] == 0) {
          psVar5[1] = 1;
        }
        FUN_004ad3e0(iVar9,iVar2);
// ...
            if ((iVar9 != 0) && (iVar3 = FUN_004ad390(iVar9 * 0x32 + DAT_005c8464), iVar3 != iVar2))
            {
              psVar5 = (short *)FUN_00462c80(iVar9);
              if ((DAT_005e4794 != 0) && ((int)local_208 < (int)*psVar5)) {
                FUN_004e0620("loser->numSurfaces <= biggestSize","\\Ns\\O\\Graph.cpp",0x113);
              }
              FUN_00462f10(iVar9,iVar2);
              local_210 = local_210 + 1;
              *psVar5 = 0;
              psVar5[1] = 0;
            }
            local_20c = local_20c + 1;
          } while (local_20c < 0x40);
          if ((DAT_005e4794 != 0) && (local_210 != iVar7 + -1)) {
            FUN_004e0620("numFlooded == numConnections - 1","\\Ns\\O\\Graph.cpp",0x11f);
          }
        }
      }
      iVar2 = 0;
      psVar5 = (short *)(DAT_00562c18 + 2);
```

원문 assertion 경계:
- `"pos.isValid()","\\Ns\\O\\Graph.cpp",0xbd)`
- `"!s.isVoid() && !s.isDead() && !s.isFree()","\\Ns\\O\\Graph.cpp",0xc2)`
- `"g->inUse == true","\\Ns\\O\\Graph.cpp",0xf3)`
- `"biggestConnection >= 0 && biggestConnection < MAX_GRAPHS", "\\Ns\\O\\Graph.cpp",0xfb)`
- `"g->inUse == true","\\Ns\\O\\Graph.cpp",0x100)`
- `"loser->numSurfaces <= biggestSize","\\Ns\\O\\Graph.cpp",0x113)`
- `"numFlooded == numConnections - 1","\\Ns\\O\\Graph.cpp",0x11f)`

## FUN_00464420 @ 00464420

**796 완료:** 같은 Client checking 뷰·진단 복귀를 공통 수명에 연결했다. 두 부모의 원본368입력/736회와 재관측736회 일치·집중19개(새2개) 통과다. 아래 미복원 설명은779 당시이며 실제 배열 경계는 [796 기록](cpp-gump-checking-parents-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\client\ChatWindow.h, cpppj\src\client\TextEntryGump.h, cpppj\src\client\BackgroundGifGump.h, cpppj\src\client\HtmlWindowGump.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int *piVar1;

  *(undefined ***)this = &PTR_FUN_0050ac70;
  if ((DAT_005e4794 != 0) && ((*(byte *)((int)this + 4) & 4) == 0)) {
    FUN_004e0620("flags & GF_DESTROYING",".\\Gump.cpp",0x1e1);
  }
  if (*(void **)((int)this + 0x78) != (void *)0x0) {
    _free(*(void **)((int)this + 0x78));
    *(undefined4 *)((int)this + 0x78) = 0;
  }
```

원문 assertion 경계:
- `"flags & GF_DESTROYING",".\\Gump.cpp",0x1e1)`

## FUN_004653c0 @ 004653c0

**796 완료:** 같은 Client checking 뷰·진단 복귀를 공통 수명에 연결했다. 두 부모의 원본368입력/736회와 재관측736회 일치·집중19개(새2개) 통과다. 아래 미복원 설명은779 당시이며 실제 배열 경계는 [796 기록](cpp-gump-checking-parents-20261103.md)을 따른다.

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\client\HudDock.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    iVar2 = iVar2 + 5;
  } while (iVar1 < 100);
  if (param_2 == 0) {
    if (DAT_005e4794 == 0) {
      if (iVar5 == -1) {
        return;
      }
    }
    else if (iVar5 == -1) {
      FUN_004e0620("found!=-1",".\\Gump.cpp",0x4f4);
```

원문 assertion 경계:
- `"found!=-1",".\\Gump.cpp",0x4f4)`
- `"found==-1 && free!=-1",".\\Gump.cpp",0x4ef)`

## FUN_00466750 @ 00466750

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\client\MoveDialog.h, cpppj\src\client\RawButtonControls.cpp, cpppj\src\client\GumpFormBinding.h, cpppj\src\client\GumpTree.h, cpppj\src\client\MenuGumpInput.h, cpppj\src\client\MenuGumpWidgets.h, cpppj\src\client\ScrollGump.h, cpppj\src\client\ScrollingTextGumpWidgets.h, cpppj\src\client\StoneButtonGump.h, cpppj\src\client\HudChatGeometry.h, cpppj\src\client\HudDock.h, cpppj\src\client\HudNsInfo.h, cpppj\src\client\HudNsInfoCreate.h, cpppj\src\client\TextEntryGump.h, cpppj\src\client\RawProductionControls.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  undefined1 local_108 [8];
  undefined1 local_100 [256];

  if ((DAT_005e4794 != 0) &&
     ((iVar5 = *(int *)((int)this + 0x20), iVar5 < 1 || (DAT_0054045e <= iVar5)))) {
    uVar4 = FUN_004a89c0(local_100,"gt = %d",iVar5);
    FUN_00436d20(uVar4);
  }
  *(uint *)((int)this + 4) = *(uint *)((int)this + 4) & 0xfffffff7;
  if (param_4 != 0) {
```

원문 assertion 경계:
- `"parent",".\\Gump.cpp",0x24d)`

## FUN_00467610 @ 00467610

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawGunAngle.h, cpppj\src\client\RawCampaignSession.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  if (0x12 < param_1) {
    param_1 = 0x13;
  }
  if ((DAT_005e4794 != 0) && ((param_1 < 0 || (0x13 < param_1)))) {
    FUN_004e0620("0 <= index && index < 20","\\Ns\\O\\Gunanim.cpp",0x387);
  }
  fVar1 = (float)(&DAT_00540940)[param_1];
  if (_DAT_0050229c <= fVar1) {
    if (fVar1 < _DAT_0050af94) {
      return 1;
```

원문 assertion 경계:
- `"0 <= index && index < 20","\\Ns\\O\\Gunanim.cpp",0x387)`

## FUN_0046a340 @ 0046a340

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawGunProcess.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...

  if (param_2 == 1) {
    if (param_4 == 4) {
      if (DAT_005e4794 != 0) {
        iVar1 = FUN_00469070((int)this + -0x28);
        if (iVar1 == 0) {
          FUN_004e0620("getAnimFormSid() != 0","\\Ns\\O\\Gunprocess.cpp",0x154);
        }
      }
      *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
// ...
    }
    if (param_4 == 7) {
      if (*(int *)((int)this + -0xc) == 6) {
        if (DAT_005e4794 != 0) {
          iVar1 = FUN_00469070((int)this + -0x28);
          if (iVar1 == 0) {
            FUN_004e0620("getAnimFormSid() != 0","\\Ns\\O\\Gunprocess.cpp",0x160);
          }
        }
        *(byte *)((int)this + -0x20) = *(byte *)((int)this + -0x20) | 8;
// ...
    }
  }
  else if ((DAT_00540bc0 != 0) && (param_2 == 2)) {
    if ((DAT_005e4794 != 0) && (param_4 != *(ushort *)((int)this + -0x10))) {
      FUN_004e0620("info2 == getTargetSid()","\\Ns\\O\\Gunprocess.cpp",0x16e);
    }
    if (param_4 == *(ushort *)((int)this + -0x10)) {
      FUN_0046a290((int)this + -0x28,1);
      iVar1 = FUN_0041bfb0(this);
      if ((*(byte *)(iVar1 + 0xb) & 2) == 0) {
```

원문 assertion 경계:
- `"getAnimFormSid() != 0","\\Ns\\O\\Gunprocess.cpp",0x154)`
- `"getAnimFormSid() != 0","\\Ns\\O\\Gunprocess.cpp",0x160)`
- `"false","\\Ns\\O\\Gunprocess.cpp",0x16a)`
- `"info2 == getTargetSid()","\\Ns\\O\\Gunprocess.cpp",0x16e)`

## FUN_0046a520 @ 0046a520

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
        FUN_0046a1a0(pvVar2,((int)this - DAT_005c8464) / 0x32);
      }
      local_4 = 0xffffffff;
      if (DAT_005e4794 != 0) {
        iVar3 = FUN_004ad730(this);
        if (iVar3 == 0) {
          FUN_004e0620("checkForms()","\\Ns\\O\\Gunprocess.cpp",0x347);
        }
      }
    }
```

원문 assertion 경계:
- `"checkForms()","\\Ns\\O\\Gunprocess.cpp",0x347)`

## FUN_0046a600 @ 0046a600

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      FUN_0046a1a0(pvVar1,((int)this - DAT_005c8464) / 0x32);
    }
    local_4 = 0xffffffff;
    if (DAT_005e4794 != 0) {
      iVar2 = FUN_004ad730(this);
      if (iVar2 == 0) {
        FUN_004e0620("checkForms()","\\Ns\\O\\Gunprocess.cpp",0x361);
      }
    }
  }
```

원문 assertion 경계:
- `"checkForms()","\\Ns\\O\\Gunprocess.cpp",0x361)`

## FUN_0046a810 @ 0046a810

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
                 "\\Ns\\O\\Gunprocess.cpp",0x3c8);
  }
LAB_0046a8e7:
  if (DAT_005e4794 != 0) {
    iVar2 = FUN_004ad730(this);
    if (iVar2 == 0) {
      FUN_004e0620("checkForms()","\\Ns\\O\\Gunprocess.cpp",0x3c9);
    }
  }
LAB_0046a912:
```

원문 assertion 경계:
- `"inFortMode || ( !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid() )", "\\Ns\\O\\Gunprocess.cpp",0x3c8)`
- `"checkForms()","\\Ns\\O\\Gunprocess.cpp",0x3c9)`

## FUN_0046aca0 @ 0046aca0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
                   "\\Ns\\O\\Gunprocess.cpp",0x32a);
    }
LAB_0046ad51:
    if (DAT_005e4794 != 0) {
      iVar2 = FUN_004ad730(this);
      if (iVar2 == 0) {
        FUN_004e0620("checkForms()","\\Ns\\O\\Gunprocess.cpp",0x32b);
      }
    }
  }
```

원문 assertion 경계:
- `"duringLoad || !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid()", "\\Ns\\O\\Gunprocess.cpp",0x32a)`
- `"checkForms()","\\Ns\\O\\Gunprocess.cpp",0x32b)`

## FUN_00471650 @ 00471650

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\client\PaletteFlash.h, cpppj\src\client\ThunderProcess.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  double local_10;

  local_10 = _DAT_0055b4d8 - *(double *)((int)this + 0x818);
  if ((DAT_005e4794 != 0) && (0x40 < (-1 < *(int *)((int)this + 0x838)))) {
    FUN_004e0620("0 <= step <= 64",".\\Kencloud.cpp",0x29e);
  }
  if (*(double *)((int)this + 0x810) < local_10) {
    do {
      if (*(int *)((int)this + 0x838) < 1) break;
      iVar4 = 0;
```

원문 assertion 경계:
- `"0 <= step <= 64",".\\Kencloud.cpp",0x29e)`

## FUN_00472f70 @ 00472f70

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  }
  bVar1 = false;
LAB_00472f94:
  if (DAT_005e4794 == 0) {
LAB_00472fc1:
    if (bVar1) goto LAB_00472fd6;
  }
  else if (bVar1) {
    FUN_004e0620("!alreadyThere","\\Ns\\O\\Mana.cpp",0x44);
    goto LAB_00472fc1;
```

원문 assertion 경계:
- `"!alreadyThere","\\Ns\\O\\Mana.cpp",0x44)`

## FUN_00482340 @ 00482340

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  float local_118;
  undefined1 auStack_110 [272];

  if ((DAT_005e4794 != 0) &&
     (iVar3 = FUN_0041be80(this,*(undefined4 *)((int)this + 0x14)), iVar3 == 0)) {
    FUN_004e0620("isRobust(firingSid)",".\\Missile.cpp",0x1ee);
  }
  fVar1 = DAT_0055b4d4;
  uVar10 = DAT_0055b4d0;
  piVar12 = (int *)(*(int *)((int)this + 8) * 0x32 + DAT_005c8464);
// ...
      *(undefined4 *)((int)this + 0x10) = 1;
    }
  }
  if ((DAT_005e4794 != 0) &&
     (iVar3 = FUN_0041be80(this,*(undefined4 *)((int)this + 0x14)), iVar3 == 0)) {
    FUN_004e0620("isRobust(firingSid)",".\\Missile.cpp",0x2a0);
  }
  *(float *)((int)this + 0x7c) = unaff_ESI;
  *(float *)((int)this + 0x78) = unaff_EDI;
  if ((!bVar2) && (bVar13)) {
```

원문 assertion 경계:
- `"isRobust(firingSid)",".\\Missile.cpp",0x1ee)`
- `"isRobust(firingSid)",".\\Missile.cpp",0x2a0)`

## FUN_00489ff0 @ 00489ff0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawPathRetarget.h, cpppj\src\o\RawBalloonFrame.h, cpppj\src\o\RawBulfFrame.h, cpppj\src\o\RawPathProcess.h, cpppj\src\client\RawCampaignSession.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  local_8 = 0;
  iVar1 = (**(code **)(*piVar4 + 0x38))();
  if (local_8 != 0) {
    if (DAT_005e4794 == 0) {
      return true;
    }
    if (iVar1 == 0xc) {
      return true;
    }
    FUN_004e0620("result == haltALREADY_DEAD","\\Ns\\O\\Pathprocess.cpp",0x2d5);
```

원문 assertion 경계:
- `"result == haltALREADY_DEAD","\\Ns\\O\\Pathprocess.cpp",0x2d5)`
- `"!deadFlag","\\Ns\\O\\Pathprocess.cpp",0x2f2)`

## FUN_0048b340 @ 0048b340

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBalloonFrame.h, cpppj\src\o\RawBulfFrame.h, cpppj\src\o\RawPathProcess.h, cpppj\src\o\RawWalkerFrame.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  (**(code **)(*piVar5 + 0x48))();
  iVar6 = 0;
  (**(code **)(*piVar5 + 0x90))(uStack_4c,local_48[0]);
  if ((DAT_005e4794 != 0) && (iVar2 = FUN_0041be80(this,1), iVar2 == 0)) {
    FUN_004e0620("isRobust()","\\Ns\\O\\Pathprocess.cpp",0x265);
  }
  iVar2 = FUN_0041be80(this,1);
  if (iVar2 != 0) {
    bVar1 = false;
    iVar2 = FUN_004e49c0();
// ...
    iVar2 = (**(code **)(*(int *)this + 0x28))();
    if (iVar2 == 0) {
      *(undefined4 *)((int)this + 0x186c8) = 1;
      if ((DAT_005e4794 != 0) && (iVar2 = FUN_0041be80(this,1), iVar2 == 0)) {
        FUN_004e0620("isRobust()","\\Ns\\O\\Pathprocess.cpp",0x284);
      }
      if ((*(byte *)((int)piVar5 + 0xb) & 3) != 0) {
        FUN_004e0620("!walker.isDead() && !walker.isFree()","\\Ns\\O\\Pathprocess.cpp",0x286);
      }
      if (*(int *)((int)this + 0x186ec) != 0) {
// ...
      if (iVar7 == 0) {
        *(undefined4 *)((int)this + 0x186ec) = 0;
        if (iVar2 == 0x3ea) {
          if ((DAT_005e4794 != 0) && (iVar7 = FUN_0041be80(this,1), iVar7 == 0)) {
            FUN_004e0620("isRobust()","\\Ns\\O\\Pathprocess.cpp",0x29f);
          }
          *(undefined4 *)((int)this + 0x186c8) = 0;
        }
        else {
          if ((DAT_005e4794 != 0) && (iVar7 = FUN_0041be80(this,1), iVar7 == 0)) {
            FUN_004e0620("isRobust()","\\Ns\\O\\Pathprocess.cpp",0x2a4);
          }
          FUN_0041bf10(this,0);
        }
        if (iVar6 != 0) {
          FUN_00443800(*(undefined4 *)((int)piVar5 + 0xe),*(undefined4 *)((int)piVar5 + 0x12),
// ...
        }
      }
      else {
        if ((DAT_005e4794 != 0) && (iVar2 != 0x3e9)) {
          FUN_004e0620("returnCode == pathALREADY_DEAD","\\Ns\\O\\Pathprocess.cpp",0x290);
        }
        if (iVar6 != 0) {
          FUN_00443800(*(undefined4 *)((int)piVar5 + 0xe),*(undefined4 *)((int)piVar5 + 0x12),
                       *(undefined1 *)((int)piVar5 + 10),*(undefined1 *)((int)piVar5 + 0x22));
          return;
```

원문 assertion 경계:
- `"walker.isValid() && !walker.isDead() && !walker.isFree()", "\\Ns\\O\\Pathprocess.cpp",0x25c)`
- `"isRobust()","\\Ns\\O\\Pathprocess.cpp",0x265)`
- `"isRobust()","\\Ns\\O\\Pathprocess.cpp",0x284)`
- `"!walker.isDead() && !walker.isFree()","\\Ns\\O\\Pathprocess.cpp",0x286)`
- `"!deadFlag","\\Ns\\O\\Pathprocess.cpp",0x289)`
- `"returnCode == pathKILL || returnCode == pathALREADY_DEAD || returnCode == pathLINGER" ,"\\Ns\\O\\Pathprocess.cpp",0x28d)`
- `"isRobust()","\\Ns\\O\\Pathprocess.cpp",0x29f)`
- `"isRobust()","\\Ns\\O\\Pathprocess.cpp",0x2a4)`
- `"returnCode == pathALREADY_DEAD","\\Ns\\O\\Pathprocess.cpp",0x290)`

## FUN_0048fb30 @ 0048fb30

별도 부모 후속: MissionClientPackets는SP/MP공통수신이다.기존수신부모의checking관문은정적으로후속대조하고신규MP검사는하지않음

현재 주소 언급: cpppj\src\o\RawMissionClientPackets.h, cpppj\src\o\RawMissionLogin.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    uVar2 = *(undefined1 *)(iVar3 + 0x71 + DAT_0059534c);
    if ((DAT_00595344 == 0) || (param_1 == _DAT_00540c70)) {
      DAT_00595348 = 1;
      if ((DAT_005e4794 != 0) &&
         (*(int *)(iVar3 + 0x2c + DAT_0059534c) != (&DAT_00595360)[param_1 * 4])) {
        FUN_004e0620("getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId",
                     "\\Ns\\O\\Player.cpp",0x224);
      }
      param_1 = param_1 * 0x10;
      puVar1 = (undefined4 *)(iVar3 + 0x6c + DAT_0059534c);
// ...
      DAT_00595350 = _DAT_00540c70 * 0xb4 + 0x6c + DAT_0059534c;
    }
    else {
      if ((DAT_005e4794 != 0) &&
         (*(int *)(iVar3 + 0x2c + DAT_0059534c) != (&DAT_00595360)[param_1 * 4])) {
        FUN_004e0620("getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId",
                     "\\Ns\\O\\Player.cpp",0x21f);
      }
      param_1 = param_1 * 0x10;
      puVar1 = (undefined4 *)(iVar3 + 0x6c + DAT_0059534c);
```

원문 assertion 경계:
- `"getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId", "\\Ns\\O\\Player.cpp",0x224)`
- `"getPlayer(playerId)->subscriberId == cachedCoreData[playerId].subId", "\\Ns\\O\\Player.cpp",0x21f)`

## FUN_004a7510 @ 004a7510

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    iVar7 = iVar7 + 1;
    piVar3 = piVar3 + 0x2d;
  } while (iVar7 < 9);
  if (DAT_005e4794 == 0) {
    return;
  }
  FUN_004e0620();
  return;
}

```

원문 assertion 경계:
- `)`

## FUN_004ad500 @ 004ad500

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\RawBombStatus.h, cpppj\src\o\SquidDisplay.cpp, cpppj\src\o\SelectionVtables.inc, cpppj\src\o\RawBombEffectTimer.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  int iVar4;

  iVar1 = *(int *)((int)this + 0x24);
  if (DAT_005e4794 == 0) goto LAB_004ad580;
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    if ((piVar2[0x45] <= iVar1) || (iVar1 < 0)) goto LAB_004ad54f;
LAB_004ad5ea:
    if (((piVar2[0x37] != 0) && (*piVar2 != -0x22222223)) && (*piVar2 != -0x32323233))
    goto LAB_004ad580;
// ...
  else {
    if ((iVar1 < piVar2[0x45] * 2) && (-1 < iVar1)) goto LAB_004ad5ea;
LAB_004ad54f:
    if (DAT_005e4794 != 0) {
      uVar3 = FUN_0049adc0(piVar2,iVar1,*(byte *)((int)this + 0x28) & 1);
      FUN_00436d40(uVar3);
    }
  }
  FUN_004e0620("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp",
               0x846);
// ...
                   *(undefined4 *)((int)this + 0x12),(*(byte *)((int)this + 0x28) >> 2 & 1) << 4);
    }
  }
  else if (DAT_005e4794 != 0) {
    uVar3 = FUN_0049adc0(piVar2,iVar1,*(byte *)((int)this + 0x28) & 1);
    FUN_00436d40(uVar3);
    return;
  }
  return;
}
```

원문 assertion 경계:
- `"getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp", 0x846)`

## FUN_004aec10 @ 004aec10

별도 부모 후속: ContentFormState.checkingLinks view는준비했으나실제Session은state=nullptr이며inBattle/포획통지부모연결이먼저필요함

현재 주소 언급: cpppj\src\o\ContentFormHost.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  iVar4 = FUN_0049a840(*(undefined2 *)((int)this + 0x12));
  if ((((*(uint *)(iVar4 + 0xec) & 0x200000) != 0) &&
      (uVar5 = FUN_004ac200(piVar6), (uVar5 & 0x30000) != 0)) && (DAT_00594fbc != 0)) {
    if (DAT_005e4794 != 0) {
      iVar4 = FUN_0040ea40(this);
      if (iVar4 == 0) {
        FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0xc2b);
      }
      if ((DAT_005e4794 != 0) && (iVar4 = FUN_004ae080(this), iVar4 == 0)) {
        FUN_004e0620("isRobust()","\\Ns\\O\\Squid.cpp",0xc2c);
      }
    }
    iVar4 = FUN_0040ea40(this);
    if ((iVar4 != 0) && (iVar4 = FUN_004ae080(this), iVar4 != 0)) {
      FUN_004927f0(unaff_retaddr,*(byte *)((int)this + 0xc) & 0x7f);
```

원문 assertion 경계:
- `"!into.isDead() && !into.isFree()","\\Ns\\O\\Squid.cpp",0xbe6)`
- `"!isDead() && !isFree()","\\Ns\\O\\Squid.cpp",0xbe7)`
- `"isValid()","\\Ns\\O\\Squid.cpp",0xc2b)`
- `"isRobust()","\\Ns\\O\\Squid.cpp",0xc2c)`

## FUN_004afe50 @ 004afe50

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\SquidSpatial.h, cpppj\src\o\SquidUnpop.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
              iVar10 = FUN_004e49c0();
              iVar11 = FUN_004e49c0();
              bVar1 = *(byte *)(iVar11 + iVar10 * 0x100 + DAT_005c7c44);
              if ((DAT_005e4794 != 0) && ((bVar1 & bVar6) != bVar6)) {
                FUN_004e0620("(flags & effectiveGenus) == ( unsigned char)effectiveGenus",
                             "\\Ns\\O\\Squid.cpp",0x617);
              }
              *(byte *)(iVar16 + DAT_005c7c44 + iVar5) = ~bVar6 & bVar1;
            }
            if (((iVar5 == iVar15) || (iVar5 == iVar8)) &&
// ...
              }
            }
            iVar10 = FUN_0049a840(*(undefined1 *)((int)this + 10));
            if ((((*(uint *)(iVar10 + 0xe8) & 0x800) != 0) && (DAT_005e4794 != 0)) &&
               (iVar10 = FUN_0040eaf0((float)iVar5,(float)local_28),
               iVar10 != ((int)this - DAT_005c8464) / 0x32)) {
              FUN_004e0620("getGridSid( (float)fx, (float)fy) == getSid()","\\Ns\\O\\Squid.cpp",
                           0x629);
            }
          }
```

원문 assertion 경계:
- `"!isForm()","\\Ns\\O\\Squid.cpp",0x5d8)`
- `"!isContained()","\\Ns\\O\\Squid.cpp",0x5d9)`
- `"isValid()","\\Ns\\O\\Squid.cpp",0x5e0)`
- `"(flags & effectiveGenus) == ( unsigned char)effectiveGenus", "\\Ns\\O\\Squid.cpp",0x617)`
- `"getGridSid( (float)fx, (float)fy) == getSid()","\\Ns\\O\\Squid.cpp", 0x629)`
- `"found","\\Ns\\O\\Squid.cpp",0x65d)`

## FUN_004b02d0 @ 004b02d0

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: cpppj\src\o\SquidPop.h, cpppj\src\o\SquidSpatial.h, cpppj\src\client\RawSurfaceWorld.cpp

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
              if (iVar9 != 0) {
                iVar9 = FUN_0040eaf0((float)iVar6,fVar2);
                iVar9 = iVar9 * 0x32 + DAT_005c8464;
                if (DAT_005e4794 != 0) {
                  FUN_004e0620("false","\\Ns\\O\\Squid.cpp",0x6e8);
                }
                iVar13 = iVar9 - DAT_005c8464;
                iVar10 = FUN_0049a840(*(undefined1 *)((int)this + 10));
                iVar14 = (int)this - DAT_005c8464;
                iVar9 = FUN_0049a840(*(undefined1 *)(iVar9 + 10));
```

원문 assertion 경계:
- `"start.isValid()","\\Ns\\O\\Squid.cpp",0x6b7)`
- `"false","\\Ns\\O\\Squid.cpp",0x6e8)`
- `"!isContained()","\\Ns\\O\\Squid.cpp",0x71a)`

## FUN_004b4df0 @ 004b4df0

후순위 MP 전용 연결 상태 진단: meta connected/monitor 상태 관문을 원문대로 사전 구현할 범위, 신규 MP 검증0

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  undefined4 uVar1;
  undefined1 local_100 [256];

  if (((DAT_005e4794 != 0) && (DAT_005c8758 != 0x1d)) && (DAT_005c8758 != 0x21)) {
    FUN_004e0620("currState.state == S_META_CHAL_CONNECTED || currState.state == S_META_CHAL_CONNECTED_MONITOR"
                 ,".\\State.cpp",0xdef);
  }
  _DAT_005c88cc = DAT_00513870;
  _DAT_005c88d0 = DAT_00513874;
  DAT_005c88d4 = DAT_00513878;
```

원문 assertion 경계:
- `"currState.state == S_META_CHAL_CONNECTED || currState.state == S_META_CHAL_CONNECTED_MONITOR" ,".\\State.cpp",0xdef)`

## FUN_004b54e0 @ 004b54e0

별도 부모 후속: Begin getter는779공유하지만Update의파일명/count checking진단2개는별도누락

현재 주소 언급: cpppj\src\o\RawMissionClientPackets.h, cpppj\src\o\RawMissionLoginTimer.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    local_11c = 0;
    FUN_0041b4c0(local_130,uVar3,2,1);
    local_c = 0;
    if ((DAT_005e4794 != 0) && (pcVar4 = _strchr(DAT_0055b490,0x2e), pcVar4 != (char *)0x0)) {
      FUN_004e0620("!(strchr(curFort,\'.\'))",".\\State.cpp",0xf79);
    }
    local_13c = 0xa004e;
    FUN_0041a910(local_130,&local_13c,4,0xffffffff);
    pcVar4 = local_118;
    for (iVar5 = 0x40; iVar5 != 0; iVar5 = iVar5 + -1) {
// ...
      iVar5 = iVar5 + 0xb4;
      uVar6 = uVar6 + 1;
    } while (iVar5 < 0x654);
    if ((DAT_005e4794 != 0) && (1 < local_13c)) {
      FUN_004e0620("serverCount <= 1",".\\State.cpp",0xfa3);
    }
    local_138[0] = DAT_00595090;
    FUN_0041a910(local_130,local_138,4,0xffffffff);
    local_138[0] = DAT_0055a420;
    FUN_0041a910(local_130,local_138,4,0xffffffff);
```

원문 assertion 경계:
- `"!(strchr(curFort,\'.\'))",".\\State.cpp",0xf79)`
- `"serverCount <= 1",".\\State.cpp",0xfa3)`

## FUN_004b8b20 @ 004b8b20

후순위 MP 전용 연결 상태 진단: meta connected/monitor 상태 관문을 원문대로 사전 구현할 범위, 신규 MP 검증0

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
  undefined1 local_100 [256];

  if ((DAT_00540bc0 == 0) && (DAT_005c85c0 == 0)) {
    if ((DAT_005e4794 != 0) && ((DAT_005c8758 != 0x1d && (DAT_005c8758 != 0x21)))) {
      FUN_004e0620("currState.state == S_META_CHAL_CONNECTED || currState.state == S_META_CHAL_CONNECTED_MONITOR"
                   ,".\\State.cpp",0xe39);
    }
    if (DAT_0054dbac != 0) {
      uVar2 = FUN_004a89c0(local_100,"%08X.%X.c.dat",*(undefined4 *)(in_stack_0000000c + 4),
                           DAT_00540c74);
```

원문 assertion 경계:
- `"currState.state == S_META_CHAL_CONNECTED || currState.state == S_META_CHAL_CONNECTED_MONITOR" ,".\\State.cpp",0xe39)`

## FUN_004b8cb0 @ 004b8cb0

별도 부모 후속: ClientLocalStateGlobals의전환진단:state부모의현재값재조회/재진입을후속복원

현재 주소 언급: cpppj\src\client\ClientLocalStateGlobals.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      if ((((*(uint *)(iVar1 + 0x50) & 1) == 0) || ((*(uint *)(iVar1 + 0x50) & 0x200) != 0)) ||
         ((param_1 != 0 && (uVar2 + 1 == DAT_00542528)))) goto LAB_004b8ea8;
      if (*(int *)(iVar1 + 0x2c) == DAT_00540c74) {
        if ((DAT_005e4794 != 0) && (param_1 != 0)) {
          FUN_004e0620("!reloadFile",".\\State.cpp",0xf24);
        }
        DAT_005c87e8 = 0xf;
        FUN_004b6dd0();
      }
      else {
```

원문 assertion 경계:
- `"!reloadFile",".\\State.cpp",0xf24)`

## FUN_004c0e20 @ 004c0e20

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      iVar2 = iVar2 + 1;
    } while (iVar2 < *(int *)((int)this + 0x2083c));
  }
  if (DAT_005e4794 != 0) {
    FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Terrainbuilder.cpp",0x25f);
  }
  return 0x2333;
}


```

원문 assertion 경계:
- `&DAT_0050161c,"\\Ns\\O\\Terrainbuilder.cpp",0x25f)`

## FUN_004c9350 @ 004c9350

별도 부모 후속: SendMoneyInput/SaveFort공통부모checking진단은Session진단뷰와별개이며후속일반입력범위

현재 주소 언급: cpppj\src\client\RawMenuSaveFort.inc, cpppj\src\client\SendMoneyInput.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
      iVar4 = *(int *)((int)this + 0xd4);
      uVar1 = *(uint *)(iVar4 * 0xb4 + 0x50 + DAT_0059534c);
      if ((uVar1 & 0x20) != 0) {
        if ((DAT_005e4794 != 0) && (DAT_00594fa4 == 0)) {
          FUN_004e0620("inSingle",".\\Ui.cpp");
        }
        FUN_0040eec0();
        FUN_00413bb0(iVar4,lVar5);
        return 1;
      }
```

원문 assertion 경계:
- `"inSingle",".\\Ui.cpp")`

## FUN_004d62b0 @ 004d62b0

별도 부모 후속: 775는AI일반명령만복원.기존Guide/Console/정지입력의별도checking진단3개는후속UI부모범위

현재 주소 언급: cpppj\src\client\ClientMain.cpp, cpppj\src\client\GameMenuActions.h

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
                    }
                  }
                  else {
                    if ((DAT_005e4794 != 0) && (DAT_00594fa4 == 0)) {
                      FUN_004e0620();
                    }
                    FUN_0040eec0();
                    FUN_00413bb0();
                  }
                }
// ...
                    }
                  }
                  else {
                    if ((DAT_005e4794 != 0) && (DAT_00594fa4 == 0)) {
                      FUN_004e0620();
                    }
                    FUN_0040eec0();
                    FUN_00413bb0();
                  }
                }
// ...
                    }
                  }
                  else {
                    if ((DAT_005e4794 != 0) && (DAT_00594fa4 == 0)) {
                      FUN_004e0620();
                    }
                    FUN_0040eec0();
                    FUN_00413bb0();
                  }
                }
```

원문 assertion 경계:
- `)`
- `)`
- `)`
- `)`
- `)`
- `)`
- `)`
- `)`
- `)`
- `)`

## FUN_004dc220 @ 004dc220

기존 현재 gate/부모 미복원: 아래 후속 묶음에서 조건/효과/원문 assertion 재개 순서를 복원할 대상

현재 주소 언급: 현재 제품에 직접 주소 언급 없음. 앞/뒤 가상 호출과 SOURCE_MAP을 통해 연결 대상을 추가 확인해야 함.

예외 한계: 원문은 아래 진단을 선택한 뒤 assertion이 돌아오면 본문을 계속한다. 현재 일부 모듈의 throw는 물리 범위/진단 장치 미연결을 명시하는 호스트 경계다. 전체 원문 parent의 동작을 복원한 것으로 세지 않으며 실제 게임/MP/공중3종/공격·포획·희생·승리 재현으로 이 공백을 검증하지 않는다.

원문 모든 checking 참조 주변:
```cpp
// ...
    if (param_3 == 0) {
      FUN_004ac220(this,4);
    }
    if ((DAT_005e4794 == 0) && ((float)param_3 <= _DAT_00502314)) goto LAB_004dc449;
    uVar2 = FUN_0046f650(&DAT_00568b60,*(undefined4 *)((int)this + 0xe),
                         *(undefined4 *)((int)this + 0x12));
    uVar2 = DAT_0054db4c << 0x10 | uVar2;
    FUN_00452fe0(DAT_00542744,uVar2,uVar2);
    FUN_004ac220(this,0);
LAB_004dc491:
```

원문 assertion 경계:
- 이 함수에는 직접4e0620 호출이 없다. checking이 정상 동작/표시 분기를 바꾸는 부모를 그대로 복원해야 한다.
