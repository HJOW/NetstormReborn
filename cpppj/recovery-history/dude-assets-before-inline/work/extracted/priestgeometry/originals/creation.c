// ==== FUN_00425c20 @ 00425c20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * __thiscall
FUN_00425c20(int *param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int extraout_ECX;
  
  iVar3 = param_2;
  param_1[0x14] = param_7;
  param_1[2] = param_4 / 2;
  *param_1 = param_2;
  param_1[1] = param_3;
  param_1[3] = 0;
  param_1[0xb] = param_5;
  param_1[0xc] = param_6;
  param_1[0xd] = 0;
  param_1[7] = 1;
  if (param_2 == DAT_00541204) {
    param_1[0xd] = (int)(&DAT_005300f0 + param_3 * 0x48);
  }
  else if (param_2 == _DAT_005411a0) {
    param_1[0xd] = (int)(&DAT_0052f998 + param_3 * 0x12);
  }
  else if (param_2 == DAT_005411d0) {
    param_1[0xd] = (int)(&DAT_00531410 + param_3 * 0x48);
  }
  else if (param_2 == DAT_005412cc) {
    param_1[0xd] = (int)(&DAT_005314a0 + param_3 * 0x48);
  }
  else {
    iVar2 = FUN_00425830(param_2);
    if (iVar2 == 0) {
      param_1[0xd] = 0;
      param_2 = 1;
    }
    else {
      param_1[0xd] = (int)(&DAT_005314e8 + extraout_ECX * 0x48);
    }
  }
  iVar3 = FUN_0049a840(iVar3);
  param_1[10] = iVar3;
  iVar3 = param_1[0xd];
  iVar2 = param_2;
  if (iVar3 != 0) {
    param_2 = *(int *)(iVar3 + 8);
    iVar2 = *(int *)(iVar3 + 4);
  }
  uVar1 = param_1[2];
  iVar3 = iVar2;
  if ((uVar1 & 1) == 0) {
    iVar3 = param_2;
  }
  param_1[0xe] = iVar3;
  iVar3 = param_2;
  if ((uVar1 & 1) == 0) {
    iVar3 = iVar2;
  }
  param_1[0xf] = iVar3;
  iVar3 = (iVar2 + -1) * *(int *)(&DAT_00531570 + uVar1 * 4);
  param_1[0x10] = iVar3;
  param_1[0x11] = (param_2 + -1) * *(int *)(&DAT_00531580 + uVar1 * 4);
  if (param_1[3] != 0) {
    param_1[0x10] = (iVar2 - iVar3) + -1;
  }
  param_1[0x13] = 0;
  param_1[0x12] = 0;
  param_1[5] = 1;
  FUN_00425860();
  return param_1;
}


// ==== FUN_00425860 @ 00425860

void __fastcall FUN_00425860(undefined4 *param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 local_14;
  int local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if ((int)param_1[0xf] <= (int)param_1[0x13]) {
      iVar4 = *(int *)(&DAT_00531530 + param_1[2] * 4);
      iVar1 = iVar4;
      if (param_1[3] != 0) {
        iVar1 = -iVar4;
      }
      if (iVar4 == 0) {
        iVar4 = param_1[0x10];
      }
      else {
        iVar4 = (iVar1 + param_1[0xe] + param_1[0x10]) % (int)param_1[0xe];
      }
      param_1[0x10] = iVar4;
      if (*(int *)(&DAT_00531540 + param_1[2] * 4) == 0) {
        iVar4 = param_1[0x11];
      }
      else {
        iVar4 = (*(int *)(&DAT_00531540 + param_1[2] * 4) + param_1[0xe] + param_1[0x11]) %
                (int)param_1[0xe];
      }
      param_1[0x11] = iVar4;
      iVar4 = param_1[0x12];
      param_1[0x12] = iVar4 + 1;
      if ((int)param_1[0xe] <= iVar4 + 1) {
        param_1[5] = 0;
        return;
      }
      param_1[0x13] = 0;
    }
    param_1[4] = 0xffffffff;
    if (param_1[0xd] == 0) {
      if (param_1[0x14] == 0) {
        param_1[4] = *(undefined4 *)(param_1[10] + 0x118);
      }
      else {
        param_1[4] = param_1[1];
        if (-1 < (int)param_1[1]) {
          iVar4 = FUN_0049a840(*param_1);
          if ((int)param_1[4] < *(int *)(iVar4 + 0x114)) goto LAB_004259a2;
        }
        FUN_004e0620("0 <= frame && frame < ::getType(type)->numFrames","\\Ns\\O\\Canondecoder.cpp",
                     0x305);
      }
    }
    else {
      iVar4 = FUN_00425700(param_1[0x10],param_1[0x11],&local_10,&local_14,param_1 + 6);
      if (iVar4 != 0) {
        uVar2 = param_1[2];
        if ((param_1[3] != 0) && (uVar2 = uVar2 + 2 & 0x80000003, (int)uVar2 < 0)) {
          uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
        }
        uVar3 = FUN_0049a940((&DAT_0053148c)[(uVar2 * 0x10 + local_10) * 4],
                             (&DAT_005315cc)[uVar2 * 0x40],local_14);
        param_1[4] = uVar3;
      }
    }
LAB_004259a2:
    if (((DAT_005e4794 != 0) && (param_1[4] != -1)) &&
       ((*(byte *)(*(int *)(param_1[10] + 0x124) + 3 + param_1[4] * 4) & 0x20) != 0)) {
      FUN_004e0620("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)",
                   "\\Ns\\O\\Canondecoder.cpp",0x325);
    }
    local_c = param_1[0x12] * param_1[7];
    local_8 = (float)(int)(param_1[7] * param_1[0x13]) + (float)param_1[0xb];
    param_1[8] = local_8;
    local_4 = (float)local_c + (float)param_1[0xc];
    param_1[9] = local_4;
    iVar4 = *(int *)(&DAT_00531550 + param_1[2] * 4);
    iVar1 = iVar4;
    if (param_1[3] != 0) {
      iVar1 = -iVar4;
    }
    if (iVar4 == 0) {
      iVar4 = param_1[0x10];
    }
    else {
      iVar4 = (iVar1 + param_1[0xf] + param_1[0x10]) % (int)param_1[0xf];
    }
    param_1[0x10] = iVar4;
    if (*(int *)(&DAT_00531560 + param_1[2] * 4) == 0) {
      iVar4 = param_1[0x11];
    }
    else {
      iVar4 = (*(int *)(&DAT_00531560 + param_1[2] * 4) + param_1[0xf] + param_1[0x11]) %
              (int)param_1[0xf];
    }
    param_1[0x11] = iVar4;
    param_1[0x13] = param_1[0x13] + 1;
    if (param_1[4] != -1) {
      return;
    }
  } while( true );
}


// ==== FUN_00425830 @ 00425830

undefined4 FUN_00425830(int param_1)

{
  if ((((param_1 != DAT_00541264) && (param_1 != DAT_0054125c)) && (param_1 != DAT_00541288)) &&
     (param_1 != DAT_00541290)) {
    return 0;
  }
  return 1;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1d8);
  return;
}


// ==== FUN_00425b90 @ 00425b90

int * __thiscall FUN_00425b90(undefined4 *param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  undefined4 uVar6;
  int local_8;
  int local_4;
  
  uVar6 = 0;
  piVar5 = &local_4;
  piVar4 = &local_8;
  FUN_0049a840(*param_1,piVar4,piVar5,0);
  FUN_0049a900(piVar4,piVar5,uVar6);
  iVar2 = FUN_004e49c0();
  iVar3 = FUN_004e49c0();
  iVar1 = param_1[0xe];
  *param_2 = iVar2 - param_1[0xf] * local_8;
  param_2[1] = iVar3 - iVar1 * local_4;
  param_2[2] = iVar2;
  param_2[3] = iVar3;
  return param_2;
}


// ==== FUN_0041d770 @ 0041d770

void __fastcall FUN_0041d770(float *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *param_1 = (float)iVar1;
  iVar1 = FUN_004e49c0();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_0049b510 @ 0049b510

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall
FUN_0049b510(int param_1,undefined4 param_2,float param_3,float param_4,undefined4 param_5,
            char param_6,int param_7)

{
  byte bVar1;
  float fVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  undefined1 uVar7;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  uint uVar14;
  int iVar15;
  int extraout_ECX;
  undefined2 uVar16;
  int iVar17;
  undefined1 *puVar18;
  int iVar19;
  int iVar20;
  undefined4 *puVar21;
  bool bVar22;
  int local_268;
  int local_260;
  int local_250;
  int local_24c;
  int local_248;
  int local_244;
  int local_238;
  int local_234;
  int local_230;
  int local_22c;
  undefined4 local_228;
  undefined4 local_224;
  undefined4 local_220;
  undefined4 local_21c;
  uint local_218;
  uint local_214;
  int local_210;
  int local_20c;
  int local_208;
  int local_204;
  undefined4 local_200 [4];
  undefined1 local_1f0 [4];
  undefined1 local_1ec [4];
  undefined **local_1e8;
  int local_1b4;
  undefined4 local_1b0;
  undefined4 local_1ac;
  undefined1 local_1a8 [20];
  int local_194;
  int local_188;
  int local_184;
  int local_cc;
  
  local_20c = 0;
  DAT_0059ab2c = 0;
  if ((DAT_0055a488 != 0) || ((*(uint *)(param_1 + 0xec) & 0x2000000) != 0)) {
    return 1;
  }
  iVar15 = (param_1 - DAT_0059ab20) / 500;
  bVar22 = param_6 == DAT_00540c70;
  local_214 = (uint)bVar22;
  FUN_0043cbd0(local_200,iVar15,param_2,param_5,local_1ec,local_1f0,0);
  iVar8 = FUN_004e49c0();
  fVar2 = (float)iVar8;
  if (param_3 < fVar2) {
    return 0;
  }
  if (_DAT_005022e4 - param_3 < fVar2) {
    return 0;
  }
  if (param_4 < fVar2) {
    return 0;
  }
  if (_DAT_005022e4 - param_4 < fVar2) {
    return 0;
  }
  if (bVar22 != 0) {
    puVar21 = &DAT_0059a9f0;
    for (iVar8 = 0x24; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar21 = 0;
      puVar21 = puVar21 + 1;
    }
    FUN_00425c20(iVar15,param_2,param_5,param_3,param_4,0);
    FUN_00425b90(&local_250);
    if (local_24c < 1) {
      local_24c = 1;
    }
    if (local_250 < 1) {
      local_250 = 1;
    }
    iVar8 = local_244 + 1;
    if (local_24c + -1 <= iVar8) {
      local_260 = iVar8 * 0x100;
      do {
        iVar17 = local_248 + 1;
        if (local_250 + -1 <= iVar17) {
          do {
            iVar20 = local_244 - iVar8;
            iVar19 = (local_248 - iVar17) + 1;
            if (iVar17 < 0) {
              return 0;
            }
            if (iVar8 < 0) {
              return 0;
            }
            if (0xff < iVar17) {
              return 0;
            }
            if (0xfd < iVar8) {
              return 0;
            }
            if ((((iVar19 < 0) || (iVar20 + 1 < 0)) || (0xb < iVar19)) || (0xb < iVar20 + 1)) {
              FUN_004e0620("px>=0 && py>=0 && px<LEGAL_ARRAY_WIDTH && py<LEGAL_ARRAY_HEIGHT",
                           "\\Ns\\O\\Rifttype.cpp",0x37e);
            }
            if (0xff < iVar8) {
              FUN_004e0620("x>=0 && y>=0 && x<BOARDDIM_IN_TILES && y<BOARDDIM_IN_TILES",
                           "\\Ns\\O\\Rifttype.cpp",0x37f);
            }
            if ((*(byte *)(local_260 + DAT_005c7c44 + iVar17) & 0x10) == 0) {
              iVar9 = FUN_0040da00(iVar17,iVar8);
              if (iVar9 == 0) {
                *(undefined1 *)(iVar20 + 0x59a9f1 + iVar19 * 0xc) = 1;
              }
              else {
                iVar9 = iVar9 * 0x32 + DAT_005c8464;
                uVar14 = FUN_004ac200();
                if (((uVar14 & 2) != 0) &&
                   (iVar9 = FUN_004629e0(*(undefined1 *)(iVar9 + 0x22),(int)param_6), iVar9 == 0))
                goto LAB_0049b7dd;
              }
            }
            else {
LAB_0049b7dd:
              *(undefined1 *)(iVar20 + 0x59a9f1 + iVar19 * 0xc) = 1;
            }
            iVar17 = iVar17 + -1;
          } while (local_250 + -1 <= iVar17);
        }
        iVar8 = iVar8 + -1;
        local_260 = local_260 + -0x100;
      } while (local_24c + -1 <= iVar8);
    }
  }
  FUN_00425c20(iVar15,param_2,param_5,param_3,param_4,0);
  local_1b0 = 0;
  local_1ac = 0;
  local_1b4 = 0;
  bVar22 = false;
  bVar5 = false;
  local_1e8 = &PTR_LAB_00502318;
  bVar4 = true;
  bVar3 = true;
  if (((*(int *)(param_1 + 0x9c) == 10) || ((*(uint *)(param_1 + 0xec) & 0x10202000) != 0)) &&
     ((*(byte *)(param_1 + 0xec) & 4) == 0)) {
    bVar22 = true;
  }
  while (local_194 != 0) {
    FUN_0041d770();
    local_204 = local_184;
    local_208 = local_188;
    local_250 = local_188;
    local_24c = local_184;
    uVar10 = FUN_004e49c0();
    uVar11 = FUN_004e49c0(uVar10);
    uVar12 = FUN_004e49c0(uVar11);
    uVar13 = FUN_004e49c0(uVar12);
    FUN_004b16d0(uVar13,uVar12,uVar11,uVar10);
    uVar16 = CONCAT11((undefined1)DAT_0050c038,(undefined1)DAT_0050c038);
    puVar21 = (undefined4 *)&DAT_0059aa88;
    for (iVar15 = 0x24; iVar15 != 0; iVar15 = iVar15 + -1) {
      *puVar21 = CONCAT22(uVar16,uVar16);
      puVar21 = puVar21 + 1;
    }
    iVar15 = FUN_004e49c0();
    iVar15 = (iVar15 - *(int *)(param_1 + 0x1d4)) + 1;
    iVar8 = FUN_004e49c0();
    local_234 = (iVar8 - *(int *)(param_1 + 0x1d8)) + 1;
    local_218 = *(uint *)(param_1 + 0xe8) & 6;
    while (local_1b4 != 0) {
      iVar8 = local_1b4 * 0x32 + DAT_005c8464;
      local_22c = iVar8;
      iVar17 = FUN_004ac550();
      local_21c = *(undefined4 *)(iVar8 + 0x12);
      local_220 = *(undefined4 *)(iVar8 + 0xe);
      if ((((DAT_00540bc0 == 0) || (iVar19 = (iVar8 - DAT_005c8464) / 0x32, iVar19 < 5)) ||
          (iVar20 = param_1, 14999 < iVar19)) &&
         (iVar19 = FUN_0049ade0(*(undefined1 *)(iVar8 + 10),param_7), iVar20 = extraout_ECX,
         iVar19 == 0)) {
        if (local_214 != 0) {
          local_210 = *(int *)(iVar17 + 0x1d8);
          local_238 = *(int *)(iVar17 + 0x1d4);
          local_260 = 0;
          if (0 < local_210) {
            do {
              iVar19 = 0;
              local_230 = 0;
              if (0 < local_238) {
                iVar20 = FUN_004e49c0();
                do {
                  iVar9 = FUN_004e49c0();
                  if (((-1 < iVar9) && (iVar9 < *(int *)(param_1 + 0x1d4))) &&
                     ((-1 < iVar20 && (iVar20 < *(int *)(param_1 + 0x1d8))))) {
                    (&DAT_0059a9fd)[iVar9 * 0xc + iVar20] = 1;
                  }
                  iVar19 = iVar19 + 1;
                  local_230 = iVar19;
                } while (iVar19 < local_238);
              }
              local_260 = local_260 + 1;
            } while (local_260 < local_210);
          }
        }
        if ((*(byte *)(iVar8 + 0x28) & 1) == 0) {
          return 0;
        }
        iVar20 = param_1;
        if (param_7 != 0) {
          return 0;
        }
      }
      if ((((*(byte *)(iVar17 + 0xec) & 4) != 0) && ((*(uint *)(iVar20 + 0xec) & 0x208100) != 0)) &&
         ((*(byte *)(iVar8 + 0x28) & 1) == 0)) {
        if (*(int *)(iVar20 + 0x1d4) != *(int *)(iVar20 + 0x1d8)) {
          FUN_004e0620("footX == footY == 1","\\Ns\\O\\Rifttype.cpp",0x3e5);
        }
        bVar5 = true;
      }
      if ((*(byte *)(iVar17 + 0xec) & 2) != 0) {
        if ((iVar17 - DAT_0059ab20) / 500 != DAT_005412cc) {
          bVar3 = false;
        }
        if ((local_218 &
            (-(uint)((*(byte *)(*(int *)(iVar17 + 0x124) + 3 + *(int *)(iVar8 + 0x24) * 4) & 4) != 0
                    ) & 2) + 2) != 0) {
          local_228 = *(undefined4 *)(iVar8 + 0xe);
          local_224 = *(undefined4 *)(iVar8 + 0x12);
          iVar8 = FUN_004e49c0();
          iVar19 = FUN_004e49c0();
          uVar7 = FUN_0046f4e0(iVar8,iVar19);
          iVar9 = (iVar19 - *(int *)(iVar17 + 0x1d8)) - local_234;
          iVar20 = iVar8 - iVar15;
          iVar8 = ((iVar8 - *(int *)(iVar17 + 0x1d4)) - iVar15) + 1;
          if (iVar8 <= iVar20) {
            puVar18 = &DAT_0059aa88 + iVar8 * 0xc;
            iVar17 = (iVar20 - iVar8) + 1;
            iVar8 = iVar9;
            do {
              while (iVar8 = iVar8 + 1, iVar8 <= iVar19 - local_234) {
                if ((((0x59aa87 < (int)puVar18) && ((int)puVar18 < 0x59aae8)) && (-1 < iVar8)) &&
                   (iVar8 < 8)) {
                  puVar18[iVar8] = uVar7;
                }
              }
              puVar18 = puVar18 + 0xc;
              iVar17 = iVar17 + -1;
              iVar8 = iVar9;
            } while (iVar17 != 0);
          }
          if ((!bVar22) &&
             (iVar8 = FUN_00462cb0((local_22c - DAT_005c8464) / 0x32,(int)param_6), iVar8 != 0)) {
            bVar22 = true;
          }
        }
      }
      FUN_004b1810();
    }
    if (DAT_0059aa88 == DAT_0050c038) {
      bVar4 = false;
    }
    else {
      iVar15 = *(int *)(param_1 + 0x1d4);
      if (0 < iVar15) {
        puVar18 = &DAT_0059aa88;
        do {
          iVar8 = 0;
          if (0 < *(int *)(param_1 + 0x1d8)) {
            do {
              if ((int)(char)puVar18[iVar8] != (int)DAT_0059aa88) {
                bVar4 = false;
              }
              iVar8 = iVar8 + 1;
            } while (iVar8 < *(int *)(param_1 + 0x1d8));
          }
          puVar18 = puVar18 + 0xc;
          iVar15 = iVar15 + -1;
        } while (iVar15 != 0);
      }
    }
    if (!bVar22) {
      FUN_004b2590(local_1a8,8);
      while (local_cc != 0) {
        uVar14 = (uint)*(byte *)(local_cc * 0x32 + 0x22 + DAT_005c8464);
        if (((((DAT_005c85a4 != 0) || (uVar14 == (int)param_6)) ||
             ((DAT_00540cb0 != 0 && (*(int *)(&DAT_00595200 + ((int)param_6 + uVar14 * 9) * 4) != 0)
              ))) || (DAT_0059ab28 != 0)) &&
           (iVar15 = FUN_00462cb0(local_cc,(int)param_6), iVar15 != 0)) {
          bVar22 = true;
        }
        FUN_004b1e70();
      }
    }
    FUN_00425860();
  }
  uVar14 = *(uint *)(param_1 + 0xec);
  if (((uVar14 & 4) != 0) ||
     (((((*(uint *)(param_1 + 0xe8) & 0x400) != 0 && (bVar3)) || (bVar4)) || (bVar6 = false, bVar5))
     )) {
    bVar6 = true;
  }
  if ((((*(uint *)(param_1 + 0xe8) & 0x400) != 0) && (!bVar3)) && (!bVar4)) {
    bVar22 = false;
  }
  local_268 = 0;
  if (((uVar14 & 0x34000) == 0) || ((uVar14 & 0x200000) != 0)) goto LAB_0049bed6;
  iVar15 = FUN_0040eaf0(param_3,param_4);
  iVar8 = iVar15 * 0x32 + DAT_005c8464;
  iVar15 = FUN_0040ea40();
  if (iVar15 == 0) {
LAB_0049beb0:
    local_268 = 1;
  }
  else if (((uVar14 & 0x4000) == 0) || (*(char *)(iVar8 + 0x22) != '\0')) {
    iVar15 = FUN_004629e0(*(undefined1 *)(iVar8 + 0x22),(int)param_6);
    if (iVar15 == 0) goto LAB_0049beb0;
  }
  else {
    local_268 = 0;
  }
  DAT_0059ab2c = local_268;
  if ((*(uint *)(param_1 + 0xec) & 0x4200) != 0) {
    DAT_0059ab2c = 0;
  }
LAB_0049bed6:
  if ((((bVar4) && ((*(uint *)(param_1 + 0xec) & 0x200) != 0)) && (DAT_0059aa88 != DAT_0050c038)) &&
     (iVar15 = FUN_0046f260((int)DAT_0059aa88), iVar15 != 0)) {
    bVar6 = false;
  }
  uVar14 = *(uint *)(param_1 + 0xec);
  if ((uVar14 & 0x200000) == 0) {
    iVar15 = FUN_0040eaf0(param_3,param_4);
    iVar15 = iVar15 * 0x32 + DAT_005c8464;
    if ((((uVar14 & 4) != 0) || (iVar8 = FUN_0040ea40(), iVar8 == 0)) ||
       ((bVar1 = *(byte *)(iVar15 + 0x22), bVar1 == 0 ||
        ((*(int *)(&DAT_00595200 + ((int)param_6 + (uint)bVar1 * 9) * 4) != 0 ||
         (iVar15 = 1, (uVar14 & 0x20a000) != 0)))))) {
      iVar15 = local_20c;
    }
    if ((((DAT_0059ab30 == 0) && ((!bVar22 || (!bVar6)))) || (local_268 != 0)) || (iVar15 != 0)) {
      return 0;
    }
  }
  return 1;
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


// ==== FUN_0043cbd0 @ 0043cbd0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0043cbd0(float *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 float *param_5,float *param_6,undefined4 param_7)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  undefined4 local_44;
  int local_40;
  float local_34;
  float local_30;
  
  *param_5 = _DAT_0059a92c;
  *param_6 = _DAT_0059a930;
  local_78 = 1000.0;
  local_74 = 1000.0;
  local_70 = 0.0;
  local_6c = 0.0;
  FUN_00425c20(param_2,param_3,param_4,0,0,param_7);
  while (local_40 != 0) {
    iVar4 = FUN_0049a840(param_2);
    iVar5 = FUN_00419850(0,local_44);
    iVar6 = (int)*(short *)(iVar5 + 0x1c) - *(int *)(iVar4 + 0x1dc);
    fVar1 = (float)((int)*(short *)(iVar5 + 0x1e) - *(int *)(iVar4 + 0x1e0));
    fVar2 = local_30 * _DAT_0059a930;
    fVar3 = local_34 * _DAT_0059a92c - (float)iVar6;
    if (fVar3 < local_78) {
      *param_5 = (float)iVar6;
    }
    fVar2 = fVar2 - fVar1;
    if (fVar2 < local_74) {
      *param_6 = fVar1;
    }
    if (fVar3 <= local_78) {
      local_78 = fVar3;
    }
    fVar3 = (float)(int)*(short *)(iVar5 + 0x18) + fVar3;
    if (local_70 <= fVar3) {
      local_70 = fVar3;
    }
    if (fVar2 <= local_74) {
      local_74 = fVar2;
    }
    fVar2 = fVar2 + (float)(int)*(short *)(iVar5 + 0x1a);
    if (local_6c <= fVar2) {
      local_6c = fVar2;
    }
    FUN_00425860();
  }
  *param_1 = local_78;
  param_1[1] = local_74;
  param_1[2] = local_70;
  param_1[3] = local_6c;
  return;
}


