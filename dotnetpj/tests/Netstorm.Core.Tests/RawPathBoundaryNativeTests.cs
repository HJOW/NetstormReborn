using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 공통 정지/도착의 현재 owner와 독립 CPU 관측을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 진단 복귀·자기 삭제·실제 finder/retarget을 검증한다.</summary>
public sealed class RawPathBoundaryNativeTests
{
    /// <summary>원문의 명시 raw 입력이 예약/미할당 슬롯도 초기화한다.</summary>
    private static Span<byte> Raw(RawSidPool pool, uint sid) => pool.RawSlot(checked((ushort)sid));
    /// <summary>전체 DWORD 현재 부모를 원문처럼 좁혀 선택한다.</summary>
    private static int Parent(RawPathRuntime path) => (ushort)path.ReadBits(8);
    /// <summary>부호 없는 출력의 참/거짓을 원문0/1로 나타낸다.</summary>
    private static uint Bit(bool value) => value ? 1u : 0u;

    /// <summary>실제 정지2288입력의 현재 checking/대상 재지정/진단 복귀와 같다.</summary>
    [Fact]
    public void NativeHalt2288UsesActualRetarget()
    {
        string[][] rows = Rows("path-halt-parent-hardware-x86.tsv"); Assert.Equal(2288, rows.Length);
        // fixture는 독립CPU 결과이며 현재 응답값만 각 장면에 공급한다.
        foreach (string[] row in rows)
        {
            var pool = new RawSidPool(24000); var path = new RawPathRuntime(OriginalEdition.Patch1078); uint result = U(row[1]), checking = U(row[2]), boss = U(row[3]), form = U(row[4]), genus = U(row[6]), first = U(row[8]), second = U(row[9]), choice = U(row[10]), action = U(row[11]), finds = 0; bool alive = true; string events = "";
            path.SetAttachment(5, 50); path.WriteBits(4, form); path.SetTargets((U(row[7]) & 1) != 0 ? 60u : 0u, (U(row[7]) & 2) != 0 ? 61u : 0u);
            // 원문 현재 부모와 죽은 두 대상의 물리 바이트를 구성한다.
            foreach (uint sid in new uint[] { 50, 51, 52, 60, 61 }) { Raw(pool, sid).Clear(); Raw(pool, sid)[10] = 1; Raw(pool, sid)[11] = sid >= 60 ? (byte)2 : (byte)0; }
            Raw(pool, 50)[40] = (byte)U(row[5]);
            // 실제 새 이동은 원문에서 기존 경로의 수명을 종료할 수 있다.
            void Go(char name, int parent, int target) { events += $"{name},{parent},{target};"; if ((action & 128) != 0) alive = false; }
            var retarget = new RawPathRetarget(pool, new UIntBox { Value = 154 }, new()
            {
                Find = (parent, mask, type, index) => { events += $"F,{parent},{mask},{type},{index};"; return (ushort)(finds++ != 0 ? second : first); },
                ChooseReturn = parent => { events += $"C,{parent};"; return (ushort)choice; }, ChooseGeyser = parent => { events += $"Y,{parent};"; return (ushort)choice; },
                GoReturn = (parent, target) => Go('R', parent, target), GoGeyser = (parent, target) => Go('G', parent, target),
            });
            bool stop = RawPathHaltParent.Run(new RawCheckingFlag(() => checking), new()
            {
                Parent = () => 50, Alive = () => alive,
                Halt = parent => { Assert.Equal(50, parent); events += $"H,50,{result};"; if ((action & 1) != 0) alive = false; if ((action & 2) != 0) checking = 0x80000000; if ((action & 4) != 0) checking = 0; if ((action & 8) != 0) boss = boss != 0 ? 0u : 1u; if ((action & 16) != 0) Raw(pool, 50)[40] ^= 0x20; if ((action & 32) != 0) Raw(pool, 50)[10] = 2; if ((action & 64) != 0) path.WriteBits(8, 51); return result; },
                Assertion = (expression, file, line) => { events += $"A,{line}:{expression}:{file};"; if ((action & 512) != 0) { checking = 0; boss = 0; path.WriteBits(8, 52); } },
                ForcedStop = parent => (Raw(pool, (uint)parent)[40] & 0x20) != 0, MayKill = () => boss != 0 || unchecked((int)form) > 4 && unchecked((int)form) < 15000,
                Kill = () => { events += $"K,{form},0;"; alive = false; }, Boss = () => boss != 0,
                Carrier = parent => ((Raw(pool, (uint)parent)[10] == 1 ? genus : genus ^ 0x30000) & 0x30000) != 0,
                HasTargets = () => path.TargetB != 0 || path.TargetA != 0, Retarget = parent => retarget.Update(path, parent),
            });
            string[] actual = new uint[] { Bit(stop), Bit(!alive), checking, boss, (uint)path.ReadBits(8), form, path.TargetA, path.TargetB, Raw(pool, 50)[10], Raw(pool, 50)[40], finds }.Select(value => value.ToString(System.Globalization.CultureInfo.InvariantCulture)).Append(events).ToArray(); Assert.Equal(row[12..24], actual);
        }
    }
    /// <summary>실제 도착860입력의15현재 필드/원문진단/일반 finder 판정이 같다.</summary>
    [Fact]
    public void NativeArrival860UsesActualFinderAndCurrentDiagnostics()
    {
        string[][] rows = Rows("path-arrival-parent-hardware-x86.tsv"); Assert.Equal(860, rows.Length);
        // 내부 finder 호출수 두 칸은 결과/외부 순서와 구별하고 비교하지 않는다.
        foreach (string[] row in rows)
        {
            var pool = new RawSidPool(24000); var hash = new RawSquidHash(); var path = new RawPathRuntime(OriginalEdition.Patch1078); OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); var spots = new byte[65536];
            uint checking = U(row[1]), parent = U(row[2]), capacity = U(row[4]), halted = U(row[6]), result = U(row[7]), deleted = U(row[8]), blockers = U(row[9]), mutation = U(row[10]), xb = U(row[13]), yb = U(row[14]), robustCalls = 0; bool alive = true; string events = "";
            path.WriteBits(4, U(row[3])); path.WriteBits(8, parent); path.SetTargets(60, 61); path.WriteBits(path.Layout.Finished, U(row[12])); path.DeathWatchToken = U(row[11]);
            // 예약0/상한128을 포함한 유효 물리 입력을 논리 capacity와 구별한다.
            foreach (uint sid in new uint[] { 0, 5, 50, 70, 71, 90, 128 }) { Raw(pool, sid).Clear(); Raw(pool, sid)[10] = sid == 70 ? (byte)2 : sid == 71 ? (byte)3 : (byte)1; Raw(pool, sid)[34] = 2; Put(Raw(pool, sid), 14, 0x3fa00000); Put(Raw(pool, sid), 18, 0xc0200000); }
            Raw(pool, parent)[11] = (byte)U(row[5]); types[3].Flags2 = 0x200; spots[21 * 256 + 11] = blockers != 0 ? (byte)8 : (byte)0;
            // 실제 finder의 후보 연쇄와 좌표를 CPU와 같게 공급한다.
            foreach (uint sid in new uint[] { 70, 71 }) { Put(Raw(pool, sid), 14, xb); Put(Raw(pool, sid), 18, yb); }
            hash.Cell(0, (int)BitConverter.UInt32BitsToSingle(xb), (int)BitConverter.UInt32BitsToSingle(yb)) = (ushort)(blockers == 4 ? 71 : blockers >= 2 ? 70 : 0); BinaryPrimitives.WriteUInt16LittleEndian(Raw(pool, 70)[4..], (ushort)(blockers == 3 ? 71 : 0));
            // 원문 진단/가상 복귀가 저장 부모의 현재 metadata를 바꾼다.
            void Change(uint x, uint y, byte owner) { Put(Raw(pool, parent), 14, x); Put(Raw(pool, parent), 18, y); Raw(pool, parent)[10] = 2; Raw(pool, parent)[34] = owner; }
            // 정상 복귀 뒤 다음 진단은 새 현재 checking/상한/부모를 읽어야 한다.
            void Diagnostic(string expression, string file, uint line)
            {
                events += $"A,{line}:{expression}:{file};";
                if (mutation is 10 or 11) { checking = mutation == 10 ? 0u : 0x80000000; path.WriteBits(4, 5); path.WriteBits(8, 50); path.DeathWatchToken = 0; path.SetTargets(95, 96); Raw(pool, parent)[11] = 0; Change(0x40700000, 0x40900000, 7); }
                if (mutation == 12 && line == 0x69) { path.WriteBits(4, 5); path.WriteBits(8, 90); capacity = 64; }
            }
            RawPathArrivalParent.Run(new RawCheckingFlag(() => checking), new()
            {
                Parent = () => Parent(path), ValidAlive = sid => sid != 0 && sid < capacity && (Raw(pool, (uint)sid)[11] & 3) == 0, Alive = () => alive,
                Step = () => { events += $"S,{xb},{yb};"; if (mutation == 1) { checking = 0x80000000; path.WriteBits(4, 128); } return new(BitConverter.UInt32BitsToSingle(xb), BitConverter.UInt32BitsToSingle(yb)); },
                Unpop = sid => events += $"U,{sid},0;", Pop = (sid, p) => { uint x = BitConverter.SingleToUInt32Bits(p.X), y = BitConverter.SingleToUInt32Bits(p.Y); events += $"P,{sid},{x},{y},0;"; Put(Raw(pool, (uint)sid), 14, x); Put(Raw(pool, (uint)sid), 18, y); if (mutation == 2) { checking = 0; path.WriteBits(4, 128); } if (mutation == 3) Raw(pool, (uint)sid)[11] = 3; },
                Assertion = Diagnostic, Robust = () => { robustCalls++; events += $"Q,{path.ReadBits(4)},{path.ReadBits(8)};"; Assert.True(RawPathArrivalParent.Robust(new() { Form = () => (uint)path.ReadBits(4), Parent = () => (uint)path.ReadBits(8), Capacity = () => capacity, Assertion = Diagnostic })); },
                Blocked = point => RawPathArrivalParent.Blocked(pool, hash, types, spots, 0x200, point),
                Halt = () => { events += $"T,{halted};"; path.DeathWatchToken = 0; if (mutation == 4) checking = 0; if (mutation == 5) checking = 0x80000000; if (mutation == 6) path.DeathWatchToken = 0x12345678; if (mutation == 7) { path.SetTargets(85, 86); Change(0x40700000, 0x40900000, 7); } return halted != 0; },
                Finished = path.SetFinished, DeadOrFree = sid => (Raw(pool, (uint)sid)[11] & 3) != 0, WatchPresent = () => path.DeathWatchToken != 0, ArmWatch = () => path.DeathWatchToken = 1, ClearWatch = () => path.DeathWatchToken = 0,
                Arrival = sid => { events += $"R,{sid},{path.TargetA},{path.TargetB},{result};"; if (mutation == 8) { checking = 0; Change(0x40b00000, 0x40d00000, 6); } if (mutation == 9) { checking = 0x80000000; path.WriteBits(4, 128); } if (deleted != 0) alive = false; return result; },
                Kill = () => { events += $"K,{path.ReadBits(4)},0;"; alive = false; }, Clear = sid => { ReadOnlySpan<byte> raw = Raw(pool, (uint)sid); events += $"C,{Get(raw, 14)},{Get(raw, 18)},{raw[10]},{raw[34]};"; },
            });
            uint[] fields = [Bit(!alive), checking, (uint)path.ReadBits(4), (uint)path.ReadBits(8), capacity, Raw(pool, parent)[11], (uint)path.ReadBits(path.Layout.Finished), Bit(path.DeathWatchToken != 0), path.TargetA, path.TargetB, Get(Raw(pool, parent), 14), Get(Raw(pool, parent), 18), Raw(pool, parent)[10], Raw(pool, parent)[34], robustCalls];
            string expectedEvents = string.Concat(row[32].Split(';').Where(part => part.Length != 0 && part[0] != 'F' && part[0] != 'N').Select(part => part + ';')); Assert.Equal(row[15..30], fields.Select(value => value.ToString(System.Globalization.CultureInfo.InvariantCulture)).ToArray()); Assert.Equal(expectedEvents, events);
        }
    }
    /// <summary>미선택 함수 누락을 허용하고 진단 실패 뒤 이미 일어난 효과를 보존한다.</summary>
    [Fact]
    public void SelectedHaltDiagnosticAndCheckingOwnerSurviveDeletion()
    {
        uint checking = 0; bool alive = true; var hooks = new PathHaltParentHooks { Parent = () => 50, Halt = _ => { alive = false; return 0; }, Alive = () => alive };
        Assert.True(RawPathHaltParent.Run(new RawCheckingFlag(() => checking), hooks)); checking = 0x80000000;
        Assert.Throws<InvalidOperationException>(() => RawPathHaltParent.Run(new RawCheckingFlag(() => checking), hooks));
        hooks.Assertion = (expression, file, line) => { Assert.Equal("result == haltALREADY_DEAD", expression); Assert.Equal(@"\Ns\O\Pathprocess.cpp", file); Assert.Equal(0x2d5u, line); checking = 0; };
        Assert.True(RawPathHaltParent.Run(new RawCheckingFlag(() => checking), hooks)); Assert.Equal(0u, checking);
    }
}
