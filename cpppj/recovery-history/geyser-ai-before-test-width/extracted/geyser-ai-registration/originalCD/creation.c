// ==== FUN_004d8670 @ 004d8670

void __cdecl FUN_004d8670(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  float10 fVar9;
  float10 fVar10;
  int local_20;
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [8];
  undefined1 local_8 [8];
  
  local_1c = 0xac;
  do {
    if ((DAT_0050f834 + local_1c != 0) &&
       (iVar1 = *(int *)(DAT_0050f834 + local_1c + 0xa8), iVar1 != 0)) {
      if (param_2 == 0) {
        iVar5 = 0;
        iVar4 = 0;
        if (0 < *(int *)(iVar1 + 0x32c)) {
          iVar7 = *(int *)(iVar1 + 0x324);
          iVar8 = *(int *)(iVar1 + 0x32c);
          do {
            if (*(int *)(iVar7 + iVar4 * 4) == param_1) {
              iVar5 = iVar5 + 1;
            }
            else {
              *(undefined4 *)(iVar7 + (iVar4 - iVar5) * 4) = *(undefined4 *)(iVar7 + iVar4 * 4);
            }
            iVar4 = iVar4 + 1;
          } while (iVar4 < iVar8);
        }
        iVar4 = 0;
        *(int *)(iVar1 + 0x32c) = *(int *)(iVar1 + 0x32c) - iVar5;
        local_20 = 0;
        if (0 < *(int *)(iVar1 + 0x338)) {
          piVar6 = *(int **)(iVar1 + 0x330);
          iVar5 = *(int *)(iVar1 + 0x338);
          do {
            if (*piVar6 == param_1) {
              local_20 = local_20 + 1;
            }
            else {
              *(int *)(*(int *)(iVar1 + 0x330) + (iVar4 - local_20) * 4) = *piVar6;
            }
            piVar6 = piVar6 + 1;
            iVar4 = iVar4 + 1;
          } while (iVar4 < iVar5);
        }
        iVar4 = 0;
        iVar5 = 0;
        *(int *)(iVar1 + 0x338) = *(int *)(iVar1 + 0x338) - local_20;
        if (0 < *(int *)(iVar1 + 0x344)) {
          iVar7 = *(int *)(iVar1 + 0x33c);
          iVar8 = *(int *)(iVar1 + 0x344);
          do {
            if (*(int *)(iVar7 + iVar5 * 4) == param_1) {
              iVar4 = iVar4 + 1;
            }
            else {
              *(undefined4 *)(iVar7 + (iVar5 - iVar4) * 4) = *(undefined4 *)(iVar7 + iVar5 * 4);
            }
            iVar5 = iVar5 + 1;
          } while (iVar5 < iVar8);
        }
        iVar5 = 0;
        *(int *)(iVar1 + 0x344) = *(int *)(iVar1 + 0x344) - iVar4;
        iVar4 = 0;
        if (0 < *(int *)(iVar1 + 0x350)) {
          iVar7 = *(int *)(iVar1 + 0x348);
          iVar8 = *(int *)(iVar1 + 0x350);
          do {
            if (*(int *)(iVar7 + iVar5 * 4) == param_1) {
              iVar4 = iVar4 + 1;
            }
            else {
              *(undefined4 *)(iVar7 + (iVar5 - iVar4) * 4) = *(undefined4 *)(iVar7 + iVar5 * 4);
            }
            iVar5 = iVar5 + 1;
          } while (iVar5 < iVar8);
        }
        iVar7 = 0;
        iVar5 = 0;
        *(int *)(iVar1 + 0x350) = *(int *)(iVar1 + 0x350) - iVar4;
        if (0 < *(int *)(iVar1 + 0x35c)) {
          iVar4 = *(int *)(iVar1 + 0x354);
          iVar8 = *(int *)(iVar1 + 0x35c);
          do {
            if (*(int *)(iVar4 + iVar5 * 4) == param_1) {
              iVar7 = iVar7 + 1;
            }
            else {
              *(undefined4 *)(iVar4 + (iVar5 - iVar7) * 4) = *(undefined4 *)(iVar4 + iVar5 * 4);
            }
            iVar5 = iVar5 + 1;
          } while (iVar5 < iVar8);
        }
        *(int *)(iVar1 + 0x35c) = *(int *)(iVar1 + 0x35c) - iVar7;
      }
      else {
        iVar4 = param_1 * 0x24 + DAT_005395dc;
        local_18 = *(undefined4 *)(iVar4 + 0xe);
        local_14 = *(undefined4 *)(iVar4 + 0x12);
        puVar3 = (undefined4 *)FUN_004dd430(iVar1,local_8);
        iVar7 = 1;
        iVar8 = 0xac;
        fVar9 = (float10)FUN_004deee0(&local_18,*puVar3,puVar3[1]);
        iVar4 = DAT_005395dc;
        iVar5 = DAT_0050f834;
        do {
          if (((iVar7 != *(int *)(iVar1 + 0x260)) &&
              ((&DAT_0050f6e0)[*(int *)(iVar1 + 0x260) * 9 + iVar7] != 0)) &&
             (iVar2 = *(int *)(iVar8 + 0x74 + iVar5), iVar2 != 0)) {
            puVar3 = (undefined4 *)FUN_004aef30(iVar4 + iVar2 * 0x24,local_10);
            fVar10 = (float10)FUN_004deee0(&local_18,*puVar3,puVar3[1]);
            iVar4 = DAT_005395dc;
            iVar5 = DAT_0050f834;
            if (fVar10 < (float10)(float)fVar9) {
              FUN_004da360(iVar1,0x1d,s_Not_closer_to_geyser__d__005410ac,param_1);
              goto LAB_004d88d8;
            }
          }
          iVar8 = iVar8 + 0xac;
          iVar7 = iVar7 + 1;
        } while (iVar8 < 0x60c);
        if (*(int *)(iVar1 + 0x32c) < *(int *)(iVar1 + 0x328)) {
          iVar4 = *(int *)(iVar1 + 0x32c) + 1;
          *(int *)(iVar1 + 0x32c) = iVar4;
          *(int *)(*(int *)(iVar1 + 0x324) + -4 + iVar4 * 4) = param_1;
        }
        FUN_004da3d0(iVar1,param_1);
      }
    }
LAB_004d88d8:
    local_1c = local_1c + 0xac;
    if (0x60b < local_1c) {
      return;
    }
  } while( true );
}


