// ==== FUN_0041bd20 @ 0041bd20

int __thiscall FUN_0041bd20(int *param_1,int param_2,int param_3,int param_4,uint param_5)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int *piVar4;
  
  if ((param_2 < 10) || (DAT_0054116c <= param_2)) {
    FUN_004e0620("isProcessType(_processType)","\\Ns\\O\\Baseprocess.cpp",0x1c);
  }
  param_1[2] = param_3;
  param_1[1] = param_4;
  iVar1 = param_3 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_3 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
    iVar2 = FUN_00471950(param_1);
    iVar1 = param_1[1];
    if (iVar1 == 0) {
      uVar3 = 0;
      if ((param_5 & 0x10) != 0) {
        uVar3 = 2;
      }
      piVar4 = (int *)FUN_004af530(DAT_00541078,uVar3);
      param_1[1] = ((int)piVar4 - DAT_005c8464) / 0x32;
      FUN_004ae2e0(param_2);
      *(int *)((int)piVar4 + 0x12) = iVar2;
      (**(code **)(*piVar4 + 0x74))(param_1[2],param_5 | 0x80);
      iVar1 = (**(code **)(*param_1 + 0x14))();
      return iVar1;
    }
    piVar4 = (int *)(iVar1 * 0x32 + DAT_005c8464);
    if ((*(byte *)(iVar1 * 0x32 + 0xb + DAT_005c8464) & 1) != 0) {
      FUN_004af610(DAT_00541078,iVar1);
      *(char *)((int)piVar4 + 10) = (char)param_2;
      *(int *)((int)piVar4 + 0x12) = iVar2;
      (**(code **)(*piVar4 + 0x74))(param_1[2],param_5 | 0x80);
    }
    iVar1 = (**(code **)*piVar4)();
    if (iVar1 != param_1[2]) {
      FUN_004e0620("s.getParentSid() == attachToSid","\\Ns\\O\\Baseprocess.cpp",0x5e);
    }
    if (*(int *)((int)piVar4 + 0x12) != iVar2) {
      FUN_004e0620("s.getPid() == pid","\\Ns\\O\\Baseprocess.cpp",0x5f);
    }
    iVar1 = (**(code **)(*param_1 + 0x14))();
  }
  return iVar1;
}


// ==== FUN_004af610 @ 004af610

int * FUN_004af610(uint param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  undefined1 local_100 [256];
  
  if ((param_2 < 15000) || (DAT_005c847c <= param_2)) {
    FUN_004e0620("(FIRST_SERVER_SID <= sid && sid < numSquids)","\\Ns\\O\\Squid.cpp",0x1a7);
  }
  piVar3 = (int *)(param_2 * 0x32 + DAT_005c8464);
  if ((*(byte *)((int)piVar3 + 0xb) & 1) == 0) {
    if (*(byte *)((int)piVar3 + 10) != param_1) {
      iVar1 = FUN_0049a840((uint)*(byte *)((int)piVar3 + 10));
      iVar1 = FUN_0049a840(param_1,iVar1 + 4);
      uVar2 = FUN_004a89c0(local_100,"Invalid take. Server type %s, client type %s",iVar1 + 4);
      FUN_00436d20(uVar2);
      FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Squid.cpp",0x1ba);
    }
    (**(code **)(*piVar3 + 0x48))(0);
  }
  *(byte *)((int)piVar3 + 0xb) = *(byte *)((int)piVar3 + 0xb) & 0xfc;
  iVar1 = FUN_0049a840(param_1);
  if (*(code **)(iVar1 + 0x1d0) == (code *)0x0) {
    piVar3 = (int *)(param_2 * 0x32 + DAT_005c8464);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_00501dd0;
    }
  }
  else {
    piVar3 = (int *)(**(code **)(iVar1 + 0x1d0))(param_2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  *(byte *)((int)piVar3 + 0xb) = *(byte *)((int)piVar3 + 0xb) | 4;
  (**(code **)(*piVar3 + 0x2c))();
  return piVar3;
}


// ==== FUN_004ae3d0 @ 004ae3d0

undefined4 * FUN_004ae3d0(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00512098;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004ded50 @ 004ded50

void FUN_004ded50(int param_1,undefined4 param_2)

{
  *(undefined4 *)(DAT_005e4778 + param_1 * 4) = param_2;
  return;
}


// ==== FUN_004ded70 @ 004ded70

void FUN_004ded70(int param_1,undefined4 param_2)

{
  *(undefined4 *)(DAT_005e477c + param_1 * 4) = param_2;
  return;
}


// ==== FUN_004ded90 @ 004ded90

void FUN_004ded90(int param_1,undefined1 param_2,undefined1 param_3)

{
  *(undefined1 *)(DAT_005e4780 + param_1 * 2) = param_2;
  *(undefined1 *)(DAT_005e4780 + 1 + param_1 * 2) = param_3;
  return;
}


// ==== FUN_004df240 @ 004df240

undefined4 __fastcall FUN_004df240(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  bVar1 = *(byte *)(param_1 + 1);
  if ((bVar1 == 0) || (uVar2 = (uint)bVar1, DAT_0052ec38 < (int)uVar2)) {
    _printf("Packet of type %d failed type validation\n",(uint)bVar1);
    return 0;
  }
  if ((9 < bVar1) && (*(int *)(DAT_005e4778 + uVar2 * 4) == 0)) {
    _printf("Packet of type %d failed handler validation\n",uVar2);
    return 0;
  }
  if ((*(short *)(&DAT_0052ec40 + uVar2 * 2) != -1) &&
     (*(short *)(&DAT_0052ec40 + uVar2 * 2) != *(short *)(param_1 + 2))) {
    _printf("Packet of type %d failed length validation\n",uVar2);
    return 0;
  }
  return 1;
}


// ==== FUN_004df740 @ 004df740

undefined4 __fastcall FUN_004df740(void *param_1)

{
  byte bVar1;
  code *pcVar2;
  int iVar3;
  uint uVar4;
  undefined4 uVar5;
  byte bVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  byte bVar10;
  byte *pbVar11;
  uint local_18;
  int local_8;
  
  if ((*(byte *)((int)param_1 + 0x34) & 0x40) != 0) {
    return 0;
  }
  pbVar11 = *(byte **)((int)param_1 + 0x60);
  uVar7 = (uint)pbVar11[1];
  uVar8 = (uint)*(ushort *)(pbVar11 + 2);
  bVar10 = *pbVar11 & 0x7f;
  iVar3 = FUN_004df240();
  if (iVar3 == 0) goto LAB_004df9af;
  if (uVar7 == 4) {
    FUN_0045ad00("[%p] %02d: buffered dispatch len=%d\n",param_1,
                 *(undefined4 *)((int)param_1 + 0x40),
                 *(undefined2 *)(*(int *)((int)param_1 + 0x60) + 2));
    local_8 = -1;
    local_18 = 4;
    if (4 < uVar8) {
      local_18 = 4;
      do {
        iVar3 = *(int *)((int)param_1 + 0x60);
        uVar4 = (uint)*(ushort *)(iVar3 + 2 + local_18);
        uVar9 = (uint)*(byte *)(iVar3 + 1 + local_18);
        pbVar11 = (byte *)(iVar3 + local_18);
        bVar6 = *pbVar11 & 0x7f;
        FUN_0045ad00("[%p] %02d: buffered packet disp type=%d len=%d\n",param_1,
                     *(undefined4 *)((int)param_1 + 0x40),uVar9,uVar4);
        local_18 = local_18 + uVar4;
        if ((int)uVar8 < (int)local_18) break;
        iVar3 = FUN_004df240();
        if (iVar3 != 0) {
          uVar5 = FUN_004e49c0();
          if (uVar9 == 6) {
            *(undefined4 *)((int)param_1 + 0x48) = uVar5;
          }
          else {
            *(undefined4 *)((int)param_1 + 0x44) = uVar5;
          }
          bVar1 = *(byte *)(DAT_005e4780 + uVar9 * 2);
          if (bVar6 != bVar1) {
            if (bVar6 < bVar1) {
              bVar6 = *(byte *)(DAT_005e4780 + 1 + uVar9 * 2);
              if ((bVar6 & 8) != 0) goto LAB_004df943;
              bVar6 = bVar6 & 2;
            }
            else {
              if (bVar6 <= bVar1) goto LAB_004df86c;
              bVar6 = *(byte *)(DAT_005e4780 + 1 + uVar9 * 2);
              if ((bVar6 & 4) != 0) goto LAB_004df943;
              bVar6 = bVar6 & 1;
            }
            if (bVar6 != 0) goto LAB_004df8bf;
          }
LAB_004df86c:
          FUN_004dec30(0,*(undefined4 *)((int)param_1 + 0x50),*(undefined4 *)((int)param_1 + 0x40),
                       pbVar11,*(undefined2 *)(pbVar11 + 2));
          pcVar2 = *(code **)(DAT_005e477c + uVar9 * 4);
          if (pcVar2 != (code *)0x0) {
            (*pcVar2)(0);
          }
          iVar3 = FUN_004df010(*(undefined4 *)(DAT_005e4778 + uVar9 * 4),
                               *(undefined4 *)((int)param_1 + 0x40),pbVar11,local_8);
          if (-1 < iVar3) {
            local_8 = iVar3;
          }
        }
LAB_004df8bf:
      } while ((int)local_18 < (int)uVar8);
    }
    if (local_18 != uVar8) goto LAB_004df943;
    goto LAB_004df9af;
  }
  bVar6 = *(byte *)(DAT_005e4780 + uVar7 * 2);
  if (bVar10 != bVar6) {
    if (bVar10 < bVar6) {
      bVar6 = *(byte *)(DAT_005e4780 + 1 + uVar7 * 2);
      if ((bVar6 & 8) != 0) {
LAB_004df943:
        uVar5 = FUN_004e0940(*(undefined4 *)((int)param_1 + 0x50),bVar10,
                             *(undefined1 *)(DAT_005e4780 + uVar7 * 2));
        _printf("version mismatch %s, incoming = %d, should be %d\n",uVar5);
        FUN_004df9f0();
        return 0;
      }
      bVar6 = bVar6 & 2;
    }
    else {
      if (bVar10 <= bVar6) goto LAB_004df926;
      bVar6 = *(byte *)(DAT_005e4780 + 1 + uVar7 * 2);
      if ((bVar6 & 4) != 0) goto LAB_004df943;
      bVar6 = bVar6 & 1;
    }
    if (bVar6 != 0) goto LAB_004df9af;
  }
LAB_004df926:
  uVar5 = FUN_004e49c0();
  if (uVar7 == 6) {
    *(undefined4 *)((int)param_1 + 0x48) = uVar5;
  }
  else {
    *(undefined4 *)((int)param_1 + 0x44) = uVar5;
  }
  iVar3 = *(int *)((int)param_1 + 0x60);
  if (9 < uVar7) {
    FUN_004dec30(0,*(undefined4 *)((int)param_1 + 0x50),*(undefined4 *)((int)param_1 + 0x40),iVar3,
                 *(undefined2 *)(iVar3 + 2));
  }
  FUN_004df010(*(undefined4 *)(DAT_005e4778 + uVar7 * 4),*(undefined4 *)((int)param_1 + 0x40),iVar3,
               0xffffffff);
LAB_004df9af:
  if ((*(int *)((int)param_1 + 0x8c) == 0) && ((*(byte *)((int)param_1 + 0x34) & 0x40) != 0)) {
    FUN_004e03d0();
    _free(param_1);
    return 0;
  }
  return 1;
}


// ==== FUN_004df010 @ 004df010

int __thiscall
FUN_004df010(int param_1,code *param_2,undefined4 param_3,undefined4 param_4,int param_5)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  void *pvVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004fc2ab;
  local_c = ExceptionList;
  if (param_2 == (code *)0x0) {
    return 0;
  }
  if (*(int *)(param_1 + 0x7c) == 0) {
    ExceptionList = &local_c;
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + 1;
    (*param_2)(param_3,param_1,param_4);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + -1;
    param_5 = 0;
  }
  else {
    ExceptionList = &local_c;
    iVar2 = _rand();
    iVar2 = iVar2 % *(int *)(param_1 + 0x78);
    iVar6 = (int)**(char **)(param_1 + 0x74);
    iVar3 = 0;
    if (iVar6 < iVar2) {
      do {
        pcVar1 = (char *)(*(int *)(param_1 + 0x74) + 1 + iVar3);
        iVar3 = iVar3 + 1;
        iVar6 = iVar6 + *pcVar1;
      } while (iVar6 < iVar2);
    }
    iVar2 = *(int *)(param_1 + 0x88);
    if (*(int *)(param_1 + 0x84) < iVar2) {
      iVar2 = *(int *)(param_1 + 0x80) - iVar2;
    }
    else {
      iVar2 = -iVar2;
    }
    iVar6 = *(int *)(param_1 + 0x80);
    if (iVar6 - (*(int *)(param_1 + 0x84) + iVar2) < 2) {
      pvVar4 = _realloc(*(void **)(param_1 + 0x7c),iVar6 << 3);
      *(void **)(param_1 + 0x7c) = pvVar4;
      puVar8 = (undefined4 *)(*(uint *)(param_1 + 0x80) * 4 + (int)pvVar4);
      for (uVar7 = *(uint *)(param_1 + 0x80) & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
        *(undefined1 *)puVar8 = 0;
        puVar8 = (undefined4 *)((int)puVar8 + 1);
      }
      iVar2 = *(int *)(param_1 + 0x84);
      if (iVar2 < *(int *)(param_1 + 0x88)) {
        puVar8 = *(undefined4 **)(param_1 + 0x7c);
        puVar9 = *(undefined4 **)(param_1 + 0x7c) + *(int *)(param_1 + 0x80);
        for (; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar9 = *puVar8;
          puVar8 = puVar8 + 1;
          puVar9 = puVar9 + 1;
        }
        puVar8 = *(undefined4 **)(param_1 + 0x7c);
        for (uVar7 = *(uint *)(param_1 + 0x84) & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
          *puVar8 = 0;
          puVar8 = puVar8 + 1;
        }
        for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
          *(undefined1 *)puVar8 = 0;
          puVar8 = (undefined4 *)((int)puVar8 + 1);
        }
        *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x84) + *(int *)(param_1 + 0x80);
      }
      *(int *)(param_1 + 0x80) = iVar6 * 2;
    }
    if (param_5 == -1) {
      iVar2 = _rand();
      param_5 = iVar2 % 100 + iVar3 * 100;
    }
    pvVar4 = operator_new(0x30);
    local_4 = 0;
    if (pvVar4 == (void *)0x0) {
      uVar5 = 0;
    }
    else {
      uVar5 = FUN_004dee60(param_3,param_2,param_4,param_5,*(undefined4 *)(param_1 + 0x4c),
                           *(undefined4 *)(param_1 + 0x50),*(undefined4 *)(param_1 + 0x54),
                           *(undefined4 *)(param_1 + 0x58),*(undefined4 *)(param_1 + 0x5c),param_1);
    }
    *(undefined4 *)(*(int *)(param_1 + 0x7c) + *(int *)(param_1 + 0x84) * 4) = uVar5;
    *(int *)(param_1 + 0x84) = (*(int *)(param_1 + 0x84) + 1) % *(int *)(param_1 + 0x80);
  }
  ExceptionList = local_c;
  return param_5;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


