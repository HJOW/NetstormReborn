// ==== FUN_0045c6c0 @ 0045c6c0

undefined4 * __thiscall FUN_0045c6c0(undefined4 *param_1,int param_2,undefined4 param_3)

{
  bool bVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  int *piVar10;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_0045c825;
  *unaff_FS_OFFSET = &local_10;
  *param_1 = &PTR_FUN_00500d68;
  param_1[2] = 0;
  param_1[1] = 0;
  local_8 = 0;
  param_1[6] = 0;
  param_1[5] = 0;
  param_1[4] = 0;
  uVar8 = DAT_005209c4;
  param_1[3] = 0;
  *param_1 = &PTR_LAB_00502468;
  bVar1 = false;
  piVar6 = DAT_005209c0;
  do {
    iVar4 = DAT_005395dc;
    iVar9 = 0;
    piVar5 = piVar6;
    if (0 < (int)uVar8) {
      do {
        if (*piVar5 == 0) {
          bVar1 = true;
          break;
        }
        iVar9 = iVar9 + 1;
        piVar5 = piVar5 + 1;
      } while (iVar9 < (int)uVar8);
    }
    DAT_005209c0 = piVar6;
    if (bVar1) {
      piVar6[iVar9] = (int)param_1;
      param_1[7] = iVar9;
      param_1[8] = (int)*(char *)(param_1[2] * 0x24 + 0x21 + iVar4);
      if (param_2 != 0) {
        FUN_0048f590(param_3,param_2,0,0x10);
        param_1[8] = (int)*(char *)(param_2 * 0x24 + 0x21 + DAT_005395dc);
      }
      *unaff_FS_OFFSET = local_10;
      return param_1;
    }
    DAT_005209c4 = uVar8 + 0x28;
    if ((int)DAT_005209c4 < 0x29) {
      DAT_005209c4 = 0x28;
    }
    piVar6 = (int *)FUN_004f1650(DAT_005209c4 << 2);
    FUN_0048ef00();
    uVar3 = DAT_005209c4;
    piVar5 = piVar6;
    for (uVar7 = DAT_005209c4 & 0x3fffffff; piVar2 = DAT_005209c0, uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar5 = 0;
      piVar5 = piVar5 + 1;
    }
    piVar5 = DAT_005209c0;
    piVar10 = piVar6;
    for (uVar8 = uVar8 & 0x3fffffff; uVar8 != 0; uVar8 = uVar8 - 1) {
      *piVar10 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar10 = piVar10 + 1;
    }
    uVar8 = uVar3;
    if (piVar2 != (int *)0x0) {
      FUN_004f1a50(piVar2);
      uVar8 = DAT_005209c4;
    }
  } while( true );
}


// ==== FUN_0045c840 @ 0045c840

void __fastcall FUN_0045c840(int *param_1)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_0045c8ed;
  *param_1 = (int)&PTR_LAB_00502468;
  *unaff_FS_OFFSET = &local_10;
  local_8 = 0;
  if (*param_1 != -0x22222223) {
    if ((param_1[7] < 0) || (DAT_005209c4 <= param_1[7])) {
      FUN_00490040(s_0_<__id_&&_id_<_totalSystemsAllo_005209d8,s_Particle_cpp_005209c8,0x8b);
    }
    if (*(int **)(DAT_005209c0 + param_1[7] * 4) != param_1) {
      FUN_00490040(s_systems_id_____this_00520a10,s_Particle_cpp_00520a00,0x8c);
    }
    *(undefined4 *)(DAT_005209c0 + param_1[7] * 4) = 0;
  }
  local_8 = 0xffffffff;
  FUN_0045c8f7();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0045c8f7 @ 0045c8f7

void FUN_0045c8f7(void)

{
  FUN_0048f570();
  return;
}


// ==== FUN_0048f570 @ 0048f570

void __fastcall FUN_0048f570(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_00500d68;
  param_1[1] = 0xffffcfc7;
  param_1[2] = 0xffffcfc7;
  return;
}


// ==== FUN_004ea360 @ 004ea360

void __fastcall FUN_004ea360(undefined4 *param_1)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  uint local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_004ea3f6;
  *param_1 = &PTR_FUN_005073e0;
  *unaff_FS_OFFSET = &local_10;
  local_8 = 1;
  if (param_1[0x13] != 0) {
    FUN_004f1a50(param_1[0x13]);
  }
  if (param_1[0x14] != 0) {
    FUN_004f1a50(param_1[0x14]);
  }
  local_8 = local_8 & 0xffffff00;
  FUN_004ea3d0();
  local_8 = 0xffffffff;
  FUN_004ea400();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004ea3d0 @ 004ea3d0

void FUN_004ea3d0(void)

{
  int unaff_EBP;
  
  if (*(int *)(unaff_EBP + -0x10) != 0) {
    *(int *)(unaff_EBP + -0x14) = *(int *)(unaff_EBP + -0x10) + 0x24;
    FUN_004ead00();
    return;
  }
  *(undefined4 *)(unaff_EBP + -0x14) = 0;
  FUN_004ead00();
  return;
}


// ==== FUN_004ea400 @ 004ea400

void FUN_004ea400(void)

{
  FUN_0045c840();
  return;
}


// ==== FUN_004ead00 @ 004ead00

void __fastcall FUN_004ead00(int param_1)

{
  if (*(int *)(param_1 + 8) != 0) {
    FUN_004f1a50(*(int *)(param_1 + 8));
  }
  return;
}


// ==== FUN_004ead20 @ 004ead20

undefined4 __thiscall FUN_004ead20(undefined4 param_1,byte param_2)

{
  FUN_004ea360();
  if ((param_2 & 1) != 0) {
    FUN_004f1a50(param_1);
  }
  return param_1;
}


// ==== FUN_004ead40 @ 004ead40

undefined4 __thiscall FUN_004ead40(undefined4 param_1,byte param_2)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined4 uStack_8;
  
  uStack_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_004ead8f;
  *unaff_FS_OFFSET = &uStack_10;
  uStack_8 = 0xffffffff;
  FUN_004ead99();
  if ((param_2 & 1) != 0) {
    FUN_004f1a50(param_1);
  }
  *unaff_FS_OFFSET = uStack_10;
  return param_1;
}


// ==== FUN_004ead99 @ 004ead99

void FUN_004ead99(void)

{
  FUN_004ea360();
  return;
}


// ==== FUN_004eadb0 @ 004eadb0

undefined4 __thiscall FUN_004eadb0(undefined4 param_1,byte param_2)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined4 uStack_8;
  
  uStack_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_004eadff;
  *unaff_FS_OFFSET = &uStack_10;
  uStack_8 = 0xffffffff;
  FUN_004eae09();
  if ((param_2 & 1) != 0) {
    FUN_004f1a50(param_1);
  }
  *unaff_FS_OFFSET = uStack_10;
  return param_1;
}


// ==== FUN_004eae09 @ 004eae09

void FUN_004eae09(void)

{
  FUN_004ea360();
  return;
}


