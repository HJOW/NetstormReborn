// ==== FUN_004be8e0 @ 004be8e0

void __thiscall FUN_004be8e0(void *this)

{
  byte bVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  int iVar4;
  int *piVar5;
  uint uVar6;
  
  if (*(int *)this == 0) {
    FUN_004e0620(&DAT_0051476c,"\\Ns\\O\\Template.cpp",0x423);
  }
  piVar5 = (int *)FUN_0044ceb0(*(undefined4 *)this,"Badges",1);
  if ((*piVar5 != 0) && (piVar5[2] != 0)) {
    bVar1 = *(byte *)(piVar5[1] + *piVar5);
    piVar5[1] = piVar5[1] + 1;
    for (uVar6 = (uint)bVar1; uVar6 != 0; uVar6 = uVar6 - 1) {
      iVar4 = piVar5[1];
      uVar2 = *(undefined1 *)(iVar4 + *piVar5);
      piVar5[1] = iVar4 + 1;
      uVar3 = *(undefined1 *)(iVar4 + 1 + *piVar5);
      piVar5[1] = iVar4 + 2;
      FUN_0047d800(uVar2,uVar3);
    }
  }
  return;
}


// ==== FUN_0047d800 @ 0047d800

void __cdecl FUN_0047d800(int param_1,int param_2)

{
  int *piVar1;
  int iVar2;
  
  if ((param_2 < 0) || (0xff < param_2)) {
    FUN_004e0620("count >=0 && count < 256",".\\Metadisplay.cpp",0x165b);
  }
  iVar2 = 0;
  piVar1 = &DAT_00593160;
  do {
    if (*piVar1 == param_1) {
      if (iVar2 != -1) {
        (&DAT_00593238)[iVar2 * 0x37] = param_2;
      }
      return;
    }
    piVar1 = piVar1 + 0x37;
    iVar2 = iVar2 + 1;
  } while ((int)piVar1 < 0x594970);
  return;
}


// ==== FUN_004beee0 @ 004beee0

void __thiscall FUN_004beee0(void *this)

{
  undefined4 uVar1;
  int *piVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb888;
  local_c = ExceptionList;
  DAT_005c89b8 = DAT_005c89b8 + 1;
  local_4 = 0;
  ExceptionList = &local_c;
  if (*(int *)this == 0) {
    ExceptionList = &local_c;
    FUN_004e0620(&DAT_0051476c,"\\Ns\\O\\Template.cpp",0x4b5);
  }
  if (this != &DAT_005c89f0 + DAT_00540cac * 5) {
    FUN_004e0620("isMine()","\\Ns\\O\\Template.cpp",0x4b6);
  }
  uVar1 = FUN_0048fc60();
  piVar2 = (int *)FUN_0044ceb0(*(undefined4 *)this,"Technology",1);
  if ((*piVar2 != 0) && (piVar2[2] != 0)) {
    FUN_004bd800(this);
    FUN_004bd130(uVar1,piVar2,0x11,1);
    if (DAT_005c89bc != (void *)0x0) {
      _free(DAT_005c89bc);
    }
    DAT_005c89bc = (void *)0x0;
  }
  FUN_0042f540();
  DAT_005c89b8 = DAT_005c89b8 + -1;
  ExceptionList = local_c;
  return;
}


// ==== FUN_004bd800 @ 004bd800

void __thiscall FUN_004bd800(void *this)

{
  char *pcVar1;
  char cVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  byte bVar10;
  undefined4 *puVar11;
  int iVar12;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb828;
  local_c = ExceptionList;
  DAT_005c89b8 = DAT_005c89b8 + 1;
  iVar8 = 0;
  local_4 = 0;
  ExceptionList = &local_c;
  piVar4 = (int *)FUN_0044ceb0(*(undefined4 *)this,"TypeNames",1);
  if ((*piVar4 == 0) || (piVar4[2] == 0)) {
    DAT_005c89bc = (undefined4 *)0x0;
  }
  else {
    bVar10 = *(byte *)(piVar4[1] + *piVar4);
    piVar4[1] = piVar4[1] + 1;
    uVar9 = (uint)bVar10;
    if (DAT_005c89bc != (undefined4 *)0x0) {
      FUN_004e0620("!typeConversion","\\Ns\\O\\Template.cpp",0x260);
    }
    DAT_005c89bc = operator_new(0xff);
    puVar11 = DAT_005c89bc;
    for (iVar7 = 0x3f; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar11 = 0;
      puVar11 = puVar11 + 1;
    }
    *(undefined2 *)puVar11 = 0;
    *(undefined1 *)((int)puVar11 + 2) = 0;
    iVar7 = piVar4[1];
    iVar3 = *piVar4;
    if (piVar4[2] < iVar7) {
      piVar4[2] = iVar7;
    }
    if (0 < DAT_00541348) {
      do {
        iVar5 = FUN_0049a840(iVar8);
        iVar6 = iVar5 + 4;
        iVar12 = 0;
        bVar10 = 0;
        cVar2 = *(char *)(iVar5 + 4);
        while (cVar2 != '\0') {
          iVar12 = iVar12 + ((int)cVar2 << ((bVar10 & 3) << 3));
          pcVar1 = (char *)(iVar6 + 1);
          iVar6 = iVar6 + 1;
          bVar10 = bVar10 + 1;
          cVar2 = *pcVar1;
        }
        iVar6 = 0;
        if (uVar9 != 0) {
          do {
            if (*(int *)(iVar3 + iVar7 + iVar6 * 4) == iVar12) {
              if (iVar6 < (int)uVar9) {
                *(char *)((int)DAT_005c89bc + iVar6) = (char)iVar8;
              }
              break;
            }
            iVar6 = iVar6 + 1;
          } while (iVar6 < (int)uVar9);
        }
        iVar8 = iVar8 + 1;
      } while (iVar8 < DAT_00541348);
    }
  }
  DAT_005c89b8 = DAT_005c89b8 + -1;
  ExceptionList = local_c;
  return;
}


