// ==== FUN_0048f570 @ 0048f570

void __fastcall FUN_0048f570(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_00500d68;
  param_1[1] = 0xffffcfc7;
  param_1[2] = 0xffffcfc7;
  return;
}


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


// ==== FUN_0048f710 @ 0048f710

undefined4 __fastcall FUN_0048f710(int param_1)

{
  uint uVar1;
  
  uVar1 = *(int *)(param_1 + 4) * 0x24 + DAT_005395dc;
  if ((*(int *)(param_1 + 4) != 0) &&
     ((uVar1 < DAT_005395dc + 0x24U || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)))) {
    FUN_00490040(s_formSid____0____form_isValid___00531284,s__ns_o_BaseProcess_cpp_0053126c,100);
  }
  uVar1 = *(int *)(param_1 + 8) * 0x24 + DAT_005395dc;
  if ((*(int *)(param_1 + 8) != 0) &&
     ((uVar1 < DAT_005395dc + 0x24U || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)))) {
    FUN_00490040(s_attachToSid____0____s_isValid___005312bc,s__ns_o_BaseProcess_cpp_005312a4,0x67);
  }
  return 1;
}


// ==== FUN_0048f7b0 @ 0048f7b0

void __thiscall FUN_0048f7b0(int param_1,uint param_2)

{
  int *piVar1;
  
  if ((DAT_00540a2c != 0) ||
     (((4 < *(int *)(param_1 + 4) && (*(int *)(param_1 + 4) < 6000)) || ((param_2 & 0x10) != 0)))) {
    piVar1 = (int *)(*(int *)(param_1 + 4) * 0x24 + DAT_005395dc);
    if ((piVar1 < (int *)(DAT_005395dc + 0x24U)) ||
       ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= piVar1)) {
      FUN_00490040(s_s_isValid___005312f4,s__ns_o_BaseProcess_cpp_005312dc,0x93);
    }
    if ((*(byte *)((int)piVar1 + 0xb) & 2) != 0) {
      FUN_00490040(s__s_isDead___00531318,s__ns_o_BaseProcess_cpp_00531300,0x94);
    }
    (**(code **)(*piVar1 + 0x10))(param_2);
  }
  return;
}


// ==== FUN_0048f850 @ 0048f850

int __fastcall FUN_0048f850(int param_1)

{
  return DAT_005395dc + *(int *)(param_1 + 4) * 0x24;
}


// ==== FUN_0048f870 @ 0048f870

int __fastcall FUN_0048f870(int param_1)

{
  int iVar1;
  
  iVar1 = DAT_005395dc + *(int *)(param_1 + 8) * 0x24;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_00490040(s_parent_>getHead______0_0053133c,s__ns_o_BaseProcess_cpp_00531324,0x9f);
  }
  return iVar1;
}


// ==== FUN_0048f8b0 @ 0048f8b0

undefined1 __fastcall FUN_0048f8b0(int param_1)

{
  return *(undefined1 *)(*(int *)(param_1 + 4) * 0x24 + 10 + DAT_005395dc);
}


// ==== FUN_0048f8d0 @ 0048f8d0

void FUN_0048f8d0(void)

{
  undefined4 *puVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  
  puVar1 = (undefined4 *)FUN_0048f850();
  iVar2 = (**(code **)*puVar1)();
  iVar4 = DAT_005395dc + iVar2 * 0x24;
  if ((*(byte *)(iVar4 + 0xb) & 2) != 0) {
    FUN_00490040(s__parent_isDead___0053136c,s__ns_o_BaseProcess_cpp_00531354,0xab);
  }
  if ((*(byte *)(iVar4 + 0xb) & 1) != 0) {
    FUN_00490040(s__parent_isFree___00531398,s__ns_o_BaseProcess_cpp_00531380,0xac);
  }
  piVar3 = (int *)FUN_0048f850();
  (**(code **)(*piVar3 + 0x48))(0);
  piVar3 = (int *)FUN_0048f850();
  (**(code **)(*piVar3 + 0x74))(iVar2,0);
  return;
}


// ==== FUN_004ed500 @ 004ed500

int FUN_004ed500(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined1 local_100 [256];
  
  iVar1 = 1;
  iVar3 = 0;
  while( true ) {
    if (((&DAT_005b7bb0)[iVar1] == 0) && (iVar3 == 0)) {
      iVar3 = iVar1;
    }
    if (param_1 == (&DAT_005b7bb0)[iVar1]) break;
    iVar1 = iVar1 + 1;
    if (3999 < iVar1) {
      if (iVar3 == 0) {
        FUN_004ed400();
        uVar2 = FUN_00420560(local_100,s_Out_of_processes__A_process_coun_00546370);
        FUN_004875b0(uVar2);
        FUN_00490040(s_firstSlot____0_00546400,s__ns_o_Kernel_cpp_005463ec,0x77);
      }
      (&DAT_005b7bb0)[iVar3] = param_1;
      return iVar3;
    }
  }
  return iVar1;
}


// ==== FUN_004ed590 @ 004ed590

void FUN_004ed590(int param_1)

{
  int *piVar1;
  
  piVar1 = &DAT_005b7bb0 + param_1;
  if (*piVar1 == 0) {
    FUN_00490040(s_processes_pid_____NULL_00546424,s__ns_o_Kernel_cpp_00546410,0x7f);
  }
  if ((undefined4 *)*piVar1 != (undefined4 *)0x0) {
    (*(code *)**(undefined4 **)*piVar1)(1);
  }
  *piVar1 = 0;
  return;
}


// ==== FUN_004ed5d0 @ 004ed5d0

void FUN_004ed5d0(void)

{
  int *piVar1;
  
  piVar1 = &DAT_005b7bb4;
  do {
    if ((int *)*piVar1 != (int *)0x0) {
      (**(code **)(*(int *)*piVar1 + 0x18))();
    }
    piVar1 = piVar1 + 1;
  } while (piVar1 < &DAT_005bba30);
  return;
}


// ==== FUN_004ed640 @ 004ed640

undefined4 FUN_004ed640(int param_1)

{
  return (&DAT_005b7bb0)[param_1];
}


// ==== FUN_0048f0e0 @ 0048f0e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_0048f0e0(int *param_1)

{
  float fVar1;
  double dVar2;
  int iVar3;
  int *piVar4;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  float10 fVar5;
  undefined8 uVar6;
  
  uVar6 = CONCAT44(unaff_ESI,unaff_EDI);
  piVar4 = (int *)FUN_0048f870();
  iVar3 = DAT_005484ac;
  if ((*(byte *)((int)piVar4 + 0xb) & 3) == 0) {
    if ((*(byte *)((int)piVar4 + 10) == DAT_0051cb2c) &&
       (_DAT_00504660 < *(double *)(param_1 + 4) - (double)CONCAT44(DAT_005484ac,DAT_005484a8))) {
      param_1[4] = DAT_005484a8;
      param_1[5] = iVar3;
    }
    if ((*(byte *)((int)piVar4 + 0x23) & 1) != 0) {
      FUN_004abae0(uVar6);
    }
    if (((*(double *)(param_1 + 4) <= (double)CONCAT44(DAT_005484ac,DAT_005484a8)) &&
        ((*(byte *)((int)piVar4 + 0xb) & 4) == 0)) && (*param_1 != -0x22222223)) {
      fVar5 = (float10)(**(code **)(*piVar4 + 0x5c))(param_1[6],param_1[8],param_1[7]);
      fVar1 = (float)fVar5;
      if ((float10)_DAT_00504668 < fVar5) {
        if (*param_1 != -0x22222223) {
          param_1[7] = (int)fVar1;
          dVar2 = (double)CONCAT44(DAT_005484ac,DAT_005484a8);
          param_1[8] = param_1[8] + 1;
          *(double *)(param_1 + 4) = dVar2 + (double)fVar1;
        }
      }
      else if (ABS(fVar1) == 0.0) {
        FUN_0048f7b0(0);
        return;
      }
    }
  }
  return;
}


// ==== FUN_0048f430 @ 0048f430

float10 FUN_0048f430(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_0048efd0 @ 0048efd0

undefined4 * __thiscall
FUN_0048efd0(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_0048f073;
  *unaff_FS_OFFSET = &local_10;
  *param_1 = &PTR_FUN_00500d68;
  puVar1 = param_1 + 4;
  local_8 = 0;
  param_1[2] = 0;
  param_1[1] = 0;
  *puVar1 = 0;
  uVar2 = DAT_0051ca18;
  param_1[5] = 0;
  *puVar1 = 0;
  *param_1 = &PTR_LAB_00504670;
  param_1[5] = 0;
  FUN_0048f590(uVar2,param_3,0,0x50);
  param_1[7] = param_4;
  uVar2 = DAT_005484ac;
  *puVar1 = DAT_005484a8;
  param_1[5] = uVar2;
  param_1[6] = param_2;
  param_1[8] = 0;
  *unaff_FS_OFFSET = local_10;
  return param_1;
}


// ==== FUN_0048f440 @ 0048f440

undefined4 __thiscall FUN_0048f440(undefined4 param_1,byte param_2)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined4 uStack_8;
  
  uStack_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_0048f48f;
  *unaff_FS_OFFSET = &uStack_10;
  uStack_8 = 0xffffffff;
  FUN_0048f499();
  if ((param_2 & 1) != 0) {
    FUN_004f1a50(param_1);
  }
  *unaff_FS_OFFSET = uStack_10;
  return param_1;
}


// ==== FUN_0048ef50 @ 0048ef50

undefined4 * __fastcall FUN_0048ef50(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_10 [12];
  
  uVar1 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = local_10;
  *param_1 = &PTR_FUN_00500d68;
  param_1[2] = 0;
  param_1[1] = 0;
  param_1[4] = 0;
  param_1[5] = 0;
  *param_1 = &PTR_FUN_00504670;
  param_1[4] = 0;
  param_1[5] = 0;
  *unaff_FS_OFFSET = uVar1;
  return param_1;
}


// ==== FUN_004b04a0 @ 004b04a0

undefined2 __fastcall FUN_004b04a0(int param_1)

{
  return *(undefined2 *)(param_1 + 0x10);
}


// ==== FUN_004b04b0 @ 004b04b0

undefined2 __fastcall FUN_004b04b0(int param_1)

{
  return *(undefined2 *)(param_1 + 0xe);
}


// ==== FUN_004b04c0 @ 004b04c0

void __thiscall FUN_004b04c0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x10) = param_2;
  return;
}


// ==== FUN_004b04d0 @ 004b04d0

void __thiscall FUN_004b04d0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xe) = param_2;
  return;
}


// ==== FUN_004af450 @ 004af450

void __thiscall FUN_004af450(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
  (**(code **)(*piVar1 + 0xc))(param_2);
  FUN_004ed590(*(undefined4 *)(param_1 + 0x12));
  FUN_004abaa0(param_2);
  return;
}


// ==== FUN_004aba90 @ 004aba90

void FUN_004aba90(void)

{
  DAT_005395fc = DAT_005395fc + -1;
  return;
}


// ==== FUN_004af4c0 @ 004af4c0

void __thiscall FUN_004af4c0(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
  if (piVar1 != (int *)0x0) {
    (**(code **)(*piVar1 + 0x10))(param_2);
  }
  return;
}


// ==== FUN_004abab0 @ 004abab0

void FUN_004abab0(void)

{
  DAT_00539600 = DAT_00539600 + -1;
  return;
}


// ==== FUN_00401c20 @ 00401c20

void FUN_00401c20(void)

{
  return;
}


// ==== FUN_004b0470 @ 004b0470

void FUN_004b0470(void)

{
  return;
}


// ==== FUN_004b0480 @ 004b0480

void FUN_004b0480(void)

{
  return;
}


// ==== FUN_004b0490 @ 004b0490

undefined4 FUN_004b0490(void)

{
  return 1;
}


// ==== FUN_004af490 @ 004af490

void __thiscall FUN_004af490(int param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540a28 != 0) {
    piVar1 = (int *)FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_004ac460 @ 004ac460

undefined4 FUN_004ac460(void)

{
  return 10;
}


// ==== FUN_004ac470 @ 004ac470

undefined4 FUN_004ac470(void)

{
  return 1000;
}


// ==== FUN_004ac480 @ 004ac480

void FUN_004ac480(void)

{
  return;
}


// ==== FUN_00401c30 @ 00401c30

void FUN_00401c30(void)

{
  return;
}


// ==== FUN_004af270 @ 004af270

void __fastcall FUN_004af270(undefined4 *param_1)

{
  int iVar1;
  
  if (0x45 < *(byte *)((int)param_1 + 10)) {
    FUN_00490040(s_isForm___0053a4fc,s__ns_o_Squid_cpp_0053a4ec,0xa10);
  }
  if ((param_1 < (undefined4 *)(DAT_005395dc + 0x24U)) ||
     ((undefined4 *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___0053a518,s__ns_o_Squid_cpp_0053a508,0xa11);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) != 0) {
    FUN_00490040(s__isVoid___0053a534,s__ns_o_Squid_cpp_0053a524,0xa12);
  }
  *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
  iVar1 = (**(code **)*param_1)();
  if ((*(byte *)(iVar1 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) {
    FUN_004ac160();
  }
  return;
}


// ==== FUN_004abfb0 @ 004abfb0

void __thiscall FUN_004abfb0(int *param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___00539cf4,s__ns_o_Squid_cpp_00539ce4,0x352);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_00490040(s_isVoid___00539d10,s__ns_o_Squid_cpp_00539d00,0x353);
  }
  iVar2 = DAT_005395dc;
  bVar1 = *(byte *)((int)param_1 + 0xb);
  *(byte *)((int)param_1 + 0xb) = bVar1 & 0xfb;
  if ((bVar1 & 0x10) != 0) {
    FUN_00490040(s__xmitJustCreatedFlag_00539d2c,s__ns_o_Squid_cpp_00539d1c,0x356);
    iVar2 = DAT_005395dc;
  }
  if (DAT_00540a28 == 0) {
    if ((param_2 & 8) == 0) {
      param_2 = param_2 | 0x10;
    }
  }
  else {
    iVar2 = ((int)param_1 - iVar2) / 0x24;
    if ((iVar2 < 5) || (5999 < iVar2)) {
      if ((param_2 & 0x10) == 0) {
        param_2 = param_2 | 8;
      }
    }
    else {
      if ((param_2 & 8) != 0) {
        FUN_00490040(s___popFlags___pfSERVER_POP__00539d54,s__ns_o_Squid_cpp_00539d44,0x35b);
      }
      param_2 = param_2 | 0x10;
    }
    if ((param_2 & 0x50) == 0) {
      if ((param_2 & 7) != 0) {
        *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 0x10;
      }
      FUN_004abb40(0xffffffff,0xffffffff,0,param_2);
      *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xef;
    }
  }
  iVar2 = DAT_00539600;
  DAT_00539600 = DAT_00539600 + 1;
  (**(code **)(*param_1 + 0x20))(param_2);
  if (iVar2 != DAT_00539600) {
    FUN_00490040(s_curPostPop____basePostPop_00539d80,s__ns_o_Squid_cpp_00539d70,0x389);
  }
  return;
}


// ==== FUN_004ac2e0 @ 004ac2e0

void __thiscall FUN_004ac2e0(int *param_1,int param_2)

{
  int iVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  uint unaff_retaddr;
  
  if (param_2 == 0) {
    FUN_00490040(s_intoSid_00539e5c,s__ns_o_Squid_cpp_00539e4c,0x3c2);
  }
  uVar3 = FUN_004abae0();
  if ((uVar3 & 0x44200) != 0) {
    FUN_00490040(s___getGenus______gEMPLACEMENT_gVO_00539e74,s__ns_o_Squid_cpp_00539e64,0x3c3);
  }
  (**(code **)(*param_1 + 8))(param_2);
  uVar2 = *(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  iVar1 = param_2 * 0x24 + DAT_005395dc;
  if (((int)param_1 - DAT_005395dc) / 0x24 == (uint)uVar2) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  iVar4 = DAT_005395dc;
  *(ushort *)(param_1 + 1) = uVar2;
  if (uVar2 != 0) {
    (**(code **)(*(int *)((uint)uVar2 * 0x24 + iVar4) + 0xc))(((int)param_1 - iVar4) / 0x24);
    iVar4 = DAT_005395dc;
  }
  *(short *)(iVar1 + 6) = (short)(((int)param_1 - iVar4) / 0x24);
  (**(code **)(*param_1 + 0xc))(0);
  *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 8;
  FUN_004abfb0(unaff_retaddr | 0x100);
  if (*(byte *)((int)param_1 + 10) == DAT_0051c978) {
    FUN_00490040(s_getType____contentFormType_00539eb8,s__ns_o_Squid_cpp_00539ea8,0x3d2);
  }
  if (0x45 < *(byte *)((int)param_1 + 10)) {
    FUN_00490040(s_isForm___00539ee4,s__ns_o_Squid_cpp_00539ed4,0x3d5);
  }
  return;
}


// ==== FUN_004af560 @ 004af560

void __thiscall FUN_004af560(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = (**(code **)*param_1)();
  iVar1 = (iVar1 * 0x24) / 0x24;
  if ((4 < iVar1) && (iVar1 < 6000)) {
    FUN_00490040(s__getParent___isClientSid___0053a65c,s__ns_o_Squid_cpp_0053a64c,0xa79);
  }
  piVar2 = (int *)FUN_004ed640(*(undefined4 *)((int)param_1 + 0x12));
  (**(code **)(*piVar2 + 8))(param_2,0,param_3);
  return;
}


// ==== FUN_004abc10 @ 004abc10

undefined4 __thiscall
FUN_004abc10(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  
  iVar2 = FUN_00418b60(DAT_0050ff50,DAT_0050ff58,DAT_0050ff54,0,param_2);
  iVar3 = (param_1 - DAT_005395dc) / 0x24;
  if ((4 < iVar3) && (iVar3 < 6000)) {
    FUN_00490040(s__isClientSid___00539bbc,s__ns_o_Squid_cpp_00539bac,0x2ed);
  }
  uVar4 = (param_1 - DAT_005395dc) / 0x24;
  *(short *)(iVar2 + 4) = (short)uVar4;
  uVar1 = FUN_004aaa00(uVar4 & 0xffff);
  *(ushort *)(iVar2 + 6) = (ushort)((uint)param_4 >> 0x10) & 0xfff0 | uVar1;
  return 8;
}


// ==== FUN_00426850 @ 00426850

undefined4 FUN_00426850(void)

{
  FUN_00490040(&DAT_0051793c,s__ns_o_template_cpp_00517928,0x8b);
  return 0;
}


// ==== FUN_004ab650 @ 004ab650

void FUN_004ab650(void)

{
  FUN_00490040(&DAT_005399b0,s__ns_o_Squid_cpp_005399a0,0x1ff);
  return;
}


// ==== FUN_004acaf0 @ 004acaf0

void FUN_004acaf0(void)

{
  FUN_004abf50();
  FUN_00444cb0();
  return;
}


// ==== FUN_004ac490 @ 004ac490

undefined4 __thiscall FUN_004ac490(int *param_1,uint *param_2)

{
  if (DAT_005128d4 == (*param_2 & 0x3ffffff)) {
    (**(code **)(*param_1 + 0x10))(0);
    return 1;
  }
  if ((*param_2 & 0x3ffffff) == DAT_00512a38) {
    (**(code **)(*param_1 + 0x70))();
    return 1;
  }
  return 0;
}


// ==== FUN_004af5e0 @ 004af5e0

void __fastcall FUN_004af5e0(int param_1)

{
  FUN_004abe10();
  FUN_0048ca70(0x1d,s__1_Process___w_03d_n_0053a678,*(undefined4 *)(param_1 + 0x12));
  return;
}


// ==== FUN_004af380 @ 004af380

void __thiscall FUN_004af380(int param_1,uint param_2,uint param_3)

{
  int *piVar1;
  int iVar2;
  undefined4 uVar3;
  
  uVar3 = 0;
  if ((*(ushort *)(param_1 + 0x10) != param_2) && (uVar3 = 1, (param_3 & 0x80) == 0)) {
    FUN_00490040(s_popFlags___pfPROCESS_DEBUG_0053a580,s__ns_o_Squid_cpp_0053a570,0xa2e);
  }
  if (DAT_00540a28 != 0) {
    piVar1 = (int *)FUN_004af4e0();
    if ((param_3 & 0x50) == 0) {
      iVar2 = (**(code **)(*piVar1 + 8))(&DAT_0056f200,uVar3,0);
      if (iVar2 == 0) {
        FUN_00490040(s_transmitSize__0_0053a5ac,s__ns_o_Squid_cpp_0053a59c,0xa36);
      }
      if (0 < iVar2) {
        if (0x7ff < iVar2) {
          FUN_00490040(s_transmitSize_<_TRANSMIT_BUFFER_S_0053a5cc,s__ns_o_Squid_cpp_0053a5bc,0xa38)
          ;
        }
        FUN_004c0420(&DAT_0056f200);
      }
    }
    param_3 = param_3 | 0x40;
  }
  FUN_004ac2e0(param_2,param_3);
  return;
}


// ==== FUN_004af5c0 @ 004af5c0

undefined4 * FUN_004af5c0(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_FUN_00505990;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004ac890 @ 004ac890

int __thiscall FUN_004ac890(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  int local_8;
  
  FUN_004eb960((param_1 - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    uVar2 = (uint)*(byte *)(local_8 * 0x24 + 10 + DAT_005395dc);
    if ((DAT_0051ca18 == uVar2) || (DAT_0051ca54 == uVar2)) {
      FUN_004eb9f0();
      iVar1 = FUN_004af4e0();
      if (*(int *)(iVar1 + 0x18) == param_2) {
        return iVar1;
      }
    }
    FUN_004eb990();
  } while( true );
}


// ==== FUN_004eb960 @ 004eb960

void __thiscall FUN_004eb960(int param_1,int param_2)

{
  *(uint *)(param_1 + 4) = (uint)*(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  FUN_004eb990();
  return;
}


// ==== FUN_004eb9f0 @ 004eb9f0

int __fastcall FUN_004eb9f0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_004ebaa0();
  if ((iVar1 == 0) && (*(int *)(param_1 + 8) != 0)) {
    FUN_00490040(s_isProcess______sid____0_005418b8,s__ns_o_SquidFinder_cpp_005418a0,0x167);
  }
  return DAT_005395dc + *(int *)(param_1 + 8) * 0x24;
}


// ==== FUN_004eb990 @ 004eb990

void __fastcall FUN_004eb990(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  
  iVar2 = DAT_005395dc;
  do {
    do {
      iVar1 = param_1[1];
      param_1[2] = iVar1;
      if (iVar1 == 0) {
        param_1[1] = 0;
        param_1[2] = 0;
        return;
      }
      iVar1 = iVar1 * 0x24 + iVar2;
      param_1[1] = (uint)*(ushort *)(iVar1 + 4);
    } while ((iVar1 != 0) && (0x45 < *(byte *)(iVar1 + 10)));
    iVar1 = (**(code **)*param_1)(iVar1);
    iVar2 = DAT_005395dc;
  } while (iVar1 == 0);
  return;
}


// ==== FUN_004af4e0 @ 004af4e0

void __fastcall FUN_004af4e0(int param_1)

{
  FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
  return;
}


// ==== FUN_00413a10 @ 00413a10

void FUN_00413a10(void)

{
  return;
}


// ==== FUN_00413a20 @ 00413a20

void FUN_00413a20(void)

{
  return;
}


// ==== FUN_0041ea30 @ 0041ea30

undefined4 FUN_0041ea30(void)

{
  return DAT_005134c0;
}


// ==== FUN_0041f9e0 @ 0041f9e0

void FUN_0041f9e0(void)

{
  FUN_004ebaa0();
  return;
}


// ==== FUN_0041f9f0 @ 0041f9f0

void FUN_0041f9f0(void)

{
  return;
}


// ==== FUN_0048f499 @ 0048f499

void FUN_0048f499(void)

{
  FUN_0048f570();
  return;
}


// ==== FUN_004aaad0 @ 004aaad0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004aaad0(int param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  int iVar7;
  
  if (0 < param_1) {
    DAT_005395f4 = param_1;
    DAT_005395dc = (undefined4 *)FUN_0048ef10(param_1 * 0x24);
  }
  iVar2 = DAT_005395f4;
  puVar6 = DAT_005395dc;
  for (uVar4 = (uint)(DAT_005395f4 * 0x24) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  pbVar5 = (byte *)((int)DAT_005395dc + 0x53);
  for (iVar3 = 2; iVar3 < iVar2; iVar3 = iVar3 + 1) {
    *pbVar5 = *pbVar5 | 1;
    pbVar5 = pbVar5 + 0x24;
  }
  iVar3 = 5;
  DAT_005395e0 = DAT_005395dc;
  DAT_005395e8 = (int)DAT_005395dc + 0xe;
  _DAT_005395f0 = 0;
  for (iVar7 = 0xb4; puVar6 = DAT_005395dc, iVar7 < 0x34bc0; iVar7 = iVar7 + 0x24) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % 6000) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % 6000);
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  iVar3 = 6000;
  for (iVar7 = 0x34bc0; puVar6 = DAT_005395dc, iVar7 < 0x7b0c0; iVar7 = iVar7 + 0x24) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % 14000) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % 14000);
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  iVar3 = 14000;
  iVar7 = 0x7b0c0;
  while (puVar1 = DAT_005395e0, puVar6 = DAT_005395dc, iVar3 < iVar2) {
    iVar3 = iVar3 + 1;
    if (iVar7 / 0x24 == iVar3 % iVar2) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)((int)puVar6 + iVar7 + 4) = (short)(iVar3 % iVar2);
    iVar7 = iVar7 + 0x24;
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  if (((int)DAT_005395e0 - (int)DAT_005395dc) / 0x24 == 5) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(undefined2 *)(puVar1 + 1) = 5;
  iVar2 = DAT_005395e8;
  DAT_005395e4 = DAT_005395dc + 0xd2e7;
  if ((DAT_005395e8 - (int)DAT_005395dc) / 0x24 == 6000) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(undefined2 *)(iVar2 + 4) = 6000;
  DAT_005395ec = DAT_005395dc + 0x1ec27;
  if (*(short *)(DAT_005395dc + 0x1ec28) != 0) {
    FUN_00490040(s_serverFreeTail_>getNext______0_00539634,s__ns_o_Squid_cpp_00539624,0xc0);
  }
  DAT_005395c0 = 1;
  return;
}


// ==== FUN_004aae40 @ 004aae40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_004aae40(uint param_1)

{
  undefined4 *puVar1;
  byte bVar2;
  ushort uVar3;
  ushort uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  int iVar9;
  int *piVar10;
  int iVar11;
  undefined4 *puVar12;
  
  if (DAT_005395c4 != 0) {
    FUN_00490040(s_disallowSquidAllocation____0_00539680,s__ns_o_Squid_cpp_00539670,0xe8);
  }
  iVar6 = DAT_005395dc;
  if ((param_1 & 1) != 0) {
    iVar11 = DAT_005395dc + 0x7b0c0;
    if ((*(byte *)(DAT_005395dc + 0x7b0cb) & 1) == 0) {
      FUN_00490040(s_predictableHead_>isFree___005396b0,s__ns_o_Squid_cpp_005396a0,0xf0);
    }
    uVar5 = DAT_005395c0 + 14000;
    if (DAT_005395f4 <= (int)uVar5) {
      FUN_00490040(s_sid_<_numSquids_005396dc,s__ns_o_Squid_cpp_005396cc,0xf2);
    }
    puVar1 = (undefined4 *)(uVar5 * 0x24 + DAT_005395dc);
    if ((*(byte *)((int)puVar1 + 0xb) & 1) == 0) {
      FUN_00490040(s_me_>isFree___005396fc,s__ns_o_Squid_cpp_005396ec,0xf4);
    }
    if (DAT_00540a28 != 0) {
      if (*(ushort *)(puVar1 + 1) - uVar5 != 1) {
        FUN_00490040(s_me_>getNext______sid_1_0053971c,s__ns_o_Squid_cpp_0053970c,0xf7);
      }
      iVar7 = (uint)*(ushort *)(puVar1 + 1) * 0x24 + DAT_005395dc;
      if (iVar7 == 0) {
        FUN_00490040(s_checkMe_00539744,s__ns_o_Squid_cpp_00539734,0xf9);
      }
      if ((*(byte *)(iVar7 + 0xb) & 1) == 0) {
        FUN_00490040(s_checkMe_>isFree___0053975c,s__ns_o_Squid_cpp_0053974c,0xfa);
      }
      uVar3 = *(ushort *)(puVar1 + 1);
      if ((iVar11 - DAT_005395dc) / 0x24 == (uint)uVar3) {
        FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
      }
      *(ushort *)(iVar6 + 0x7b0c4) = uVar3;
    }
    puVar12 = puVar1;
    for (iVar6 = 9; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar12 = 0;
      puVar12 = puVar12 + 1;
    }
    bVar2 = *(byte *)((int)puVar1 + 0xb);
    _DAT_005395f0 = _DAT_005395f0 + -1;
    DAT_005395c0 = DAT_005395c0 + 1;
    *(byte *)((int)puVar1 + 0xb) = bVar2 | 4;
    *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfe | 4;
    *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfc | 4;
    return uVar5;
  }
  param_1 = param_1 & 2;
  if (param_1 == 0) {
    if (DAT_00540a28 == 0) {
      FUN_00490040(s_server_00539780,s__ns_o_Squid_cpp_00539770,0x10a);
    }
    piVar10 = &DAT_005395e8;
    piVar8 = &DAT_005395ec;
  }
  else {
    piVar10 = &DAT_005395e0;
    piVar8 = &DAT_005395e4;
  }
  if ((uint)*(ushort *)(*piVar10 + 4) * 0x24 + DAT_005395dc == *piVar8) {
    if (param_1 != 0) {
      FUN_0041f3e0();
      FUN_004875b0(s_alCLIENT__Send_in__NetStorm_squi_00539788);
      FUN_00490040(&DAT_005397d0,s__ns_o_Squid_cpp_005397c0,0x118);
    }
    if (DAT_00540a2c == 0) {
      FUN_0041f3e0();
      FUN_004875b0(s__Server__Send_in__NetStorm_squid_005397e4);
      FUN_00490040(&DAT_0053982c,s__ns_o_Squid_cpp_0053981c,300);
    }
    else {
      iVar11 = 0x32;
      iVar6 = 2;
      if (2 < DAT_005395f4) {
        do {
          if ((*(byte *)(iVar6 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) break;
          iVar6 = iVar6 + 1;
        } while (iVar6 < DAT_005395f4);
      }
      iVar7 = DAT_005395dc;
      iVar9 = DAT_005395f4;
      do {
        piVar8 = (int *)(iVar6 * 0x24 + iVar7);
        if ((piVar8 < (int *)(iVar7 + 0x24U)) || ((int *)(iVar9 * 0x24 + iVar7) <= piVar8)) break;
        if (*(byte *)((int)piVar8 + 10) == DAT_0051ca8c) {
          iVar11 = iVar11 + -1;
          (**(code **)(*piVar8 + 0x10))(0);
          iVar7 = DAT_005395dc;
          iVar9 = DAT_005395f4;
        }
      } while (iVar11 != 0);
      FUN_004c3dc0(s_BridgesKilled_005397d4);
    }
    if (param_1 == 0) {
      if (DAT_00540a28 == 0) {
        FUN_00490040(s_server_00539840,s__ns_o_Squid_cpp_00539830,0x130);
      }
      piVar10 = &DAT_005395e8;
      piVar8 = &DAT_005395ec;
    }
    else {
      piVar10 = &DAT_005395e0;
      piVar8 = &DAT_005395e4;
    }
  }
  uVar3 = *(ushort *)(*piVar10 + 4);
  puVar1 = (undefined4 *)((uint)uVar3 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)puVar1 + 0xb) & 1) == 0) {
    FUN_00490040(s_p_>isFree___00539858,s__ns_o_Squid_cpp_00539848,0x140);
  }
  if ((undefined4 *)*piVar8 == puVar1) {
    FUN_00490040(s_p_____freeTail_00539874,s__ns_o_Squid_cpp_00539864,0x141);
  }
  iVar6 = *piVar10;
  uVar4 = *(ushort *)(puVar1 + 1);
  if ((iVar6 - DAT_005395dc) / 0x24 == (uint)uVar4) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  *(ushort *)(iVar6 + 4) = uVar4;
  puVar12 = puVar1;
  for (iVar6 = 9; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar12 = 0;
    puVar12 = puVar12 + 1;
  }
  bVar2 = *(byte *)((int)puVar1 + 0xb);
  _DAT_005395f0 = _DAT_005395f0 + -1;
  *(byte *)((int)puVar1 + 0xb) = bVar2 | 4;
  *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfe | 4;
  *(byte *)((int)puVar1 + 0xb) = bVar2 & 0xfc | 4;
  return (uint)uVar3;
}


// ==== FUN_004ab250 @ 004ab250

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004ab250(int param_1)

{
  undefined1 uVar1;
  byte bVar2;
  int iVar3;
  undefined4 *puVar4;
  int *piVar5;
  undefined4 *puVar6;
  
  puVar4 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if ((*(byte *)((int)puVar4 + 0xb) & 4) == 0) {
    FUN_00490040(s_p_>isVoid___00539894,s__ns_o_Squid_cpp_00539884,0x153);
  }
  if ((*(byte *)((int)puVar4 + 0xb) & 1) != 0) {
    FUN_00490040(s__p_>isFree___005398b0,s__ns_o_Squid_cpp_005398a0,0x154);
  }
  FUN_004cff40(param_1,*(undefined1 *)((int)puVar4 + 10));
  uVar1 = *(undefined1 *)((int)puVar4 + 10);
  puVar6 = puVar4;
  for (iVar3 = 9; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  *(undefined1 *)((int)puVar4 + 10) = uVar1;
  if ((DAT_00540a28 == 0) && (5999 < param_1)) {
    *(byte *)((int)puVar4 + 0xb) = *(byte *)((int)puVar4 + 0xb) | 1;
    return;
  }
  if ((param_1 < 5) || (piVar5 = &DAT_005395e4, 5999 < param_1)) {
    piVar5 = &DAT_005395ec;
  }
  if ((4 < param_1) && (param_1 < 14000)) {
    iVar3 = *piVar5;
    if ((iVar3 - DAT_005395dc) / 0x24 == param_1) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(short *)(iVar3 + 4) = (short)param_1;
    iVar3 = (int)puVar4 - DAT_005395dc;
    *piVar5 = (int)puVar4;
    if (iVar3 / 0x24 == 0) {
      FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
    }
    *(undefined2 *)(puVar4 + 1) = 0;
    _DAT_005395f0 = _DAT_005395f0 + 1;
  }
  bVar2 = *(byte *)((int)puVar4 + 0xb);
  *(byte *)((int)puVar4 + 0xb) = bVar2 | 1;
  *(byte *)((int)puVar4 + 0xb) = bVar2 | 3;
  return;
}


// ==== FUN_004ab390 @ 004ab390

int * FUN_004ab390(int param_1,uint param_2)

{
  code *pcVar1;
  int iVar2;
  int *piVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  iVar2 = FUN_004aae40(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar2 < 14000) goto LAB_004ab3e2;
    uVar6 = 0x18b;
    pcVar5 = s__ns_o_Squid_cpp_005398f0;
    pcVar4 = s_sid_<_FIRST_PREDICTABLE_SID_00539900;
  }
  else {
    if (13999 < iVar2) goto LAB_004ab3e2;
    uVar6 = 0x188;
    pcVar5 = s__ns_o_Squid_cpp_005398c0;
    pcVar4 = s_sid_>__FIRST_PREDICTABLE_SID_005398d0;
  }
  FUN_00490040(pcVar4,pcVar5,uVar6);
LAB_004ab3e2:
  pcVar1 = *(code **)(param_1 * 0x1d4 + 0x1b0 + DAT_0051c960);
  if (pcVar1 == (code *)0x0) {
    piVar3 = (int *)(DAT_005395dc + iVar2 * 0x24);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar3 = (int *)(*pcVar1)(iVar2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  (**(code **)(*piVar3 + 0x28))();
  return piVar3;
}


// ==== FUN_004ab7e0 @ 004ab7e0

void __thiscall FUN_004ab7e0(int *param_1,uint param_2)

{
  int *piVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint unaff_retaddr;
  
  bVar2 = *(byte *)((int)param_1 + 0xb);
  if (((bVar2 & 2) == 0) &&
     (*(byte *)((int)param_1 + 0xb) = bVar2 | 2, iVar4 = DAT_005395f8, (bVar2 & 1) == 0)) {
    if ((DAT_00540a28 != 0) &&
       ((iVar3 = ((int)param_1 - DAT_005395dc) / 0x24, 4 < iVar3 && (iVar3 < 6000)))) {
      param_2 = param_2 | 0x10;
    }
    if (((DAT_00540a28 != 0) || ((param_2 & 8) != 0)) ||
       ((iVar3 = ((int)param_1 - DAT_005395dc) / 0x24, 4 < iVar3 && (iVar3 < 6000)))) {
      DAT_005395f8 = DAT_005395f8 + 1;
      iVar3 = FUN_004abf50(((int)param_1 - DAT_005395dc) / 0x24);
      FUN_0048ca70(0x27,s_destroying__s__d_00539a60,iVar3 + 4);
      (**(code **)(*param_1 + 0x14))(param_2);
      if (iVar4 != DAT_005395f8) {
        FUN_00490040(s_baseDestroy____curDestroy_00539a84,s__ns_o_Squid_cpp_00539a74,0x26d);
      }
      iVar4 = (int)param_1 - DAT_005395dc;
      uVar5 = (uint)*(ushort *)((int)param_1 + 6);
      while (uVar5 != 0) {
        piVar1 = (int *)(uVar5 * 0x24 + DAT_005395dc);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
          FUN_00490040(s__s_isFree___00539ab0,s__ns_o_Squid_cpp_00539aa0,0x274);
        }
        if (((*(byte *)((int)piVar1 + 0xb) & 8) == 0) && (0x45 < *(byte *)((int)piVar1 + 10))) {
          FUN_00490040(s_s_isContained______s_isForm___00539acc,s__ns_o_Squid_cpp_00539abc,0x275);
        }
        uVar5 = (uint)*(ushort *)(piVar1 + 1);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
          FUN_00490040(s__s_isFree___00539afc,s__ns_o_Squid_cpp_00539aec,0x277);
        }
        (**(code **)(*piVar1 + 0x1c))(iVar4 / 0x24);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) == 0) {
          (**(code **)(*piVar1 + 0x10))(unaff_retaddr | 0x40);
        }
      }
      if ((*(byte *)((int)param_1 + 0xb) & 1) == 0) {
        if ((((unaff_retaddr & 0x50) == 0) && (DAT_00540a28 != 0)) &&
           ((iVar4 = ((int)param_1 - DAT_005395dc) / 0x24, iVar4 < 5 || (5999 < iVar4)))) {
          FUN_004abb40(0xffffffff,0xfffffffe,0,unaff_retaddr | 8);
        }
        iVar4 = (int)param_1 - DAT_005395dc;
        iVar3 = FUN_0041ea30();
        if (iVar4 / 0x24 == iVar3) {
          FUN_0041ed70();
        }
        if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
           ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
          FUN_00490040(s_isValid___00539b18,s__ns_o_Squid_cpp_00539b08,0x299);
        }
        if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
          (**(code **)(*param_1 + 0x48))(0);
        }
        iVar4 = DAT_005395fc;
        DAT_005395fc = DAT_005395fc + 1;
        (**(code **)(*param_1 + 0x18))(unaff_retaddr);
        if (iVar4 != DAT_005395fc) {
          FUN_00490040(s_curPostDestroy____basePostDestro_00539b34,s__ns_o_Squid_cpp_00539b24,0x2a0)
          ;
        }
        FUN_004ab250(((int)param_1 - DAT_005395dc) / 0x24);
      }
    }
  }
  return;
}


// ==== FUN_004abaa0 @ 004abaa0

void FUN_004abaa0(void)

{
  DAT_005395f8 = DAT_005395f8 + -1;
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004ac160 @ 004ac160

void __fastcall FUN_004ac160(int *param_1)

{
  int *piVar1;
  ushort uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  
  if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
    FUN_00490040(s_containedFlag_00539dc8,s__ns_o_Squid_cpp_00539db8,0x39a);
  }
  iVar3 = (**(code **)*param_1)();
  if (iVar3 == 0) {
    FUN_00490040(s_parentSid_00539de8,s__ns_o_Squid_cpp_00539dd8,0x39c);
  }
  piVar1 = (int *)(iVar3 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
    FUN_00490040(s__p_isFree___00539e04,s__ns_o_Squid_cpp_00539df4,0x39f);
  }
  if ((*(byte *)((int)piVar1 + 0xb) & 2) == 0) {
    uVar4 = ((int)param_1 - DAT_005395dc) / 0x24;
    if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
      FUN_00490040(s_containedFlag_00539e20,s__ns_o_Squid_cpp_00539e10,0x3a8);
    }
    uVar2 = *(ushort *)(param_1 + 1);
    uVar6 = (uint)uVar2;
    iVar3 = (**(code **)(*param_1 + 4))();
    if (iVar3 != 0) {
      iVar5 = iVar3 * 0x24 + DAT_005395dc;
      if ((iVar5 - DAT_005395dc) / 0x24 == uVar6) {
        FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
      }
      *(ushort *)(iVar5 + 4) = uVar2;
    }
    if (uVar6 != 0) {
      (**(code **)(*(int *)(DAT_005395dc + uVar6 * 0x24) + 0xc))(iVar3);
    }
    if (*(ushort *)((int)piVar1 + 6) == uVar4) {
      if (iVar3 != 0) {
        FUN_00490040(s_prev____0_00539e40,s__ns_o_Squid_cpp_00539e30,0x3b7);
      }
      *(ushort *)((int)piVar1 + 6) = uVar2;
    }
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xf7;
    if ((0x45 < *(byte *)((int)param_1 + 10)) || (*(byte *)((int)param_1 + 10) == DAT_0051c978)) {
      (**(code **)(*piVar1 + 0x24))(uVar4,2);
    }
  }
  return;
}


// ==== FUN_004ac9c0 @ 004ac9c0

int __thiscall FUN_004ac9c0(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int local_8;
  
  FUN_004eb960((param_1 - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    uVar1 = (uint)*(byte *)(local_8 * 0x24 + 10 + DAT_005395dc);
    if ((DAT_0051ca18 == uVar1) || (DAT_0051ca54 == uVar1)) {
      FUN_004eb9f0();
      iVar2 = FUN_004af4e0();
      if ((*(uint *)(iVar2 + 0x18) & param_2) == param_3) {
        return iVar2;
      }
    }
    FUN_004eb990();
  } while( true );
}


// ==== FUN_004af4f0 @ 004af4f0

void __thiscall FUN_004af4f0(int param_1,int param_2)

{
  if ((param_2 < 0) || (0xfe < param_2)) {
    FUN_00490040(s_0_<__pType_&&_pType_<_MAX_TYPES_0053a600,s__ns_o_Squid_cpp_0053a5f0,0xa6e);
  }
  if (*(byte *)(param_1 + 10) != DAT_0051c968) {
    FUN_00490040(s_typeNum____processFormType_0053a630,s__ns_o_Squid_cpp_0053a620,0xa6f);
  }
  *(char *)(param_1 + 10) = (char)param_2;
  return;
}


// ==== FUN_004cff40 @ 004cff40

void FUN_004cff40(int param_1,int param_2)

{
  int *_Src;
  
  if ((param_1 < 5) || (_Src = &DAT_005890b0, 5999 < param_1)) {
    _Src = &DAT_00589010;
  }
  FID_conflict__memcpy(_Src + 2,_Src,0x98);
  *_Src = param_1;
  _Src[1] = param_2;
  return;
}


// ==== FUN_004ebaa0 @ 004ebaa0

undefined4 __fastcall FUN_004ebaa0(int param_1)

{
  byte bVar1;
  
  bVar1 = *(byte *)(*(int *)(param_1 + 8) * 0x24 + 10 + DAT_005395dc);
  if ((9 < bVar1) && ((int)(uint)bVar1 < DAT_0051ca58)) {
    return 1;
  }
  return 0;
}


// ==== FID_conflict:_memcpy @ 004f1a60

/* Library Function - Multiple Matches With Different Base Names
    _memcpy
    _memmove
   
   Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release */

void * __cdecl FID_conflict__memcpy(void *_Dst,void *_Src,size_t _Size)

{
  uint uVar1;
  int in_EDX;
  uint uVar2;
  undefined4 *puVar3;
  undefined1 *puVar4;
  undefined4 *puVar5;
  undefined1 *puVar6;
  
  if ((_Src < _Dst) && (_Dst < (void *)((int)_Src + _Size))) {
    puVar3 = (undefined4 *)((int)_Src + _Size);
    puVar5 = (undefined4 *)((int)_Dst + _Size);
    if (((uint)puVar5 & 3) == 0) {
      uVar1 = _Size >> 2;
      while( true ) {
        puVar5 = puVar5 + -1;
        puVar3 = puVar3 + -1;
        if (uVar1 == 0) break;
        uVar1 = uVar1 - 1;
        *puVar5 = *puVar3;
      }
      switch(_Size & 3) {
      case 1:
switchD_004f1b29_caseD_1:
        *(undefined1 *)((int)puVar5 + 3) = *(undefined1 *)((int)puVar3 + 3);
        return _Dst;
      case 2:
switchD_004f1b29_caseD_2:
        *(undefined2 *)((int)puVar5 + 2) = *(undefined2 *)((int)puVar3 + 2);
        return _Dst;
      case 3:
switchD_004f1b29_caseD_3:
        *(undefined2 *)((int)puVar5 + 2) = *(undefined2 *)((int)puVar3 + 2);
        *(undefined1 *)((int)puVar5 + 1) = *(undefined1 *)((int)puVar3 + 1);
        return _Dst;
      }
    }
    else {
      puVar4 = (undefined1 *)((int)puVar3 + -1);
      puVar6 = (undefined1 *)((int)puVar5 + -1);
      if (_Size < 0xd) {
        for (; _Size != 0; _Size = _Size - 1) {
          *puVar6 = *puVar4;
          puVar4 = puVar4 + -1;
          puVar6 = puVar6 + -1;
        }
        return _Dst;
      }
      uVar2 = -in_EDX & 3;
      uVar1 = _Size - uVar2;
      for (; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar6 = *puVar4;
        puVar4 = puVar4 + -1;
        puVar6 = puVar6 + -1;
      }
      puVar3 = (undefined4 *)(puVar4 + -3);
      puVar5 = (undefined4 *)(puVar6 + -3);
      for (uVar2 = uVar1 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar5 = *puVar3;
        puVar3 = puVar3 + -1;
        puVar5 = puVar5 + -1;
      }
      switch(uVar1 & 3) {
      case 1:
        goto switchD_004f1b29_caseD_1;
      case 2:
        goto switchD_004f1b29_caseD_2;
      case 3:
        goto switchD_004f1b29_caseD_3;
      }
    }
    return _Dst;
  }
  puVar3 = _Dst;
  if (((uint)_Dst & 3) == 0) {
    for (uVar1 = _Size >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar3 = *(undefined4 *)_Src;
      _Src = (undefined4 *)((int)_Src + 4);
      puVar3 = puVar3 + 1;
    }
    switch(_Size & 3) {
    case 1:
switchD_004f1a90_caseD_1:
      *(undefined1 *)puVar3 = *(undefined1 *)_Src;
      return _Dst;
    case 2:
switchD_004f1a90_caseD_2:
      *(undefined2 *)puVar3 = *(undefined2 *)_Src;
      return _Dst;
    case 3:
switchD_004f1a90_caseD_3:
      *(undefined2 *)puVar3 = *(undefined2 *)_Src;
      *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)_Src + 2);
      return _Dst;
    }
  }
  else {
    puVar4 = _Dst;
    if (_Size < 0xd) {
      for (; _Size != 0; _Size = _Size - 1) {
        *puVar4 = *(undefined1 *)_Src;
        _Src = (undefined1 *)((int)_Src + 1);
        puVar4 = puVar4 + 1;
      }
      return _Dst;
    }
    uVar2 = -(int)_Dst & 3;
    uVar1 = _Size - uVar2;
    for (; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined1 *)puVar3 = *(undefined1 *)_Src;
      _Src = (undefined4 *)((int)_Src + 1);
      puVar3 = (undefined4 *)((int)puVar3 + 1);
    }
    for (uVar2 = uVar1 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *puVar3 = *(undefined4 *)_Src;
      _Src = (undefined4 *)((int)_Src + 4);
      puVar3 = puVar3 + 1;
    }
    switch(uVar1 & 3) {
    case 1:
      goto switchD_004f1a90_caseD_1;
    case 2:
      goto switchD_004f1a90_caseD_2;
    case 3:
      goto switchD_004f1a90_caseD_3;
    }
  }
  return _Dst;
}


