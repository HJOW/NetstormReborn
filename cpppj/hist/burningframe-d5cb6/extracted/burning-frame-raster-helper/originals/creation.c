// ==== FUN_00401d92 @ 00401d92

undefined4 FUN_00401d92(int *param_1,int param_2,int param_3,int param_4,int param_5)

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
        FUN_004021b5(param_1,param_2,param_4 - param_1[1],param_5 - param_1[2],iVar4);
        return 0;
      }
      local_28 = (byte *)(local_24 * iVar4 * piVar1[3] + *piVar1 + iVar9 * piVar1[3]);
LAB_00401f57:
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
            else if (!bVar20) goto code_r0x00401f53;
          }
          pbVar17 = pbVar17 + 1;
        } while( true );
      }
      local_2c = local_28 + (iVar5 - iVar9);
      local_30 = local_28 + (iVar7 - iVar9);
LAB_00402185:
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
          if (!bVar20 && bVar3 != 0) goto LAB_00401fc2;
          if (bVar3 != 0) goto LAB_00401feb;
joined_r0x00401fab:
          if (bVar20) {
            do {
              while( true ) {
                pbVar18 = pbVar17 + 1;
                pbVar19 = pbVar19 + *pbVar17;
                pbVar17 = pbVar17 + 2;
                bVar20 = (bool)(*pbVar18 & 1);
                bVar3 = *pbVar18 >> 1;
                uVar12 = (uint)bVar3;
                if (!bVar20 && bVar3 != 0) goto LAB_00401fc2;
                if (bVar3 == 0) break;
LAB_00401feb:
                do {
                  while( true ) {
                    uVar12 = uVar12 & 0xff;
LAB_00401fee:
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
                    if (bVar3 == 0) goto joined_r0x00401fab;
                  }
LAB_00401fc2:
                  do {
                    uVar12 = uVar12 & 0xff;
LAB_00401fc5:
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
                  if (!bVar20) goto LAB_00402176;
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
          if (!bVar20 && bVar3 != 0) goto LAB_004020b8;
          if (bVar3 != 0) goto LAB_004020fb;
joined_r0x0040209d:
          if (bVar20) {
            do {
              while( true ) {
                pbVar18 = pbVar17 + 1;
                pbVar19 = pbVar19 + *pbVar17;
                pbVar17 = pbVar17 + 2;
                bVar20 = (bool)(*pbVar18 & 1);
                bVar3 = *pbVar18 >> 1;
                if (!bVar20 && bVar3 != 0) goto LAB_004020b8;
                if (bVar3 == 0) break;
LAB_004020fb:
                do {
                  while( true ) {
                    uVar12 = (uint)bVar3;
LAB_004020fe:
                    if ((int)local_30 < (int)pbVar19) goto LAB_00402167;
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
                    if (bVar3 == 0) goto joined_r0x0040209d;
                  }
LAB_004020b8:
                  do {
                    uVar12 = (uint)bVar3;
LAB_004020bb:
                    if ((int)local_30 < (int)pbVar19) goto LAB_00402156;
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
                  if (!bVar20) goto LAB_00402176;
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
        if (!bVar20 && bVar3 != 0) goto LAB_0040202a;
        if (bVar3 != 0) goto LAB_0040205b;
joined_r0x00402013:
        if (bVar20) {
          do {
            while( true ) {
              pbVar18 = pbVar17 + 1;
              pbVar19 = pbVar19 + *pbVar17;
              pbVar17 = pbVar17 + 2;
              bVar20 = (bool)(*pbVar18 & 1);
              bVar3 = *pbVar18 >> 1;
              if (!bVar20 && bVar3 != 0) goto LAB_0040202a;
              if (bVar3 == 0) break;
LAB_0040205b:
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
                    if ((bVar16 & 4) == 0) goto LAB_00401fee;
                    goto LAB_004020fe;
                  }
                  pbVar19 = pbVar19 + uVar12;
                  bVar3 = pbVar17[uVar12];
                  pbVar17 = pbVar17 + uVar12 + 1;
                  bVar20 = (bool)(bVar3 & 1);
                  bVar3 = bVar3 >> 1;
                  if (!bVar20 && bVar3 != 0) break;
                  if (bVar3 == 0) goto joined_r0x00402013;
                }
LAB_0040202a:
                do {
                  uVar12 = (uint)bVar3;
                  iVar9 = (int)local_2c - (int)pbVar19;
                  if (iVar9 < (int)uVar12) {
                    if (-1 < iVar9) {
                      pbVar19 = pbVar19 + iVar9;
                      uVar12 = uVar12 - iVar9;
                    }
                    if ((bVar16 & 4) == 0) goto LAB_00401fc5;
                    goto LAB_004020bb;
                  }
                  pbVar19 = pbVar19 + uVar12;
                  pbVar18 = pbVar17 + 1;
                  pbVar17 = pbVar17 + 2;
                  bVar20 = (bool)(*pbVar18 & 1);
                  bVar3 = *pbVar18 >> 1;
                } while (!bVar20 && bVar3 != 0);
                if (!bVar20) goto LAB_00402176;
              } while (bVar3 != 0);
            }
          } while (bVar20);
        }
      }
      goto LAB_00402176;
    }
  }
  return 0xffffffff;
code_r0x00401f53:
  local_28 = local_28 + iVar4;
  local_24 = local_24 + 1;
  goto LAB_00401f57;
LAB_00402156:
  do {
    pbVar19 = pbVar17 + 1;
    pbVar17 = pbVar17 + 2;
    bVar20 = (bool)(*pbVar19 & 1);
    bVar3 = *pbVar19 >> 1;
  } while (!bVar20 && bVar3 != 0);
  if (bVar20) {
    if (bVar3 == 0) goto LAB_0040213e;
    while( true ) {
      uVar12 = (uint)bVar3;
LAB_00402167:
      bVar3 = pbVar17[uVar12];
      pbVar17 = pbVar17 + uVar12 + 1;
      bVar20 = (bool)(bVar3 & 1);
      bVar3 = bVar3 >> 1;
      bVar21 = bVar3 != 0;
      if (!bVar20 && bVar21) break;
      while (!bVar21) {
        if (!bVar20) goto LAB_00402176;
LAB_0040213e:
        pbVar19 = pbVar17 + 1;
        pbVar17 = pbVar17 + 2;
        bVar20 = (bool)(*pbVar19 & 1);
        bVar3 = *pbVar19 >> 1;
        bVar21 = bVar3 != 0;
        if (!bVar20 && bVar21) goto LAB_00402156;
      }
    }
    goto LAB_00402156;
  }
LAB_00402176:
  local_28 = local_28 + iVar4;
  local_2c = local_2c + iVar4;
  local_30 = local_30 + iVar4;
  local_24 = local_24 + 1;
  goto LAB_00402185;
}


// ==== FUN_0040229f @ 0040229f

undefined4 FUN_0040229f(int *param_1,int param_2,int param_3,int param_4,int param_5)

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
        FUN_00402796(param_1,param_2,param_4 - param_1[1],param_5 - param_1[2],iVar5);
        return 0;
      }
      local_28 = (undefined4 *)(local_24 * iVar5 * piVar2[3] + *piVar2 + iVar10 * piVar2[3]);
LAB_00402464:
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
            else if (!bVar22) goto code_r0x00402460;
          }
          pbVar19 = pbVar19 + 1;
        } while( true );
      }
      local_2c = (undefined1 *)((int)local_28 + (iVar6 - iVar10));
      local_30 = (undefined1 *)((int)local_28 + (iVar8 - iVar10));
LAB_00402766:
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
          if (!bVar22 && bVar4 != 0) goto LAB_004024db;
          if (bVar4 != 0) goto LAB_0040250c;
joined_r0x004024bc:
          if (bVar22) {
LAB_004024c2:
            do {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              uVar14 = (uint)bVar4;
              if (!bVar22 && bVar4 != 0) goto LAB_004024db;
              if (bVar4 != 0) {
LAB_0040250c:
                uVar14 = uVar14 & 0xff;
joined_r0x00402513:
                for (; uVar14 != 0; uVar14 = uVar14 - 4) {
                  if ((int)uVar14 < 4) goto LAB_00402558;
                  *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
                  *(undefined1 *)((int)puVar21 + 1) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[1]);
                  *(undefined1 *)((int)puVar21 + 2) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[2]);
                  *(undefined1 *)((int)puVar21 + 3) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[3]);
                  pbVar19 = pbVar19 + 4;
                  puVar21 = puVar21 + 1;
                }
                goto LAB_00402567;
              }
            } while (bVar22);
          }
        }
        else {
          bVar4 = *pbVar19;
          pbVar19 = pbVar19 + 1;
          bVar22 = (bool)(bVar4 & 1);
          bVar4 = bVar4 >> 1;
          if (!bVar22 && bVar4 != 0) goto LAB_00402635;
          if (bVar4 != 0) goto LAB_00402684;
joined_r0x00402616:
          if (bVar22) {
LAB_0040261c:
            do {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              if (!bVar22 && bVar4 != 0) goto LAB_00402635;
              if (bVar4 != 0) {
LAB_00402684:
                uVar14 = (uint)bVar4;
LAB_00402687:
                if ((int)local_30 < (int)puVar21) goto LAB_00402748;
                puVar13 = (undefined1 *)((int)puVar21 + ((uVar14 - 1) - (int)local_30));
                uVar15 = ~((int)puVar13 >> 0x1f) & (uint)puVar13;
                for (iVar10 = uVar14 - uVar15; iVar10 != 0; iVar10 = iVar10 + -4) {
                  if (iVar10 < 4) goto LAB_004026e8;
                  *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
                  *(undefined1 *)((int)puVar21 + 1) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[1]);
                  *(undefined1 *)((int)puVar21 + 2) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[2]);
                  *(undefined1 *)((int)puVar21 + 3) =
                       *(undefined1 *)((int)&DAT_0052eac0 + (uint)pbVar19[3]);
                  pbVar19 = pbVar19 + 4;
                  puVar21 = puVar21 + 1;
                }
                goto LAB_004026f7;
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
        if (!bVar22 && bVar4 != 0) goto LAB_0040259f;
        if (bVar4 != 0) goto LAB_004025d0;
joined_r0x00402588:
        if (bVar22) {
          do {
            while( true ) {
              pbVar20 = pbVar19 + 1;
              puVar21 = (undefined4 *)((int)puVar21 + (uint)*pbVar19);
              pbVar19 = pbVar19 + 2;
              bVar22 = (bool)(*pbVar20 & 1);
              bVar4 = *pbVar20 >> 1;
              if (!bVar22 && bVar4 != 0) goto LAB_0040259f;
              if (bVar4 == 0) break;
LAB_004025d0:
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
                    if ((bVar18 & 4) == 0) goto joined_r0x00402513;
                    goto LAB_00402687;
                  }
                  puVar21 = (undefined4 *)((int)puVar21 + uVar14);
                  bVar4 = pbVar19[uVar14];
                  pbVar19 = pbVar19 + uVar14 + 1;
                  bVar22 = (bool)(bVar4 & 1);
                  bVar4 = bVar4 >> 1;
                  if (!bVar22 && bVar4 != 0) break;
                  if (bVar4 == 0) goto joined_r0x00402588;
                }
LAB_0040259f:
                do {
                  uVar14 = (uint)bVar4;
                  iVar10 = (int)local_2c - (int)puVar21;
                  if (iVar10 < (int)uVar14) {
                    if (-1 < iVar10) {
                      puVar21 = (undefined4 *)((int)puVar21 + iVar10);
                      uVar14 = uVar14 - iVar10;
                    }
                    if ((bVar18 & 4) == 0) goto LAB_004024de;
                    goto LAB_00402638;
                  }
                  puVar21 = (undefined4 *)((int)puVar21 + uVar14);
                  pbVar20 = pbVar19 + 1;
                  pbVar19 = pbVar19 + 2;
                  bVar22 = (bool)(*pbVar20 & 1);
                  bVar4 = *pbVar20 >> 1;
                } while (!bVar22 && bVar4 != 0);
                if (!bVar22) goto LAB_00402757;
              } while (bVar4 != 0);
            }
          } while (bVar22);
        }
      }
      goto LAB_00402757;
    }
  }
  return 0xffffffff;
code_r0x00402460:
  local_28 = (undefined4 *)((int)local_28 + iVar5);
  local_24 = local_24 + 1;
  goto LAB_00402464;
LAB_004026e8:
  do {
    *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
    pbVar19 = pbVar19 + 1;
    puVar21 = (undefined4 *)((int)puVar21 + 1);
    iVar10 = iVar10 + -1;
  } while (iVar10 != 0);
LAB_004026f7:
  puVar21 = (undefined4 *)((int)puVar21 + uVar15);
  bVar4 = pbVar19[uVar15];
  pbVar19 = pbVar19 + uVar15 + 1;
  bVar22 = (bool)(bVar4 & 1);
  bVar4 = bVar4 >> 1;
  if (bVar22 || bVar4 == 0) {
    if (bVar4 == 0) goto joined_r0x00402616;
    goto LAB_00402684;
  }
LAB_00402635:
  do {
    uVar14 = (uint)bVar4;
LAB_00402638:
    if ((int)local_30 < (int)puVar21) goto LAB_00402737;
    puVar13 = (undefined1 *)((int)puVar21 + ((uVar14 - 1) - (int)local_30));
    uVar17 = ~((int)puVar13 >> 0x1f) & (uint)puVar13;
    uVar14 = uVar14 - uVar17;
    pbVar20 = pbVar19 + 1;
    uVar1 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
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
  if (!bVar22) goto LAB_00402757;
  if (bVar4 == 0) goto LAB_0040261c;
  goto LAB_00402684;
LAB_00402737:
  do {
    pbVar20 = pbVar19 + 1;
    pbVar19 = pbVar19 + 2;
    bVar22 = (bool)(*pbVar20 & 1);
    bVar4 = *pbVar20 >> 1;
  } while (!bVar22 && bVar4 != 0);
  if (bVar22) {
    if (bVar4 == 0) goto LAB_0040271f;
    while( true ) {
      uVar14 = (uint)bVar4;
LAB_00402748:
      bVar4 = pbVar19[uVar14];
      pbVar19 = pbVar19 + uVar14 + 1;
      bVar22 = (bool)(bVar4 & 1);
      bVar4 = bVar4 >> 1;
      bVar23 = bVar4 != 0;
      if (!bVar22 && bVar23) break;
      while (!bVar23) {
        if (!bVar22) goto LAB_00402757;
LAB_0040271f:
        pbVar20 = pbVar19 + 1;
        pbVar19 = pbVar19 + 2;
        bVar22 = (bool)(*pbVar20 & 1);
        bVar4 = *pbVar20 >> 1;
        bVar23 = bVar4 != 0;
        if (!bVar22 && bVar23) goto LAB_00402737;
      }
    }
    goto LAB_00402737;
  }
LAB_00402757:
  local_28 = (undefined4 *)((int)local_28 + iVar5);
  local_2c = local_2c + iVar5;
  local_30 = local_30 + iVar5;
  local_24 = local_24 + 1;
  goto LAB_00402766;
LAB_00402558:
  do {
    *(undefined1 *)puVar21 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
    pbVar19 = pbVar19 + 1;
    puVar21 = (undefined4 *)((int)puVar21 + 1);
    uVar14 = uVar14 - 1;
  } while (uVar14 != 0);
LAB_00402567:
  bVar4 = *pbVar19;
  pbVar19 = pbVar19 + 1;
  bVar22 = (bool)(bVar4 & 1);
  bVar4 = bVar4 >> 1;
  uVar14 = (uint)bVar4;
  if (bVar22 || bVar4 == 0) {
    if (bVar4 == 0) goto joined_r0x004024bc;
    goto LAB_0040250c;
  }
LAB_004024db:
  do {
    uVar14 = uVar14 & 0xff;
LAB_004024de:
    pbVar20 = pbVar19 + 1;
    uVar1 = *(undefined1 *)((int)&DAT_0052eac0 + (uint)*pbVar19);
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
  if (!bVar22) goto LAB_00402757;
  if (bVar4 == 0) goto LAB_004024c2;
  goto LAB_0040250c;
}


// ==== FUN_0040227e @ 0040227e

void FUN_0040227e(undefined4 *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  
  puVar2 = &DAT_0052eac0;
  for (iVar1 = 0x40; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = *param_1;
    param_1 = param_1 + 1;
    puVar2 = puVar2 + 1;
  }
  return;
}


// ==== FUN_00463f80 @ 00463f80

undefined4 __thiscall FUN_00463f80(void *this,int param_2,int param_3,int param_4,int param_5)

{
  if (*(int *)this <= *(int *)((int)this + 8)) {
    if (*(int *)((int)this + 4) <= *(int *)((int)this + 0xc)) {
      if ((((param_2 <= param_4) && (param_3 <= param_5)) && (*(int *)this < param_4)) &&
         (((param_2 < *(int *)((int)this + 8) && (*(int *)((int)this + 4) < param_5)) &&
          (param_3 < *(int *)((int)this + 0xc))))) {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}


// ==== FUN_00445290 @ 00445290

void __thiscall FUN_00445290(void *this,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *(int *)this;
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  iVar2 = param_2;
  if ((param_2 <= iVar3) && (iVar2 = iVar1, param_4 <= iVar1)) {
    iVar2 = param_4;
  }
  *(int *)this = iVar2;
  iVar1 = *(int *)((int)this + 4);
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  iVar2 = param_3;
  if ((param_3 <= iVar3) && (iVar2 = iVar1, param_5 <= iVar1)) {
    iVar2 = param_5;
  }
  *(int *)((int)this + 4) = iVar2;
  iVar1 = *(int *)((int)this + 8);
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  if ((param_2 <= iVar3) && (param_2 = iVar1, param_4 <= iVar1)) {
    param_2 = param_4;
  }
  *(int *)((int)this + 8) = param_2;
  iVar1 = *(int *)((int)this + 0xc);
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  if (param_3 <= iVar3) {
    if (param_5 <= iVar1) {
      *(int *)((int)this + 0xc) = param_5;
      return;
    }
    *(int *)((int)this + 0xc) = iVar1;
    return;
  }
  *(int *)((int)this + 0xc) = param_3;
  return;
}


// ==== FUN_00497580 @ 00497580

void __thiscall FUN_00497580(void *this,int *param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  uint *local_20;
  int local_14;
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  if ((((*param_2 < DAT_005c78f0) && (param_2[1] < DAT_005c78f4)) && (0 < param_2[2])) &&
     (0 < param_2[3])) {
    if (99 < *(int *)((int)this + 100)) {
      DAT_0059a8b0 = 1;
      return;
    }
    iVar2 = (param_2[3] - param_2[1]) * (param_2[2] - *param_2);
    local_14 = 0;
    if (0 < *(int *)((int)this + 100)) {
      local_20 = (uint *)((int)this + 0x6a8);
      piVar5 = (int *)((int)this + 0x68);
      do {
        if (DAT_0059a8d0 != 0) break;
        uVar1 = *local_20;
        if ((uVar1 & 1) == 0) {
          iVar4 = (piVar5[2] - *piVar5) * (piVar5[3] - piVar5[1]);
          local_10 = *piVar5;
          local_c = piVar5[1];
          local_8 = piVar5[2];
          local_4 = piVar5[3];
          FUN_004834c0(&local_10,*param_2,param_2[1],param_2[2],param_2[3]);
          iVar3 = (local_4 - local_c) * (local_8 - local_10);
          if (((param_3 & 4) == 0) || (iVar3 - iVar2 != 0)) {
            if (((uVar1 & 4) != 0) && (iVar3 - iVar4 == 0)) {
              return;
            }
            if (iVar3 - ((int)(iVar3 + (iVar3 >> 0x1f & 7U)) >> 3) < iVar4 + iVar2) {
              FUN_004970a0(piVar5,local_10,local_c,local_8,local_4);
              return;
            }
          }
          else {
            *local_20 = uVar1 | 1;
          }
        }
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
        piVar5 = piVar5 + 4;
      } while (local_14 < *(int *)((int)this + 100));
    }
    piVar5 = (int *)(*(int *)((int)this + 100) * 0x10 + 0x68 + (int)this);
    *piVar5 = *param_2;
    piVar5[1] = param_2[1];
    piVar5[2] = param_2[2];
    piVar5[3] = param_2[3];
    *(uint *)((int)this + *(int *)((int)this + 100) * 4 + 0x6a8) = param_3 & 0xfffffffe;
    *(int *)((int)this + 100) = *(int *)((int)this + 100) + 1;
  }
  return;
}


