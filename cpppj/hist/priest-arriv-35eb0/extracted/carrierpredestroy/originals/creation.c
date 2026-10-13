// ==== FUN_00426890 @ 00426890

void FUN_00426890(undefined4 param_1)

{
  int iVar1;
  
  FUN_0044b4b0(param_1);
  if (DAT_00540bc4 != 0) {
    iVar1 = FUN_004ac9f0(1,0,0);
    if (iVar1 != 0) {
      FUN_00485e40();
    }
  }
  return;
}


// ==== FUN_004ac9f0 @ 004ac9f0

int __thiscall FUN_004ac9f0(int param_1,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  int local_8;
  
  FUN_004b2270((param_1 - DAT_005c8464) / 0x32);
  do {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(DAT_005c8464 + 0x12 + local_8 * 0x32) != param_3)) {
      bVar1 = false;
    }
    if (((param_2 == 0) || ((param_2 & *(ushort *)(DAT_005c8464 + 0x14 + local_8 * 0x32)) != 0)) &&
       (bVar1)) {
      if (param_4 == 0) {
        return (local_8 * 0x32) / 0x32;
      }
      param_4 = param_4 + -1;
    }
    FUN_004b1b20();
  } while( true );
}


// ==== FUN_004b2270 @ 004b2270

void __thiscall FUN_004b2270(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((DAT_005e4794 != 0) && ((*(byte *)(iVar1 + 0xb) & 2) != 0)) && (DAT_00540bc4 == 0)) &&
     (*(int *)(param_1 + 0xc) == 0)) {
    FUN_004e0620("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",
                 "\\Ns\\O\\Squidfinder.cpp",0x13c);
  }
  *(uint *)(param_1 + 4) = (uint)*(ushort *)(iVar1 + 6);
  FUN_004b1b20();
  return;
}


// ==== FUN_004b1b20 @ 004b1b20

void __fastcall FUN_004b1b20(undefined4 *param_1)

{
  int iVar1;
  
  do {
    do {
      iVar1 = param_1[1];
      param_1[2] = iVar1;
      if (iVar1 == 0) {
        param_1[1] = 0;
        param_1[2] = 0;
        return;
      }
      param_1[1] = (uint)*(ushort *)(iVar1 * 0x32 + 4 + DAT_005c8464);
      iVar1 = param_1[2] * 0x32 + DAT_005c8464;
    } while ((iVar1 != 0) && (*(byte *)(iVar1 + 10) != DAT_00541088));
    iVar1 = (**(code **)*param_1)(iVar1);
  } while (iVar1 == 0);
  return;
}


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
}


// ==== FUN_0044b4b0 @ 0044b4b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0044b4b0(int param_1,uint param_2)

{
  undefined2 uVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  float *pfVar7;
  int *piVar8;
  float fVar9;
  undefined4 unaff_ESI;
  float fVar10;
  float local_20;
  float local_1c;
  undefined1 local_18 [8];
  undefined **local_10;
  int local_8;
  undefined4 local_4;
  
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    if ((param_2 & 0x200000) != 0) {
      FUN_004605f0((param_1 - DAT_005c8464) / 0x32);
      FUN_004a9d70(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),"collapse.wav",0);
    }
    if ((param_2 & 0x100000) != 0) {
      FUN_004605d0((param_1 - DAT_005c8464) / 0x32);
      FUN_004a9d70(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),"explosion.wav",0)
      ;
    }
    local_1c = *(float *)(param_1 + 0x12);
    local_20 = *(float *)(param_1 + 0xe);
    iVar4 = FUN_004e49c0();
    iVar5 = FUN_004e49c0();
    if ((*(byte *)(iVar5 + iVar4 * 0x100 + DAT_005c7c44) & 6) != 0) {
      local_10 = &PTR_FUN_00500ff4;
      local_4 = 1;
      FUN_004b2270((param_1 - DAT_005c8464) / 0x32);
      while (local_8 != 0) {
        if (*(ushort *)(local_8 * 0x32 + 0x12 + DAT_005c8464) == DAT_005412d0) {
          FUN_004a9cb0("priestFree.wav",0,0,0,1,0);
        }
        FUN_004b1b20();
      }
    }
    if ((DAT_00540bc4 != 0) && ((param_2 & 0x800) == 0)) {
      iVar4 = FUN_004ac550();
      if (*(int *)(iVar4 + 0x1d4) == 1) {
        local_20 = *(float *)(param_1 + 0xe);
        local_1c = *(float *)(param_1 + 0x12);
        pfVar7 = &local_20;
      }
      else {
        pfVar7 = (float *)FUN_004adea0(local_18);
      }
      fVar2 = *pfVar7;
      fVar3 = pfVar7[1];
      FUN_0041d7a0();
      local_10 = &PTR_FUN_00500ff4;
      local_4 = 1;
      FUN_004b2270((param_1 - DAT_005c8464) / 0x32);
      while (local_8 != 0) {
        iVar4 = local_8 * 0x32 + DAT_005c8464;
        if (((*(byte *)(iVar4 + 0x14) & 0x20) != 0) &&
           ((uVar1 = *(undefined2 *)(iVar4 + 0x12), DAT_00594fbc != 0 ||
            (iVar4 = FUN_0049a840(uVar1), (DAT_00542640 & *(uint *)(iVar4 + 0xec)) != 0)))) {
          iVar4 = FUN_0049a840(uVar1);
          if ((*(uint *)(iVar4 + 0xec) & 0x200000) == 0) {
            iVar4 = FUN_004e49c0();
            iVar5 = FUN_004e49c0();
            if (((*(byte *)(iVar5 + iVar4 * 0x100 + DAT_005c7c44) & 0x10) == 0) &&
               (uVar6 = FUN_0040d9d0(fVar2,fVar3), (uVar6 & 6) != 0)) {
              piVar8 = (int *)FUN_004af530(uVar1,0);
              FUN_004ac220(*(undefined2 *)(local_8 * 0x32 + 0xc + DAT_005c8464));
              (**(code **)(*piVar8 + 0x74))(0);
              (**(code **)(*piVar8 + 0x90))(unaff_ESI,fVar2,0);
            }
          }
          else {
            fVar10 = *(float *)(param_1 + 0xe);
            fVar9 = *(float *)(param_1 + 0x12);
            uVar6 = FUN_004ac200();
            if ((uVar6 & 0x80400000) != 0) {
              pfVar7 = (float *)FUN_004adea0(local_18);
              fVar10 = *pfVar7 + _DAT_00500ee4;
              fVar9 = pfVar7[1] + _DAT_00500ee4;
              local_20 = fVar10;
              local_1c = fVar9;
            }
            FUN_0044b2e0(fVar10,fVar9,uVar1,*(undefined2 *)(local_8 * 0x32 + 0xc + DAT_005c8464));
          }
        }
        FUN_004b1b20();
      }
    }
  }
  FUN_004b0950(param_2);
  return;
}


// ==== FUN_00485e40 @ 00485e40

void FUN_00485e40(void)

{
  undefined4 *puVar1;
  float *pfVar2;
  int local_28 [2];
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined1 local_10 [16];
  
  if ((DAT_00540bc4 == 0) &&
     (FUN_004e0620(&DAT_00502348,"\\Ns\\O\\Nugget.cpp",0x246), DAT_00540bc4 == 0)) {
    return;
  }
  if (((DAT_00594fa4 == 0) && (DAT_00594fb8 == 0)) && (DAT_005c85a4 == 0)) {
    puVar1 = (undefined4 *)FUN_00432430(local_28,0);
    pfVar2 = &local_20;
    FUN_00431ff0(local_10,*puVar1,*(undefined2 *)(puVar1 + 1),pfVar2);
    FUN_0040d900(pfVar2);
    local_28[0] = DAT_00542654 * 0x10;
    local_20 = local_20 - (float)(local_28[0] + -0x10);
    local_1c = local_1c - (float)(local_28[0] + -0x10);
    local_18 = local_18 + (float)local_28[0];
    local_14 = (float)local_28[0] + local_14;
    FUN_0040e050(local_20,local_1c,local_18,local_14,1);
  }
  return;
}


