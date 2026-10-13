// ==== FUN_00465772 @ 00465772

undefined4 FUN_00465772(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int *piVar1;
  undefined3 uVar2;
  byte bVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  uint uVar13;
  byte bVar14;
  byte bVar16;
  uint uVar15;
  byte *pbVar17;
  byte *pbVar18;
  byte *pbVar19;
  bool bVar20;
  bool bVar21;
  byte *local_30;
  byte *local_2c;
  byte *local_28;
  int local_24;
  
  piVar1 = (int *)*param_1;
  iVar8 = piVar1[1];
  iVar4 = iVar8 + 1;
  if (iVar4 != 0 && -2 < iVar8) {
    iVar9 = piVar1[2];
    if (-1 < iVar9) {
      iVar10 = param_1[1];
      iVar5 = iVar10;
      if (iVar10 < 1) {
        iVar5 = 0;
      }
      iVar11 = param_1[2];
      iVar6 = iVar11;
      if (iVar11 < 1) {
        iVar6 = 0;
      }
      iVar7 = param_1[3];
      if (iVar8 <= param_1[3]) {
        iVar7 = iVar8;
      }
      iVar8 = param_1[4];
      if (iVar9 <= param_1[4]) {
        iVar8 = iVar9;
      }
      if ((iVar7 < iVar5) || (iVar8 < iVar6)) {
        return 0xfffffffe;
      }
      param_4 = param_4 + iVar10;
      param_5 = param_5 + iVar11;
      param_2 = *(int *)(param_3 * 8 + 8 + param_2) + param_2;
      iVar9 = *(int *)(param_2 + 8) + param_4;
      local_24 = *(int *)(param_2 + 0xc) + param_5;
      iVar10 = *(int *)(param_2 + 0x10) + param_4;
      iVar11 = *(int *)(param_2 + 0x14) + param_5;
      pbVar17 = (byte *)(param_2 + 0x18);
      if ((iVar10 < iVar9) || (iVar11 < local_24)) {
        return 0xfffffffc;
      }
      bVar14 = (((iVar9 - iVar5 < 0) * '\x02' + (iVar7 - iVar9 < 0)) * '\x02' +
               (local_24 - iVar6 < 0)) * '\x02' + (iVar8 - local_24 < 0);
      bVar16 = (((iVar10 - iVar5 < 0) * '\x02' + (iVar7 - iVar10 < 0)) * '\x02' +
               (iVar11 - iVar6 < 0)) * '\x02' + (iVar8 - iVar11 < 0);
      if ((bVar16 & bVar14) != 0) {
        return 0xfffffffd;
      }
      if (bVar14 == 0 && bVar16 == 0) {
        FUN_00465b95(param_1,param_2,param_4 - param_1[1],param_5 - param_1[2],iVar4);
        return 0;
      }
      local_28 = (byte *)(local_24 * iVar4 * piVar1[3] + *piVar1 + iVar9 * piVar1[3]);
LAB_00465937:
      if (local_24 < iVar6) {
        do {
          bVar3 = *pbVar17;
          pbVar17 = pbVar17 + 1;
          bVar20 = (bool)(bVar3 & 1);
          bVar3 = bVar3 >> 1;
          if (bVar20 || bVar3 == 0) {
            if (bVar3 != 0) {
              pbVar17 = pbVar17 + (bVar3 - 1);
            }
            else if (!bVar20) goto code_r0x00465933;
          }
          pbVar17 = pbVar17 + 1;
        } while( true );
      }
      local_2c = local_28 + (iVar5 - iVar9);
      local_30 = local_28 + (iVar7 - iVar9);
LAB_00465b65:
      if (iVar11 < local_24) {
        return 0;
      }
      if (iVar8 < local_24) {
        return 0;
      }
      pbVar19 = local_28;
      if ((bVar14 & 8) == 0) {
        if ((bVar16 & 4) == 0) {
          bVar3 = *pbVar17;
          pbVar17 = pbVar17 + 1;
          bVar20 = (bool)(bVar3 & 1);
          bVar3 = bVar3 >> 1;
          uVar12 = (uint)bVar3;
          if (!bVar20 && bVar3 != 0) goto LAB_004659a2;
          if (bVar3 != 0) goto LAB_004659cb;
joined_r0x0046598b:
          if (bVar20) {
            do {
              while( true ) {
                pbVar18 = pbVar17 + 1;
                pbVar19 = pbVar19 + *pbVar17;
                pbVar17 = pbVar17 + 2;
                bVar20 = (bool)(*pbVar18 & 1);
                bVar3 = *pbVar18 >> 1;
                uVar12 = (uint)bVar3;
                if (!bVar20 && bVar3 != 0) goto LAB_004659a2;
                if (bVar3 == 0) break;
LAB_004659cb:
                do {
                  while( true ) {
                    uVar12 = uVar12 & 0xff;
LAB_004659ce:
                    for (uVar13 = uVar12 & 3; uVar13 != 0; uVar13 = uVar13 - 1) {
                      *pbVar19 = *pbVar17;
                      pbVar17 = pbVar17 + 1;
                      pbVar19 = pbVar19 + 1;
                    }
                    pbVar18 = pbVar17;
                    for (uVar12 = uVar12 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
                      *(undefined4 *)pbVar19 = *(undefined4 *)pbVar18;
                      pbVar18 = pbVar18 + 4;
                      pbVar19 = pbVar19 + 4;
                    }
                    pbVar17 = pbVar18 + 1;
                    bVar20 = (bool)(*pbVar18 & 1);
                    bVar3 = *pbVar18 >> 1;
                    uVar12 = (uint)bVar3;
                    if (!bVar20 && bVar3 != 0) break;
                    if (bVar3 == 0) goto joined_r0x0046598b;
                  }
LAB_004659a2:
                  do {
                    uVar12 = uVar12 & 0xff;
LAB_004659a5:
                    bVar3 = *pbVar17;
                    pbVar18 = pbVar17 + 1;
                    for (uVar13 = uVar12 & 3; uVar13 != 0; uVar13 = uVar13 - 1) {
                      *pbVar19 = bVar3;
                      pbVar19 = pbVar19 + 1;
                    }
                    uVar2 = CONCAT21(CONCAT11(bVar3,bVar3),bVar3);
                    for (uVar12 = uVar12 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
                      *(uint *)pbVar19 = CONCAT31(uVar2,bVar3);
                      pbVar19 = pbVar19 + 4;
                    }
                    pbVar17 = pbVar17 + 2;
                    bVar20 = (bool)(*pbVar18 & 1);
                    bVar3 = *pbVar18 >> 1;
                    uVar12 = CONCAT31(uVar2,bVar3);
                  } while (!bVar20 && bVar3 != 0);
                  if (!bVar20) goto LAB_00465b56;
                } while (bVar3 != 0);
              }
            } while (bVar20);
          }
        }
        else {
          bVar3 = *pbVar17;
          pbVar17 = pbVar17 + 1;
          bVar20 = (bool)(bVar3 & 1);
          bVar3 = bVar3 >> 1;
          if (!bVar20 && bVar3 != 0) goto LAB_00465a98;
          if (bVar3 != 0) goto LAB_00465adb;
joined_r0x00465a7d:
          if (bVar20) {
            do {
              while( true ) {
                pbVar18 = pbVar17 + 1;
                pbVar19 = pbVar19 + *pbVar17;
                pbVar17 = pbVar17 + 2;
                bVar20 = (bool)(*pbVar18 & 1);
                bVar3 = *pbVar18 >> 1;
                if (!bVar20 && bVar3 != 0) goto LAB_00465a98;
                if (bVar3 == 0) break;
LAB_00465adb:
                do {
                  while( true ) {
                    uVar12 = (uint)bVar3;
LAB_00465ade:
                    if ((int)local_30 < (int)pbVar19) goto LAB_00465b47;
                    uVar15 = ~((int)(pbVar19 + ((uVar12 - 1) - (int)local_30)) >> 0x1f) &
                             (uint)(pbVar19 + ((uVar12 - 1) - (int)local_30));
                    for (uVar13 = uVar12 - uVar15 & 3; uVar13 != 0; uVar13 = uVar13 - 1) {
                      *pbVar19 = *pbVar17;
                      pbVar17 = pbVar17 + 1;
                      pbVar19 = pbVar19 + 1;
                    }
                    for (uVar12 = uVar12 - uVar15 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
                      *(undefined4 *)pbVar19 = *(undefined4 *)pbVar17;
                      pbVar17 = pbVar17 + 4;
                      pbVar19 = pbVar19 + 4;
                    }
                    pbVar19 = pbVar19 + uVar15;
                    bVar3 = pbVar17[uVar15];
                    pbVar17 = pbVar17 + uVar15 + 1;
                    bVar20 = (bool)(bVar3 & 1);
                    bVar3 = bVar3 >> 1;
                    if (!bVar20 && bVar3 != 0) break;
                    if (bVar3 == 0) goto joined_r0x00465a7d;
                  }
LAB_00465a98:
                  do {
                    uVar12 = (uint)bVar3;
LAB_00465a9b:
                    if ((int)local_30 < (int)pbVar19) goto LAB_00465b36;
                    uVar15 = ~((int)(pbVar19 + ((uVar12 - 1) - (int)local_30)) >> 0x1f) &
                             (uint)(pbVar19 + ((uVar12 - 1) - (int)local_30));
                    uVar12 = uVar12 - uVar15;
                    bVar3 = *pbVar17;
                    pbVar18 = pbVar17 + 1;
                    for (uVar13 = uVar12 & 3; uVar13 != 0; uVar13 = uVar13 - 1) {
                      *pbVar19 = bVar3;
                      pbVar19 = pbVar19 + 1;
                    }
                    for (uVar12 = uVar12 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
                      *(uint *)pbVar19 = CONCAT31(CONCAT21(CONCAT11(bVar3,bVar3),bVar3),bVar3);
                      pbVar19 = pbVar19 + 4;
                    }
                    pbVar19 = pbVar19 + uVar15;
                    pbVar17 = pbVar17 + 2;
                    bVar20 = (bool)(*pbVar18 & 1);
                    bVar3 = *pbVar18 >> 1;
                  } while (!bVar20 && bVar3 != 0);
                  if (!bVar20) goto LAB_00465b56;
                } while (bVar3 != 0);
              }
            } while (bVar20);
          }
        }
      }
      else {
        bVar3 = *pbVar17;
        pbVar17 = pbVar17 + 1;
        bVar20 = (bool)(bVar3 & 1);
        bVar3 = bVar3 >> 1;
        if (!bVar20 && bVar3 != 0) goto LAB_00465a0a;
        if (bVar3 != 0) goto LAB_00465a3b;
joined_r0x004659f3:
        if (bVar20) {
          do {
            while( true ) {
              pbVar18 = pbVar17 + 1;
              pbVar19 = pbVar19 + *pbVar17;
              pbVar17 = pbVar17 + 2;
              bVar20 = (bool)(*pbVar18 & 1);
              bVar3 = *pbVar18 >> 1;
              if (!bVar20 && bVar3 != 0) goto LAB_00465a0a;
              if (bVar3 == 0) break;
LAB_00465a3b:
              do {
                while( true ) {
                  uVar12 = (uint)bVar3;
                  iVar9 = (int)local_2c - (int)pbVar19;
                  if (iVar9 < (int)uVar12) {
                    if (-1 < iVar9) {
                      pbVar19 = pbVar19 + iVar9;
                      uVar12 = uVar12 - iVar9;
                      pbVar17 = pbVar17 + iVar9;
                    }
                    if ((bVar16 & 4) == 0) goto LAB_004659ce;
                    goto LAB_00465ade;
                  }
                  pbVar19 = pbVar19 + uVar12;
                  bVar3 = pbVar17[uVar12];
                  pbVar17 = pbVar17 + uVar12 + 1;
                  bVar20 = (bool)(bVar3 & 1);
                  bVar3 = bVar3 >> 1;
                  if (!bVar20 && bVar3 != 0) break;
                  if (bVar3 == 0) goto joined_r0x004659f3;
                }
LAB_00465a0a:
                do {
                  uVar12 = (uint)bVar3;
                  iVar9 = (int)local_2c - (int)pbVar19;
                  if (iVar9 < (int)uVar12) {
                    if (-1 < iVar9) {
                      pbVar19 = pbVar19 + iVar9;
                      uVar12 = uVar12 - iVar9;
                    }
                    if ((bVar16 & 4) == 0) goto LAB_004659a5;
                    goto LAB_00465a9b;
                  }
                  pbVar19 = pbVar19 + uVar12;
                  pbVar18 = pbVar17 + 1;
                  pbVar17 = pbVar17 + 2;
                  bVar20 = (bool)(*pbVar18 & 1);
                  bVar3 = *pbVar18 >> 1;
                } while (!bVar20 && bVar3 != 0);
                if (!bVar20) goto LAB_00465b56;
              } while (bVar3 != 0);
            }
          } while (bVar20);
        }
      }
      goto LAB_00465b56;
    }
  }
  return 0xffffffff;
code_r0x00465933:
  local_28 = local_28 + iVar4;
  local_24 = local_24 + 1;
  goto LAB_00465937;
LAB_00465b36:
  do {
    pbVar19 = pbVar17 + 1;
    pbVar17 = pbVar17 + 2;
    bVar20 = (bool)(*pbVar19 & 1);
    bVar3 = *pbVar19 >> 1;
  } while (!bVar20 && bVar3 != 0);
  if (bVar20) {
    if (bVar3 == 0) goto LAB_00465b1e;
    while( true ) {
      uVar12 = (uint)bVar3;
LAB_00465b47:
      bVar3 = pbVar17[uVar12];
      pbVar17 = pbVar17 + uVar12 + 1;
      bVar20 = (bool)(bVar3 & 1);
      bVar3 = bVar3 >> 1;
      bVar21 = bVar3 != 0;
      if (!bVar20 && bVar21) break;
      while (!bVar21) {
        if (!bVar20) goto LAB_00465b56;
LAB_00465b1e:
        pbVar19 = pbVar17 + 1;
        pbVar17 = pbVar17 + 2;
        bVar20 = (bool)(*pbVar19 & 1);
        bVar3 = *pbVar19 >> 1;
        bVar21 = bVar3 != 0;
        if (!bVar20 && bVar21) goto LAB_00465b36;
      }
    }
    goto LAB_00465b36;
  }
LAB_00465b56:
  local_28 = local_28 + iVar4;
  local_2c = local_2c + iVar4;
  local_30 = local_30 + iVar4;
  local_24 = local_24 + 1;
  goto LAB_00465b65;
}


// ==== FUN_00465c7f @ 00465c7f

undefined4 FUN_00465c7f(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  undefined1 uVar1;
  int *piVar2;
  undefined3 uVar3;
  byte bVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  undefined1 *puVar13;
  uint uVar14;
  uint uVar15;
  byte bVar16;
  byte bVar18;
  uint uVar17;
  byte *pbVar19;
  byte *pbVar20;
  undefined4 *puVar21;
  bool bVar22;
  bool bVar23;
  undefined1 *local_30;
  undefined1 *local_2c;
  undefined4 *local_28;
  int local_24;
  
  piVar2 = (int *)*param_1;
  iVar9 = piVar2[1];
  iVar5 = iVar9 + 1;
  if (iVar5 != 0 && -2 < iVar9) {
    iVar10 = piVar2[2];
    if (-1 < iVar10) {
      iVar11 = param_1[1];
      iVar6 = iVar11;
      if (iVar11 < 1) {
        iVar6 = 0;
      }
      iVar12 = param_1[2];
      iVar7 = iVar12;
      if (iVar12 < 1) {
        iVar7 = 0;
      }
      iVar8 = param_1[3];
      if (iVar9 <= param_1[3]) {
        iVar8 = iVar9;
      }
      iVar9 = param_1[4];
      if (iVar10 <= param_1[4]) {
        iVar9 = iVar10;
      }
      if ((iVar8 < iVar6) || (iVar9 < iVar7)) {
        return 0xfffffffe;
      }
      param_4 = param_4 + iVar11;
      param_5 = param_5 + iVar12;
      param_2 = *(int *)(param_3 * 8 + 8 + param_2) + param_2;
      iVar10 = *(int *)(param_2 + 8) + param_4;
      local_24 = *(int *)(param_2 + 0xc) + param_5;
      iVar11 = *(int *)(param_2 + 0x10) + param_4;
      iVar12 = *(int *)(param_2 + 0x14) + param_5;
      pbVar19 = (byte *)(param_2 + 0x18);
      if ((iVar11 < iVar10) || (iVar12 < local_24)) {
        return 0xfffffffc;
      }
      bVar16 = (((iVar10 - iVar6 < 0) * '\x02' + (iVar8 - iVar10 < 0)) * '\x02' +
               (local_24 - iVar7 < 0)) * '\x02' + (iVar9 - local_24 < 0);
      bVar18 = (((iVar11 - iVar6 < 0) * '\x02' + (iVar8 - iVar11 < 0)) * '\x02' +
               (iVar12 - iVar7 < 0)) * '\x02' + (iVar9 - iVar12 < 0);
      if ((bVar18 & bVar16) != 0) {
        return 0xfffffffd;
      }
      if (bVar16 == 0 && bVar18 == 0) {
        FUN_00466176(param_1,param_2,param_4 - param_1[1],param_5 - param_1[2],iVar5);
        return 0;
      }
      local_28 = (undefined4 *)(local_24 * iVar5 * piVar2[3] + *piVar2 + iVar10 * piVar2[3]);
LAB_00465e44:
      if (local_24 < iVar7) {
        do {
          bVar4 = *pbVar19;
          pbVar19 = pbVar19 + 1;
          bVar22 = (bool)(bVar4 & 1);
          bVar4 = bVar4 >> 1;
          if (bVar22 || bVar4 == 0) {
            if (bVar4 != 0) {
              pbVar19 = pbVar19 + (bVar4 - 1);
            }
            else if (!bVar22) goto code_r0x00465e40;
          }
          pbVar19 = pbVar19 + 1;
        } while( true );
      }
      local_2c = (undefined1 *)((int)local_28 + (iVar6 - iVar10));
      local_30 = (undefined1 *)((int)local_28 + (iVar8 - iVar10));
LAB_00466146:
      if (iVar12 < local_24) {
        return 0;
      }
      if (iVar9 < local_24) {
        return 0;
      }
      puVar21 = local_28;
      if ((bVar16 & 8) == 0) {
        if ((bVar18 & 4) == 0) {
          bVar4 = *pbVar19;
          pbVar19 = pbVar19 + 1;
          bVar22 = (bool)(bVar4 & 1);
          bVar4 = bVar4 >> 1;
          uVar14 = (uint)bVar4;
          if (!bVar22 && bVar4 != 0) goto LAB_00465ebb;
          if (bVar4 != 0) goto LAB_00465eec;
joined_r0x00465e9c:
          if (bVar22) {
LAB_00465ea2:
            do {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              uVar14 = (uint)bVar4;
              if (!bVar22 && bVar4 != 0) goto LAB_00465ebb;
              if (bVar4 != 0) {
LAB_00465eec:
                uVar14 = uVar14 & 0xff;
joined_r0x00465ef3:
                for (; uVar14 != 0; uVar14 = uVar14 - 4) {
                  if ((int)uVar14 < 4) goto LAB_00465f38;
                  *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
                  *(undefined1 *)((int)puVar21 + 1) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[1]);
                  *(undefined1 *)((int)puVar21 + 2) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[2]);
                  *(undefined1 *)((int)puVar21 + 3) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[3]);
                  pbVar19 = pbVar19 + 4;
                  puVar21 = puVar21 + 1;
                }
                goto LAB_00465f47;
              }
            } while (bVar22);
          }
        }
        else {
          bVar4 = *pbVar19;
          pbVar19 = pbVar19 + 1;
          bVar22 = (bool)(bVar4 & 1);
          bVar4 = bVar4 >> 1;
          if (!bVar22 && bVar4 != 0) goto LAB_00466015;
          if (bVar4 != 0) goto LAB_00466064;
joined_r0x00465ff6:
          if (bVar22) {
LAB_00465ffc:
            do {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              if (!bVar22 && bVar4 != 0) goto LAB_00466015;
              if (bVar4 != 0) {
LAB_00466064:
                uVar14 = (uint)bVar4;
LAB_00466067:
                if ((int)local_30 < (int)puVar21) goto LAB_00466128;
                puVar13 = (undefined1 *)((int)puVar21 + ((uVar14 - 1) - (int)local_30));
                uVar15 = ~((int)puVar13 >> 0x1f) & (uint)puVar13;
                for (iVar10 = uVar14 - uVar15; iVar10 != 0; iVar10 = iVar10 + -4) {
                  if (iVar10 < 4) goto LAB_004660c8;
                  *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
                  *(undefined1 *)((int)puVar21 + 1) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[1]);
                  *(undefined1 *)((int)puVar21 + 2) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[2]);
                  *(undefined1 *)((int)puVar21 + 3) =
                       *(undefined1 *)((int)&DAT_0052abe0 + (uint)pbVar19[3]);
                  pbVar19 = pbVar19 + 4;
                  puVar21 = puVar21 + 1;
                }
                goto LAB_004660d7;
              }
            } while (bVar22);
          }
        }
      }
      else {
        bVar4 = *pbVar19;
        pbVar19 = pbVar19 + 1;
        bVar22 = (bool)(bVar4 & 1);
        bVar4 = bVar4 >> 1;
        if (!bVar22 && bVar4 != 0) goto LAB_00465f7f;
        if (bVar4 != 0) goto LAB_00465fb0;
joined_r0x00465f68:
        if (bVar22) {
          do {
            while( true ) {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              if (!bVar22 && bVar4 != 0) goto LAB_00465f7f;
              if (bVar4 == 0) break;
LAB_00465fb0:
              do {
                while( true ) {
                  uVar14 = (uint)bVar4;
                  iVar10 = (int)local_2c - (int)puVar21;
                  if (iVar10 < (int)uVar14) {
                    if (-1 < iVar10) {
                      puVar21 = (undefined4 *)((int)puVar21 + iVar10);
                      uVar14 = uVar14 - iVar10;
                      pbVar19 = pbVar19 + iVar10;
                    }
                    if ((bVar18 & 4) == 0) goto joined_r0x00465ef3;
                    goto LAB_00466067;
                  }
                  puVar21 = (undefined4 *)((int)puVar21 + uVar14);
                  bVar4 = pbVar19[uVar14];
                  pbVar19 = pbVar19 + uVar14 + 1;
                  bVar22 = (bool)(bVar4 & 1);
                  bVar4 = bVar4 >> 1;
                  if (!bVar22 && bVar4 != 0) break;
                  if (bVar4 == 0) goto joined_r0x00465f68;
                }
LAB_00465f7f:
                do {
                  uVar14 = (uint)bVar4;
                  iVar10 = (int)local_2c - (int)puVar21;
                  if (iVar10 < (int)uVar14) {
                    if (-1 < iVar10) {
                      puVar21 = (undefined4 *)((int)puVar21 + iVar10);
                      uVar14 = uVar14 - iVar10;
                    }
                    if ((bVar18 & 4) == 0) goto LAB_00465ebe;
                    goto LAB_00466018;
                  }
                  puVar21 = (undefined4 *)((int)puVar21 + uVar14);
                  pbVar20 = pbVar19 + 1;
                  pbVar19 = pbVar19 + 2;
                  bVar22 = (bool)(*pbVar20 & 1);
                  bVar4 = *pbVar20 >> 1;
                } while (!bVar22 && bVar4 != 0);
                if (!bVar22) goto LAB_00466137;
              } while (bVar4 != 0);
            }
          } while (bVar22);
        }
      }
      goto LAB_00466137;
    }
  }
  return 0xffffffff;
code_r0x00465e40:
  local_28 = (undefined4 *)((int)local_28 + iVar5);
  local_24 = local_24 + 1;
  goto LAB_00465e44;
LAB_004660c8:
  do {
    *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
    pbVar19 = pbVar19 + 1;
    puVar21 = (undefined4 *)((int)puVar21 + 1);
    iVar10 = iVar10 + -1;
  } while (iVar10 != 0);
LAB_004660d7:
  puVar21 = (undefined4 *)((int)puVar21 + uVar15);
  bVar4 = pbVar19[uVar15];
  pbVar19 = pbVar19 + uVar15 + 1;
  bVar22 = (bool)(bVar4 & 1);
  bVar4 = bVar4 >> 1;
  if (bVar22 || bVar4 == 0) {
    if (bVar4 == 0) goto joined_r0x00465ff6;
    goto LAB_00466064;
  }
LAB_00466015:
  do {
    uVar14 = (uint)bVar4;
LAB_00466018:
    if ((int)local_30 < (int)puVar21) goto LAB_00466117;
    puVar13 = (undefined1 *)((int)puVar21 + ((uVar14 - 1) - (int)local_30));
    uVar17 = ~((int)puVar13 >> 0x1f) & (uint)puVar13;
    uVar14 = uVar14 - uVar17;
    pbVar20 = pbVar19 + 1;
    uVar1 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
    for (uVar15 = uVar14 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *(undefined1 *)puVar21 = uVar1;
      puVar21 = (undefined4 *)((int)puVar21 + 1);
    }
    for (uVar14 = uVar14 >> 2; uVar14 != 0; uVar14 = uVar14 - 1) {
      *puVar21 = CONCAT31(CONCAT21(CONCAT11(uVar1,uVar1),uVar1),uVar1);
      puVar21 = puVar21 + 1;
    }
    puVar21 = (undefined4 *)((int)puVar21 + uVar17);
    pbVar19 = pbVar19 + 2;
    bVar22 = (bool)(*pbVar20 & 1);
    bVar4 = *pbVar20 >> 1;
  } while (!bVar22 && bVar4 != 0);
  if (!bVar22) goto LAB_00466137;
  if (bVar4 == 0) goto LAB_00465ffc;
  goto LAB_00466064;
LAB_00466117:
  do {
    pbVar20 = pbVar19 + 1;
    pbVar19 = pbVar19 + 2;
    bVar22 = (bool)(*pbVar20 & 1);
    bVar4 = *pbVar20 >> 1;
  } while (!bVar22 && bVar4 != 0);
  if (bVar22) {
    if (bVar4 == 0) goto LAB_004660ff;
    while( true ) {
      uVar14 = (uint)bVar4;
LAB_00466128:
      bVar4 = pbVar19[uVar14];
      pbVar19 = pbVar19 + uVar14 + 1;
      bVar22 = (bool)(bVar4 & 1);
      bVar4 = bVar4 >> 1;
      bVar23 = bVar4 != 0;
      if (!bVar22 && bVar23) break;
      while (!bVar23) {
        if (!bVar22) goto LAB_00466137;
LAB_004660ff:
        pbVar20 = pbVar19 + 1;
        pbVar19 = pbVar19 + 2;
        bVar22 = (bool)(*pbVar20 & 1);
        bVar4 = *pbVar20 >> 1;
        bVar23 = bVar4 != 0;
        if (!bVar22 && bVar23) goto LAB_00466117;
      }
    }
    goto LAB_00466117;
  }
LAB_00466137:
  local_28 = (undefined4 *)((int)local_28 + iVar5);
  local_2c = local_2c + iVar5;
  local_30 = local_30 + iVar5;
  local_24 = local_24 + 1;
  goto LAB_00466146;
LAB_00465f38:
  do {
    *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
    pbVar19 = pbVar19 + 1;
    puVar21 = (undefined4 *)((int)puVar21 + 1);
    uVar14 = uVar14 - 1;
  } while (uVar14 != 0);
LAB_00465f47:
  bVar4 = *pbVar19;
  pbVar19 = pbVar19 + 1;
  bVar22 = (bool)(bVar4 & 1);
  bVar4 = bVar4 >> 1;
  uVar14 = (uint)bVar4;
  if (bVar22 || bVar4 == 0) {
    if (bVar4 == 0) goto joined_r0x00465e9c;
    goto LAB_00465eec;
  }
LAB_00465ebb:
  do {
    uVar14 = uVar14 & 0xff;
LAB_00465ebe:
    pbVar20 = pbVar19 + 1;
    uVar1 = *(undefined1 *)((int)&DAT_0052abe0 + (uint)*pbVar19);
    for (uVar15 = uVar14 & 3; uVar15 != 0; uVar15 = uVar15 - 1) {
      *(undefined1 *)puVar21 = uVar1;
      puVar21 = (undefined4 *)((int)puVar21 + 1);
    }
    uVar3 = CONCAT21(CONCAT11(uVar1,uVar1),uVar1);
    for (uVar14 = uVar14 >> 2; uVar14 != 0; uVar14 = uVar14 - 1) {
      *puVar21 = CONCAT31(uVar3,uVar1);
      puVar21 = puVar21 + 1;
    }
    pbVar19 = pbVar19 + 2;
    bVar22 = (bool)(*pbVar20 & 1);
    bVar4 = *pbVar20 >> 1;
    uVar14 = CONCAT31(uVar3,bVar4);
  } while (!bVar22 && bVar4 != 0);
  if (!bVar22) goto LAB_00466137;
  if (bVar4 == 0) goto LAB_00465ea2;
  goto LAB_00465eec;
}


// ==== FUN_00465c5e @ 00465c5e

void FUN_00465c5e(undefined4 *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  
  puVar2 = &DAT_0052abe0;
  for (iVar1 = 0x40; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = *param_1;
    param_1 = param_1 + 1;
    puVar2 = puVar2 + 1;
  }
  return;
}


// ==== FUN_00457320 @ 00457320

void __thiscall FUN_00457320(void *this,int *param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int *piVar12;
  int iVar13;
  uint *local_20;
  int local_14;
  
  iVar1 = *param_2;
  if ((((iVar1 < DAT_00516b10) && (iVar2 = param_2[1], iVar2 < DAT_00516b14)) &&
      (iVar3 = param_2[2], 0 < iVar3)) && (iVar4 = param_2[3], 0 < iVar4)) {
    if (99 < *(int *)((int)this + 100)) {
      DAT_0052039c = 1;
      return;
    }
    iVar9 = (iVar4 - iVar2) * (iVar3 - iVar1);
    local_14 = 0;
    if (0 < *(int *)((int)this + 100)) {
      piVar12 = (int *)((int)this + 0x68);
      local_20 = (uint *)((int)this + 0x6a8);
      do {
        if (DAT_005203bc != 0) break;
        uVar5 = *local_20;
        if ((uVar5 & 1) == 0) {
          iVar6 = (piVar12[3] - piVar12[1]) * (piVar12[2] - *piVar12);
          iVar13 = *piVar12;
          if (iVar1 <= *piVar12) {
            iVar13 = iVar1;
          }
          iVar11 = piVar12[1];
          if (iVar2 <= piVar12[1]) {
            iVar11 = iVar2;
          }
          iVar7 = piVar12[2];
          if (piVar12[2] <= iVar3) {
            iVar7 = iVar3;
          }
          iVar8 = piVar12[3];
          if (piVar12[3] <= iVar4) {
            iVar8 = iVar4;
          }
          iVar10 = (iVar8 - iVar11) * (iVar7 - iVar13);
          if (((param_3 & 4) == 0) || (iVar9 != iVar10)) {
            if (((uVar5 & 4) != 0) && (iVar10 - iVar6 == 0)) {
              return;
            }
            if (iVar10 - ((int)(iVar10 + (iVar10 >> 0x1f & 7U)) >> 3) < iVar9 + iVar6) {
              *piVar12 = iVar13;
              piVar12[1] = iVar11;
              piVar12[2] = iVar7;
              piVar12[3] = iVar8;
              return;
            }
          }
          else {
            *local_20 = uVar5 | 1;
          }
        }
        piVar12 = piVar12 + 4;
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
      } while (local_14 < *(int *)((int)this + 100));
    }
    iVar1 = *(int *)((int)this + 100);
    iVar2 = param_2[1];
    *(int *)((int)this + iVar1 * 0x10 + 0x68) = *param_2;
    iVar3 = param_2[2];
    iVar4 = param_2[3];
    *(int *)((int)this + iVar1 * 0x10 + 0x6c) = iVar2;
    *(int *)((int)this + iVar1 * 0x10 + 0x70) = iVar3;
    *(int *)((int)this + iVar1 * 0x10 + 0x74) = iVar4;
    *(uint *)((int)this + *(int *)((int)this + 100) * 4 + 0x6a8) = param_3 & 0xfffffffe;
    *(int *)((int)this + 100) = *(int *)((int)this + 100) + 1;
  }
  return;
}


