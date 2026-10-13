// ==== FUN_0041fcf0 @ 0041fcf0

int * __thiscall
FUN_0041fcf0(int *param_1,int param_2,int param_3,uint param_4,int param_5,int param_6,int param_7)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  int local_4;
  
  *param_1 = param_2;
  param_1[0x14] = param_7;
  param_1[1] = param_3;
  param_1[3] = 0;
  if ((param_4 & 1) != 0) {
    FUN_00490040(s___rot_1_____0_00516858,s__ns_o_CanonDecoder_cpp_00516840,0x2ae);
  }
  param_1[0xd] = 0;
  param_1[7] = 1;
  bVar5 = param_2 == DAT_0051caf0;
  param_1[2] = (int)param_4 / 2;
  param_1[0xb] = param_5;
  param_1[0xc] = param_6;
  if (bVar5) {
    param_1[0xd] = (int)(&DAT_005151d8 + param_3 * 0x48);
  }
  else if (param_2 == DAT_0051ca8c) {
    param_1[0xd] = (int)(&DAT_00514a80 + param_3 * 0x12);
  }
  else if (param_2 == DAT_0051cabc) {
    param_1[0xd] = (int)(&DAT_005164f8 + param_3 * 0x48);
  }
  else if (param_2 == DAT_0051cbb8) {
    param_1[0xd] = (int)(&DAT_00516588 + param_3 * 0x48);
  }
  else {
    iVar2 = FUN_0041fcc0(param_2);
    if (iVar2 == 0) {
      param_1[0xd] = 0;
      local_4 = 1;
    }
    else {
      param_1[0xd] = (int)(&DAT_005165d0 + param_3 * 0x48);
    }
  }
  iVar2 = param_1[0xd];
  param_1[10] = *param_1 * 0x1d4 + DAT_0051c960;
  iVar3 = local_4;
  if (iVar2 != 0) {
    local_4 = *(int *)(iVar2 + 4);
    iVar3 = *(int *)(iVar2 + 8);
  }
  uVar1 = param_1[2];
  param_1[0xe] = local_4;
  if ((uVar1 & 1) == 0) {
    param_1[0xe] = iVar3;
  }
  param_1[0xf] = iVar3;
  if ((uVar1 & 1) == 0) {
    param_1[0xf] = local_4;
  }
  iVar4 = *(int *)(&DAT_00516660 + uVar1 * 4) * (local_4 + -1);
  iVar2 = *(int *)(&DAT_00516670 + uVar1 * 4);
  param_1[0x10] = iVar4;
  param_1[0x11] = iVar2 * (iVar3 + -1);
  if (param_1[3] != 0) {
    param_1[0x10] = (local_4 - iVar4) + -1;
  }
  param_1[0x13] = 0;
  param_1[0x12] = 0;
  param_1[5] = 1;
  FUN_0041feb0();
  return param_1;
}


// ==== FUN_0041feb0 @ 0041feb0

int __fastcall FUN_0041feb0(int *param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if (param_1[0xf] <= param_1[0x13]) {
      iVar2 = *(int *)(&DAT_00516620 + param_1[2] * 4);
      if (param_1[3] != 0) {
        iVar2 = -iVar2;
      }
      iVar4 = param_1[2] * 4;
      if (*(int *)(&DAT_00516620 + iVar4) != 0) {
        param_1[0x10] = (param_1[0x10] + iVar2 + param_1[0xe]) % param_1[0xe];
      }
      if (*(int *)(&DAT_00516630 + iVar4) != 0) {
        param_1[0x11] =
             (param_1[0x11] + *(int *)(&DAT_00516630 + iVar4) + param_1[0xe]) % param_1[0xe];
      }
      iVar2 = param_1[0x12] + 1;
      param_1[0x12] = iVar2;
      if (param_1[0xe] <= iVar2) {
        param_1[5] = 0;
        return iVar2;
      }
      param_1[0x13] = 0;
    }
    param_1[4] = -1;
    if (param_1[0xd] == 0) {
      if (param_1[0x14] == 0) {
        param_1[4] = *(int *)(param_1[10] + 0x118);
      }
      else {
        iVar2 = param_1[1];
        param_1[4] = iVar2;
        if ((iVar2 < 0) || (*(int *)(*param_1 * 0x1d4 + 0x114 + DAT_0051c960) <= iVar2)) {
          FUN_00490040(s_0_<__frame_&&_frame_<_::getType(_00516880,s__ns_o_CanonDecoder_cpp_00516868
                       ,0x300);
        }
      }
    }
    else {
      iVar2 = FUN_0041fbb0(param_1[0x10],param_1[0x11],&local_c,&local_10,param_1 + 6);
      if (iVar2 != 0) {
        if (param_1[3] == 0) {
          uVar6 = *(undefined4 *)(&DAT_005166bc + param_1[2] * 0x40);
          uVar5 = *(undefined4 *)(&DAT_0051657c + (param_1[2] * 0x10 + local_c) * 4);
        }
        else {
          uVar3 = (int)(param_1[2] + 2U) >> 0x1f;
          iVar2 = ((param_1[2] + 2U ^ uVar3) - uVar3 & 3 ^ uVar3) - uVar3;
          uVar6 = *(undefined4 *)(&DAT_005166bc + iVar2 * 0x40);
          uVar5 = *(undefined4 *)(&DAT_0051657c + (iVar2 * 0x10 + local_c) * 4);
        }
        iVar2 = FUN_004442f0(uVar5,uVar6,local_10);
        param_1[4] = iVar2;
      }
    }
    local_8 = (float)(param_1[7] * param_1[0x13]) + (float)param_1[0xb];
    local_4 = (float)(param_1[0x12] * param_1[7]) + (float)param_1[0xc];
    param_1[8] = (int)local_8;
    param_1[9] = (int)local_4;
    iVar2 = *(int *)(&DAT_00516640 + param_1[2] * 4);
    if (param_1[3] != 0) {
      iVar2 = -iVar2;
    }
    iVar4 = param_1[2] * 4;
    if (*(int *)(&DAT_00516640 + iVar4) != 0) {
      param_1[0x10] = (param_1[0x10] + iVar2 + param_1[0xf]) % param_1[0xf];
    }
    iVar2 = 0;
    if (*(int *)(&DAT_00516650 + iVar4) != 0) {
      iVar1 = param_1[0xf];
      iVar4 = param_1[0x11] + *(int *)(&DAT_00516650 + iVar4) + iVar1;
      iVar2 = iVar4 / iVar1;
      param_1[0x11] = iVar4 % iVar1;
    }
    param_1[0x13] = param_1[0x13] + 1;
  } while (param_1[4] == -1);
  return iVar2;
}


// ==== FUN_0041fcc0 @ 0041fcc0

undefined4 FUN_0041fcc0(int param_1)

{
  if ((((DAT_0051cb50 != param_1) && (DAT_0051cb48 != param_1)) && (DAT_0051cb74 != param_1)) &&
     (DAT_0051cb7c != param_1)) {
    return 0;
  }
  return 1;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1c4) + *(int *)(param_1 + 0x1b8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1b8);
  return;
}


// ==== FUN_004202e0 @ 004202e0

void __thiscall FUN_004202e0(int param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_18;
  int local_14 [3];
  int local_8;
  
  FUN_004442b0(&local_18,local_14,0);
  iVar2 = __ftol();
  local_8 = iVar2;
  iVar3 = __ftol();
  iVar1 = *(int *)(param_1 + 0x38);
  *param_2 = iVar2 - *(int *)(param_1 + 0x3c) * local_18;
  param_2[1] = iVar3 - iVar1 * local_14[0];
  param_2[2] = local_8;
  param_2[3] = iVar3;
  return;
}


// ==== FUN_00440a50 @ 00440a50

void __fastcall FUN_00440a50(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_00445200 @ 00445200

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall
FUN_00445200(int param_1,undefined4 param_2,float param_3,float param_4,undefined4 param_5,
            char param_6,int param_7)

{
  bool bVar1;
  undefined2 uVar2;
  bool bVar3;
  undefined1 uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  int iVar12;
  int iVar13;
  uint uVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  undefined4 *puVar18;
  int local_208;
  float local_200;
  int local_1fc;
  int local_1f8;
  int local_1f4;
  int local_1f0;
  int local_1ec;
  int local_1e8;
  int local_1e4;
  undefined4 local_1e0;
  int local_1dc;
  int local_1d8;
  uint local_1d0;
  int local_1cc;
  int local_1c8;
  int *local_1c4;
  int local_1c0;
  int local_1bc;
  int local_1b8;
  int local_1b4;
  uint local_1b0;
  float local_1ac;
  float local_1a8;
  float local_1a4;
  float local_1a0;
  undefined1 local_19c [4];
  undefined1 local_198 [4];
  undefined **local_194;
  int local_160;
  undefined4 local_15c;
  undefined4 local_158;
  undefined1 local_154 [20];
  int local_140;
  int local_134;
  int local_130;
  float local_100;
  int local_cc;
  
  local_1b4 = 0;
  DAT_0051cbfc = 0;
  if (DAT_00537e98 != 0) {
    DAT_0051cbfc = 0;
    return 1;
  }
  if ((*(byte *)(param_1 + 0xef) & 2) != 0) {
    DAT_0051cbfc = 0;
    return 1;
  }
  local_1e4 = FUN_004448e0();
  uVar5 = (uint)param_6;
  local_1d0 = (uint)(DAT_0050f6c8 == uVar5);
  FUN_004ed7c0(&local_1ac,local_1e4,param_2,param_5,local_19c,local_198,0);
  local_200 = local_1a0 * _DAT_005019a0 + local_1a8 * _DAT_0050199c;
  local_100 = local_1a4 * _DAT_005019a8 + local_1ac * _DAT_005019a4;
  if (local_100 <= local_200) {
    local_100 = local_200;
  }
  iVar6 = __ftol();
  local_100 = (float)iVar6;
  if ((((param_3 < local_100) || (_DAT_005019ac - param_3 < local_100)) || (param_4 < local_100)) ||
     (_DAT_005019ac - param_4 < local_100)) {
    return 0;
  }
  if (local_1d0 != 0) {
    puVar18 = &DAT_00565998;
    for (iVar6 = 0x24; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar18 = 0;
      puVar18 = puVar18 + 1;
    }
    FUN_0041fcf0(local_1e4,param_2,param_5,param_3,param_4,0);
    FUN_004202e0(&local_200);
    if (local_1fc < 2) {
      local_1fc = 1;
    }
    if ((int)local_200 < 2) {
      local_200 = 1.4013e-45;
    }
    local_208 = local_1f4 + 1;
    if (local_1fc + -1 <= local_208) {
      local_1f0 = local_208 * 0x100;
      do {
        iVar6 = local_1f8 + 1;
        if ((int)local_200 + -1 <= iVar6) {
          do {
            iVar16 = local_1f8 - iVar6;
            iVar15 = local_1f4 - local_208;
            if ((*(byte *)(DAT_0052fe48 + iVar6 + local_1f0) & 0x10) == 0) {
              if (((iVar6 < 0) || (local_1f0 < 0)) || ((0xff < iVar6 || (0xffff < local_1f0)))) {
                uVar7 = 0;
              }
              else {
                uVar7 = (uint)*(ushort *)(DAT_0052d590 + (local_1f0 + iVar6) * 2);
              }
              if (uVar7 == 0) goto LAB_004454da;
              local_1d8 = uVar7 * 0x24 + DAT_005395dc;
              uVar7 = FUN_004abae0();
              if ((((uVar7 & 2) != 0) && (DAT_00518904 == 0)) &&
                 ((*(byte *)(local_1d8 + 0x20) != uVar5 &&
                  ((DAT_0050f824 == 0 ||
                   ((&DAT_0050f6e0)[(uint)*(byte *)(local_1d8 + 0x20) * 9 + uVar5] == 0))))))
              goto LAB_004454da;
            }
            else {
LAB_004454da:
              *(undefined1 *)(iVar15 + 0x565999 + (iVar16 + 1) * 0xc) = 1;
            }
            iVar6 = iVar6 + -1;
          } while ((int)local_200 + -1 <= iVar6);
        }
        local_208 = local_208 + -1;
        local_1f0 = local_1f0 + -0x100;
      } while (local_1fc + -1 <= local_208);
    }
  }
  FUN_0041fcf0(local_1e4,param_2,param_5,param_3,param_4,0);
  local_15c = 0;
  local_158 = 0;
  local_160 = 0;
  bVar3 = false;
  local_1e8 = 1;
  local_1b8 = 0;
  local_1dc = 1;
  local_194 = &PTR_LAB_005003d0;
  if (((*(int *)(param_1 + 0x9c) == 10) || ((*(uint *)(param_1 + 0xec) & 0x10202000) != 0)) &&
     ((*(byte *)(param_1 + 0xec) & 4) == 0)) {
    bVar3 = true;
  }
  while (local_140 != 0) {
    FUN_00440a50();
    local_1fc = local_130;
    local_1d8 = local_134;
    uVar8 = __ftol();
    uVar9 = __ftol(uVar8);
    uVar10 = __ftol(uVar9);
    uVar11 = __ftol(uVar10);
    FUN_004eae20(uVar11,uVar10,uVar9,uVar8);
    uVar2 = CONCAT11((char)DAT_00505eb0,(char)DAT_00505eb0);
    puVar18 = (undefined4 *)&DAT_00565a30;
    for (iVar6 = 0x24; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar18 = CONCAT22(uVar2,uVar2);
      puVar18 = puVar18 + 1;
    }
    iVar6 = __ftol();
    local_1bc = (iVar6 - *(int *)(param_1 + 0x1b4)) + 1;
    iVar6 = __ftol();
    local_1c0 = (iVar6 - *(int *)(param_1 + 0x1b8)) + 1;
    local_1b0 = *(uint *)(param_1 + 0xe8) & 6;
    while (local_160 != 0) {
      iVar6 = DAT_005395dc + local_160 * 0x24;
      iVar15 = FUN_004abf50();
      local_1c4 = (int *)(iVar6 + 0xe);
      local_1e4 = *local_1c4;
      local_1e0 = *(undefined4 *)(iVar6 + 0x12);
      if (((DAT_00540a28 == 0) || (iVar16 = (iVar6 - DAT_005395dc) / 0x24, iVar16 < 5)) ||
         (bVar1 = false, 5999 < iVar16)) {
        bVar1 = true;
      }
      if ((bVar1) && (iVar16 = FUN_00444900(*(undefined1 *)(iVar6 + 10),param_7), iVar16 == 0)) {
        if (local_1d0 != 0) {
          iVar16 = 0;
          local_1cc = *(int *)(iVar15 + 0x1b4);
          local_1c8 = *(int *)(iVar15 + 0x1b8);
          if (0 < local_1c8) {
            do {
              iVar17 = 0;
              if (0 < local_1cc) {
                iVar12 = __ftol();
                do {
                  iVar13 = __ftol();
                  if ((((-1 < iVar13) && (iVar13 < *(int *)(param_1 + 0x1b4))) && (-1 < iVar12)) &&
                     (iVar12 < *(int *)(param_1 + 0x1b8))) {
                    (&DAT_005659a5)[iVar13 * 0xc + iVar12] = 1;
                  }
                  iVar17 = iVar17 + 1;
                } while (iVar17 < local_1cc);
              }
              iVar16 = iVar16 + 1;
            } while (iVar16 < local_1c8);
          }
        }
        if (((*(byte *)(iVar6 + 0x23) & 1) == 0) || (param_7 != 0)) {
          return 0;
        }
      }
      if ((((*(byte *)(iVar15 + 0xec) & 4) != 0) && ((*(uint *)(param_1 + 0xec) & 0x208100) != 0))
         && ((*(byte *)(iVar6 + 0x23) & 1) == 0)) {
        if (*(int *)(param_1 + 0x1b8) != *(int *)(param_1 + 0x1b4)) {
          FUN_00490040(s_footX____footY____1_0051d9c0,s__ns_o_RiftType_cpp_0051d9ac,0x3c8);
        }
        local_1b8 = 1;
      }
      if ((*(byte *)(iVar15 + 0xec) & 2) != 0) {
        iVar16 = FUN_004448e0();
        if (iVar16 != DAT_0051cbb8) {
          local_1dc = 0;
        }
        if ((local_1b0 &
            (-(uint)((*(byte *)(*(int *)(iVar15 + 0x124) + 3 + (uint)*(byte *)(iVar6 + 0x22) * 4) &
                     4) == 0) & 0xfffffffe) + 4) != 0) {
          local_1f0 = *local_1c4;
          local_1ec = local_1c4[1];
          iVar16 = __ftol();
          iVar17 = __ftol();
          uVar4 = FUN_004bdbf0(iVar16,iVar17);
          iVar12 = (iVar17 - *(int *)(iVar15 + 0x1b8)) - local_1c0;
          iVar15 = ((iVar16 - *(int *)(iVar15 + 0x1b4)) - local_1bc) + 1;
          if (iVar15 <= iVar16 - local_1bc) {
            iVar13 = iVar15 * 0xc;
            iVar16 = ((iVar16 - local_1bc) - iVar15) + 1;
            iVar15 = iVar12;
            do {
              while (iVar15 = iVar15 + 1, iVar15 <= iVar17 - local_1c0) {
                if ((((-1 < iVar13) && (iVar13 < 0x60)) && (-1 < iVar15)) && (iVar15 < 8)) {
                  (&DAT_00565a30)[iVar15 + iVar13] = uVar4;
                }
              }
              iVar13 = iVar13 + 0xc;
              iVar16 = iVar16 + -1;
              iVar15 = iVar12;
            } while (iVar16 != 0);
          }
          if ((!bVar3) && (iVar6 = FUN_0045bae0((iVar6 - DAT_005395dc) / 0x24,uVar5), iVar6 != 0)) {
            bVar3 = true;
          }
        }
      }
      FUN_004eafe0();
    }
    if (DAT_00505eb0 == DAT_00565a30) {
      local_1e8 = 0;
    }
    else {
      iVar6 = *(int *)(param_1 + 0x1b4);
      if (0 < iVar6) {
        iVar15 = 0;
        do {
          iVar16 = 0;
          if (0 < *(int *)(param_1 + 0x1b8)) {
            do {
              if ((int)(&DAT_00565a30)[iVar16 + iVar15] != (int)DAT_00565a30) {
                local_1e8 = 0;
              }
              iVar16 = iVar16 + 1;
            } while (iVar16 < *(int *)(param_1 + 0x1b8));
          }
          iVar15 = iVar15 + 0xc;
          iVar6 = iVar6 + -1;
        } while (iVar6 != 0);
      }
    }
    if (!bVar3) {
      FUN_004ebce0(local_154,8);
      while (local_cc != 0) {
        if ((((DAT_00518904 != 0) ||
             (uVar7 = (uint)*(byte *)(local_cc * 0x24 + 0x20 + DAT_005395dc), uVar7 == uVar5)) ||
            (((DAT_0050f824 != 0 && ((&DAT_0050f6e0)[uVar7 * 9 + uVar5] != 0)) ||
             (DAT_0051cbf8 != 0)))) && (iVar6 = FUN_0045bae0(local_cc,uVar5), iVar6 != 0)) {
          bVar3 = true;
        }
        FUN_004ec030();
      }
    }
    FUN_0041feb0();
  }
  uVar7 = *(uint *)(param_1 + 0xec);
  if ((((uVar7 & 4) == 0) &&
      ((((*(byte *)(param_1 + 0xe9) & 4) == 0 || (local_1dc == 0)) && (local_1e8 == 0)))) &&
     (local_1b8 == 0)) {
    local_100 = 0.0;
  }
  else {
    local_100 = 1.4013e-45;
  }
  if ((((*(byte *)(param_1 + 0xe9) & 4) != 0) && (local_1dc == 0)) && (local_1e8 == 0)) {
    bVar3 = false;
  }
  iVar6 = 0;
  if (((uVar7 & 0x34000) == 0) || ((uVar7 & 0x200000) != 0)) goto LAB_00445bfd;
  iVar15 = __ftol();
  iVar16 = __ftol();
  if ((iVar15 < 0) || (((iVar16 < 0 || (0xff < iVar15)) || (0xff < iVar16)))) {
    uVar14 = 0;
  }
  else {
    uVar14 = (uint)*(ushort *)(DAT_005670cc + (iVar16 * 0x100 + iVar15) * 2);
  }
  uVar14 = uVar14 * 0x24 + DAT_005395dc;
  if ((uVar14 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar14)) {
LAB_00445bd8:
    iVar6 = 1;
  }
  else if (((uVar7 & 0x4000) == 0) || (*(char *)(uVar14 + 0x20) != '\0')) {
    if (((DAT_00518904 == 0) && (*(byte *)(uVar14 + 0x20) != uVar5)) &&
       ((DAT_0050f824 == 0 || ((&DAT_0050f6e0)[(uint)*(byte *)(uVar14 + 0x20) * 9 + uVar5] == 0))))
    goto LAB_00445bd8;
  }
  else {
    iVar6 = 0;
  }
  DAT_0051cbfc = iVar6;
  if ((uVar7 & 0x4200) != 0) {
    DAT_0051cbfc = 0;
  }
LAB_00445bfd:
  iVar15 = DAT_005395dc;
  if ((uVar7 & 0x200000) == 0) {
    local_200 = (float)__ftol();
    iVar16 = __ftol();
    if (((((int)local_200 < 0) || (iVar16 < 0)) || (0xff < (int)local_200)) || (0xff < iVar16)) {
      uVar14 = 0;
    }
    else {
      uVar14 = (uint)*(ushort *)(DAT_005670cc + (iVar16 * 0x100 + (int)local_200) * 2);
    }
    uVar14 = uVar14 * 0x24 + iVar15;
    if (((((uVar7 & 4) == 0) && (iVar15 + 0x24U <= uVar14)) &&
        ((uVar14 < (uint)(DAT_005395f4 * 0x24 + iVar15) &&
         ((*(byte *)(uVar14 + 0x20) != 0 &&
          ((&DAT_0050f6e0)[(uint)*(byte *)(uVar14 + 0x20) * 9 + uVar5] == 0)))))) &&
       ((uVar7 & 0x20a000) == 0)) {
      local_1b4 = 1;
    }
    if ((((DAT_0051cc00 == 0) && ((!bVar3 || (local_100 == 0.0)))) || (iVar6 != 0)) ||
       (local_1b4 != 0)) {
      return 0;
    }
  }
  return 1;
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


// ==== FUN_004ed7c0 @ 004ed7c0

void FUN_004ed7c0(float *param_1,int param_2,undefined4 param_3,undefined4 param_4,float *param_5,
                 float *param_6,undefined4 param_7)

{
  int iVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  int local_44;
  int local_40;
  float local_34;
  float local_30;
  
  fVar3 = DAT_00565c48;
  *param_5 = DAT_00565c44;
  *param_6 = fVar3;
  local_68 = 0.0;
  local_70 = 1000.0;
  local_6c = 1000.0;
  local_64 = 0.0;
  FUN_0041fcf0(param_2,param_3,param_4,0,0,param_7);
  if (local_40 != 0) {
    do {
      iVar2 = param_2 * 0x1d4 + DAT_0051c960;
      iVar1 = *(int *)(*(int *)(iVar2 + 0xdc) + 8 + local_44 * 8) + -0x24 + *(int *)(iVar2 + 0xdc);
      fVar3 = (float)((int)*(short *)(iVar1 + 0x1c) - *(int *)(iVar2 + 0x1bc));
      fVar4 = (float)((int)*(short *)(iVar1 + 0x1e) - *(int *)(iVar2 + 0x1c0));
      fVar7 = local_34 * DAT_00565c44;
      fVar8 = local_30 * DAT_00565c48;
      fVar5 = fVar7 - fVar3;
      if (fVar5 < local_70) {
        *param_5 = fVar3;
      }
      fVar6 = fVar8 - fVar4;
      if (fVar6 < local_6c) {
        *param_6 = fVar4;
      }
      if (fVar5 <= local_70) {
        local_70 = fVar5;
      }
      fVar7 = ((float)(int)*(short *)(iVar1 + 0x18) - fVar3) + fVar7;
      if (fVar7 <= local_68) {
        fVar7 = local_68;
      }
      if (fVar6 <= local_6c) {
        local_6c = fVar6;
      }
      fVar8 = ((float)(int)*(short *)(iVar1 + 0x1a) - fVar4) + fVar8;
      if (fVar8 <= local_64) {
        fVar8 = local_64;
      }
      FUN_0041feb0();
      local_68 = fVar7;
      local_64 = fVar8;
    } while (local_40 != 0);
  }
  *param_1 = local_70;
  param_1[1] = local_6c;
  param_1[2] = local_68;
  param_1[3] = local_64;
  return;
}


