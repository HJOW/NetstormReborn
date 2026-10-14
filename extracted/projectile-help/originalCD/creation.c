// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __thiscall FUN_004abac0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_0043f950 @ 0043f950

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0043f950(void *this)

{
  float fVar1;
  
  fVar1 = *(float *)this;
  if (*(float *)this <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *(float *)this = fVar1;
  fVar1 = *(float *)((int)this + 4);
  if (*(float *)((int)this + 4) <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *(float *)((int)this + 4) = fVar1;
  fVar1 = *(float *)this;
  if (_DAT_00501768 <= *(float *)this) {
    fVar1 = _DAT_00501768;
  }
  *(float *)this = fVar1;
  fVar1 = *(float *)((int)this + 4);
  if (_DAT_00501768 <= *(float *)((int)this + 4)) {
    fVar1 = _DAT_00501768;
  }
  *(float *)((int)this + 4) = fVar1;
  return;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


