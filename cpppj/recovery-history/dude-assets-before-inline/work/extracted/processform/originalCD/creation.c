// ==== FUN_0048f590 @ 0048f590

void __thiscall FUN_0048f590(int *param_1,int param_2,int param_3,int param_4,uint param_5)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  int iVar4;
  
  if ((param_2 < 10) || (DAT_0051ca58 <= param_2)) {
    FUN_00490040(s_isProcessType__processType__0053119c,s__ns_o_BaseProcess_cpp_00531184,0x1c);
  }
  iVar4 = DAT_005395dc;
  param_1[2] = param_3;
  param_1[1] = param_4;
  iVar4 = iVar4 + param_3 * 0x24;
  if ((*(byte *)(iVar4 + 0xb) & 1) != 0) {
    FUN_00490040(s__base_isFree___005311d0,s__ns_o_BaseProcess_cpp_005311b8,0x38);
  }
  if ((*(byte *)(iVar4 + 0xb) & 2) != 0) {
    FUN_00490040(s__base_isDead___005311f8,s__ns_o_BaseProcess_cpp_005311e0,0x39);
  }
  iVar1 = FUN_004ed500(param_1);
  iVar4 = param_1[1];
  if (iVar4 == 0) {
    uVar2 = 0;
    if ((param_5 & 0x10) != 0) {
      uVar2 = 2;
    }
    piVar3 = (int *)FUN_004ab390(DAT_0051c968,uVar2);
    param_1[1] = ((int)piVar3 - DAT_005395dc) / 0x24;
    FUN_004af4f0(param_2);
    iVar4 = param_1[2];
    *(int *)((int)piVar3 + 0x12) = iVar1;
    (**(code **)(*piVar3 + 0x74))(iVar4,param_5 | 0x80);
  }
  else {
    piVar3 = (int *)(DAT_005395dc + iVar4 * 0x24);
    if ((*(byte *)((int)piVar3 + 0xb) & 1) != 0) {
      FUN_004ab440(DAT_0051c968,iVar4);
      *(undefined1 *)((int)piVar3 + 10) = (undefined1)param_2;
      iVar4 = param_1[2];
      *(int *)((int)piVar3 + 0x12) = iVar1;
      (**(code **)(*piVar3 + 0x74))(iVar4,param_5 | 0x80);
    }
    iVar4 = (**(code **)*piVar3)();
    if (iVar4 != param_1[2]) {
      FUN_00490040(s_s_getParentSid______attachToSid_00531220,s__ns_o_BaseProcess_cpp_00531208,0x59)
      ;
    }
    if (*(int *)((int)piVar3 + 0x12) != iVar1) {
      FUN_00490040(s_s_getPid______pid_00531258,s__ns_o_BaseProcess_cpp_00531240,0x5a);
    }
  }
  (**(code **)(*param_1 + 0x14))();
  return;
}


// ==== FUN_004ab440 @ 004ab440

int * FUN_004ab440(uint param_1,int param_2)

{
  byte bVar1;
  code *pcVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int *piVar6;
  undefined1 local_100 [256];
  
  if ((param_2 < 6000) || (DAT_005395f4 <= param_2)) {
    FUN_00490040(s__FIRST_SERVER_SID_<__sid_&&_sid_<_0053992c,s__ns_o_Squid_cpp_0053991c,0x19e);
  }
  piVar6 = (int *)(param_2 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)piVar6 + 0xb) & 1) == 0) {
    if (param_1 != *(byte *)((int)piVar6 + 10)) {
      uVar5 = FUN_00420560(local_100,s_Invalid_take__Server_type__s__cl_0053995c,
                           param_1 * 0x1d4 + DAT_0051c960 + 4,
                           (uint)*(byte *)((int)piVar6 + 10) * 0x1d4 + DAT_0051c960 + 4);
      FUN_004875b0(uVar5);
      FUN_00490040(&DAT_0053999c,s__ns_o_Squid_cpp_0053998c,0x1b1);
    }
    (**(code **)(*piVar6 + 0x48))(0);
  }
  iVar4 = DAT_005395dc;
  iVar3 = DAT_0051c960;
  bVar1 = *(byte *)((int)piVar6 + 0xb);
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfe;
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfc;
  pcVar2 = *(code **)(param_1 * 0x1d4 + 0x1b0 + iVar3);
  if (pcVar2 == (code *)0x0) {
    piVar6 = (int *)(param_2 * 0x24 + iVar4);
    if (piVar6 == (int *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      *piVar6 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar6 = (int *)(*pcVar2)(param_2);
  }
  *(byte *)((int)piVar6 + 0xb) = *(byte *)((int)piVar6 + 0xb) | 4;
  *(char *)((int)piVar6 + 10) = (char)param_1;
  (**(code **)(*piVar6 + 0x2c))();
  return piVar6;
}


// ==== FUN_004af5c0 @ 004af5c0

undefined4 * FUN_004af5c0(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00505990;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_00418bb0 @ 00418bb0

void FUN_00418bb0(int param_1,undefined4 param_2)

{
  *(undefined4 *)(DAT_00512424 + param_1 * 4) = param_2;
  return;
}


// ==== FUN_00418bd0 @ 00418bd0

void FUN_00418bd0(int param_1,undefined4 param_2)

{
  *(undefined4 *)(DAT_00512428 + param_1 * 4) = param_2;
  return;
}


// ==== FUN_00418bf0 @ 00418bf0

void FUN_00418bf0(int param_1,undefined1 param_2,undefined1 param_3)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)(DAT_0051242c + param_1 * 2);
  *puVar1 = param_2;
  puVar1[1] = param_3;
  return;
}


// ==== FUN_004190a0 @ 004190a0

int __fastcall FUN_004190a0(int param_1)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  
  bVar1 = *(byte *)(param_1 + 1);
  if ((bVar1 == 0) || (uVar2 = (uint)bVar1, DAT_0050fcf8 < (int)uVar2)) {
    FID_conflict__wprintf(s_Packet_of_type__d_failed_type_va_00512474,(uint)bVar1);
    return 0;
  }
  if ((9 < bVar1) && (*(int *)(DAT_00512424 + uVar2 * 4) == 0)) {
    FID_conflict__wprintf(s_Packet_of_type__d_failed_handler_005124a0,uVar2);
    return 0;
  }
  if ((*(short *)(&DAT_0050fd00 + uVar2 * 2) == -1) ||
     (iVar3 = 0, *(short *)(param_1 + 2) == *(short *)(&DAT_0050fd00 + uVar2 * 2))) {
    iVar3 = 1;
  }
  if (iVar3 == 0) {
    FID_conflict__wprintf(s_Packet_of_type__d_failed_length_v_005124d0,uVar2);
  }
  return iVar3;
}


// ==== FUN_00419130 @ 00419130

undefined4 __fastcall FUN_00419130(int param_1)

{
  byte *pbVar1;
  byte bVar2;
  ushort uVar3;
  code *pcVar4;
  byte bVar5;
  byte bVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  undefined4 uVar10;
  uint uVar11;
  byte *pbVar12;
  uint local_14;
  int local_4;
  
  if ((*(byte *)(param_1 + 0x34) & 0x40) != 0) {
    return 0;
  }
  pbVar12 = *(byte **)(param_1 + 0x60);
  bVar5 = *pbVar12 & 0x7f;
  uVar7 = (uint)pbVar12[1];
  uVar8 = (uint)*(ushort *)(pbVar12 + 2);
  iVar9 = FUN_004190a0();
  if (iVar9 == 0) goto LAB_004191a7;
  if (uVar7 == 4) {
    FUN_0047ca50(s___p___02d__buffered_dispatch_len_005124fc,param_1,*(undefined4 *)(param_1 + 0x40)
                 ,*(undefined2 *)(*(int *)(param_1 + 0x60) + 2));
    local_4 = -1;
    local_14 = 4;
    if (4 < uVar8) {
      do {
        pbVar12 = (byte *)(*(int *)(param_1 + 0x60) + local_14);
        uVar11 = (uint)pbVar12[1];
        uVar3 = *(ushort *)(pbVar12 + 2);
        bVar6 = *pbVar12 & 0x7f;
        FUN_0047ca50(s___p___02d__buffered_packet_disp_t_00512524,param_1,
                     *(undefined4 *)(param_1 + 0x40),uVar11,(uint)uVar3);
        local_14 = local_14 + uVar3;
        if ((int)uVar8 < (int)local_14) break;
        iVar9 = FUN_004190a0();
        if (iVar9 != 0) {
          if (uVar11 == 6) {
            uVar10 = __ftol();
            *(undefined4 *)(param_1 + 0x48) = uVar10;
          }
          else {
            uVar10 = __ftol();
            *(undefined4 *)(param_1 + 0x44) = uVar10;
          }
          pbVar1 = (byte *)(DAT_0051242c + uVar11 * 2);
          bVar2 = *pbVar1;
          if (bVar2 != bVar6) {
            if (bVar6 < bVar2) {
              if ((pbVar1[1] & 8) != 0) goto LAB_004193a5;
              bVar6 = pbVar1[1] & 2;
            }
            else {
              if (bVar6 <= bVar2) goto LAB_00419333;
              if ((pbVar1[1] & 4) != 0) goto LAB_004193a5;
              bVar6 = pbVar1[1] & 1;
            }
            if (bVar6 != 0) goto LAB_00419389;
          }
LAB_00419333:
          FUN_00418a30(0,*(undefined4 *)(param_1 + 0x50),*(undefined4 *)(param_1 + 0x40),pbVar12,
                       *(undefined2 *)(pbVar12 + 2));
          pcVar4 = *(code **)(DAT_00512428 + uVar11 * 4);
          if (pcVar4 != (code *)0x0) {
            (*pcVar4)(0);
          }
          iVar9 = FUN_00418d40(*(undefined4 *)(DAT_00512424 + uVar11 * 4),
                               *(undefined4 *)(param_1 + 0x40),pbVar12,local_4);
          if (-1 < iVar9) {
            local_4 = iVar9;
          }
        }
LAB_00419389:
      } while ((int)local_14 < (int)uVar8);
    }
    if (local_14 != uVar8) goto LAB_004193a5;
    goto LAB_004191a7;
  }
  pbVar12 = (byte *)(DAT_0051242c + uVar7 * 2);
  bVar6 = *pbVar12;
  if (bVar5 != bVar6) {
    if (bVar5 < bVar6) {
      if ((pbVar12[1] & 8) != 0) {
LAB_004193a5:
        uVar10 = FUN_004a6c80(*(undefined4 *)(param_1 + 0x50),bVar5,
                              *(undefined1 *)(DAT_0051242c + uVar7 * 2));
        FID_conflict__wprintf(s_version_mismatch__s__incoming_____00512554,uVar10);
        FUN_00419400();
        return 0;
      }
      bVar5 = pbVar12[1] & 2;
    }
    else {
      if (bVar5 <= bVar6) goto LAB_004191f1;
      if ((pbVar12[1] & 4) != 0) goto LAB_004193a5;
      bVar5 = pbVar12[1] & 1;
    }
    if (bVar5 != 0) goto LAB_004191a7;
  }
LAB_004191f1:
  if (uVar7 == 6) {
    uVar10 = __ftol();
    *(undefined4 *)(param_1 + 0x48) = uVar10;
  }
  else {
    uVar10 = __ftol();
    *(undefined4 *)(param_1 + 0x44) = uVar10;
  }
  iVar9 = *(int *)(param_1 + 0x60);
  if (9 < uVar7) {
    FUN_00418a30(0,*(undefined4 *)(param_1 + 0x50),*(undefined4 *)(param_1 + 0x40),iVar9,
                 *(undefined2 *)(iVar9 + 2));
  }
  FUN_00418d40(*(undefined4 *)(DAT_00512424 + uVar7 * 4),*(undefined4 *)(param_1 + 0x40),iVar9,
               0xffffffff);
LAB_004191a7:
  if ((*(int *)(param_1 + 0x8c) == 0) && ((*(byte *)(param_1 + 0x34) & 0x40) != 0)) {
    if (param_1 != 0) {
      FUN_00419880();
      FUN_004f1a50(param_1);
    }
    return 0;
  }
  return 1;
}


// ==== FUN_00418d40 @ 00418d40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall
FUN_00418d40(int param_1,code *param_2,undefined4 param_3,undefined4 param_4,int param_5)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  double *pdVar8;
  undefined4 *puVar9;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_34 [5];
  undefined8 local_20;
  int local_18;
  undefined4 *local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00418f7a;
  *unaff_FS_OFFSET = &local_10;
  if (param_2 == (code *)0x0) {
    local_18 = 0;
  }
  else if (*(int *)(param_1 + 0x7c) == 0) {
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + 1;
    (*param_2)(param_3,param_1,param_4);
    *(int *)(param_1 + 0x8c) = *(int *)(param_1 + 0x8c) + -1;
    local_18 = 0;
  }
  else {
    iVar1 = _rand();
    local_20._4_4_ = 0;
    iVar2 = 0;
    while (iVar2 = iVar2 + *(char *)(local_20._4_4_ + *(int *)(param_1 + 0x74)),
          iVar2 < iVar1 % *(int *)(param_1 + 0x78)) {
      local_20._4_4_ = local_20._4_4_ + 1;
    }
    local_20._4_4_ = local_20._4_4_ * 100;
    iVar1 = *(int *)(param_1 + 0x88);
    iVar2 = *(int *)(param_1 + 0x84);
    if (iVar2 < iVar1) {
      iVar2 = (iVar2 - iVar1) + *(int *)(param_1 + 0x80);
    }
    else {
      iVar2 = iVar2 - iVar1;
    }
    iVar1 = *(int *)(param_1 + 0x80);
    if (iVar1 - iVar2 < 2) {
      local_18 = iVar1 * 2;
      iVar1 = FUN_0048ef30(*(undefined4 *)(param_1 + 0x7c),iVar1 << 3);
      *(int *)(param_1 + 0x7c) = iVar1;
      puVar3 = (undefined4 *)(*(uint *)(param_1 + 0x80) * 4 + iVar1);
      puVar7 = puVar3;
      for (uVar5 = *(uint *)(param_1 + 0x80) & 0x3fffffff; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar7 = 0;
        puVar7 = puVar7 + 1;
      }
      for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
        *(undefined1 *)puVar7 = 0;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      uVar5 = *(uint *)(param_1 + 0x84);
      if ((int)uVar5 < *(int *)(param_1 + 0x88)) {
        puVar7 = *(undefined4 **)(param_1 + 0x7c);
        for (uVar6 = uVar5 & 0x3fffffff; uVar6 != 0; uVar6 = uVar6 - 1) {
          *puVar3 = *puVar7;
          puVar7 = puVar7 + 1;
          puVar3 = puVar3 + 1;
        }
        for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
          *(undefined1 *)puVar3 = *(undefined1 *)puVar7;
          puVar7 = (undefined4 *)((int)puVar7 + 1);
          puVar3 = (undefined4 *)((int)puVar3 + 1);
        }
        puVar3 = *(undefined4 **)(param_1 + 0x7c);
        for (uVar5 = uVar5 & 0x3fffffff; uVar5 != 0; uVar5 = uVar5 - 1) {
          *puVar3 = 0;
          puVar3 = puVar3 + 1;
        }
        for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
          *(undefined1 *)puVar3 = 0;
          puVar3 = (undefined4 *)((int)puVar3 + 1);
        }
        *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x80) + *(int *)(param_1 + 0x84);
      }
      *(int *)(param_1 + 0x80) = local_18;
    }
    if (param_5 == -1) {
      iVar1 = _rand();
      param_5 = local_20._4_4_ + iVar1 % 100;
    }
    local_18 = param_5;
    puVar3 = (undefined4 *)FUN_004f1650(0x30);
    local_14 = (undefined4 *)0x0;
    local_8 = 0;
    if (puVar3 != (undefined4 *)0x0) {
      puVar7 = (undefined4 *)(param_1 + 0x4c);
      puVar9 = local_34;
      local_14 = puVar3;
      for (iVar1 = 5; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar9 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar9 = puVar9 + 1;
      }
      pdVar8 = (double *)(local_14 + 4);
      puVar7 = local_14 + 6;
      *(undefined4 *)pdVar8 = 0;
      local_14[5] = 0;
      FUN_004a7030(s_0_0_0_0_00512838,0);
      *local_14 = param_3;
      local_14[1] = param_2;
      uVar4 = FUN_00418af0(param_4,0,0);
      local_20 = (double)local_18 * _DAT_00500cc0;
      local_14[2] = uVar4;
      local_20 = local_20 + _DAT_00548490;
      *pdVar8 = local_20;
      puVar3 = local_34;
      for (iVar1 = 5; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar7 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar7 = puVar7 + 1;
      }
      local_14[0xb] = param_1;
    }
    iVar1 = *(int *)(param_1 + 0x84);
    *(undefined4 **)(*(int *)(param_1 + 0x7c) + iVar1 * 4) = local_14;
    *(int *)(param_1 + 0x84) = (iVar1 + 1) % *(int *)(param_1 + 0x80);
  }
  *unaff_FS_OFFSET = local_10;
  return local_18;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_004accc0 @ 004accc0

void __thiscall FUN_004accc0(int param_1,int param_2)

{
  uint uVar1;
  
  uVar1 = *(uint *)(param_1 + 0xe) & 0xffff;
  *(uint *)(param_1 + 0xe) = uVar1;
  *(uint *)(param_1 + 0xe) = param_2 << 0x10 | uVar1;
  return;
}


