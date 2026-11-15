using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 현재 다리 수명·낙하 포장의 원본 관측을 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj/tests/BridgeLifecycleTests.cpp의 원본 효과·순서·전체 슬롯 불변 검사를 옮긴다.</summary>
public sealed class BridgeLifecycleCurrentTests
{
    /// <summary>독립 관측이 사용하는 여섯 합성 객체 번호다.</summary>
    private static readonly int[] ObjectIds = [50, 60, 61, 62, 63, 64];

    /// <summary>실제 원본 입력의 풀·현재 타입과 명시 탐색/외부 효과를 묶는다.</summary>
    private sealed class Scene
    {
        /// <summary>이 검사만 소유하는 실제 관리 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>현재 후보의 genus를 읽는 원본 입력 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 256).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>호출 순서대로 모은 관측 문자열이다.</summary>
        public readonly List<string> Events = [];
        /// <summary>원본 실행기에 공급한 탐색 번호 목록이다.</summary>
        private int[] _selected = [];
        /// <summary>다음 탐색 후보의 위치다.</summary>
        private int _cursor;
        /// <summary>낙하 효과가 다음 후보의 타입을 바꾸는 입력이다.</summary>
        private bool _dynamic;
        /// <summary>실제 판본의 추가 비트 위치다.</summary>
        private int Extra => Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35;

        /// <summary>독립 관측과 같은 client 슬롯50~64를 준비한다.</summary>
        public Scene(OriginalEdition edition)
        {
            Pool = new RawSidPool(24000, false, edition: edition);
            // 원본 관측이 사용하는 명시 슬롯까지 client 영역에서 할당한다.
            for (int sid = 5; sid <= 64; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Types[74].Flags2 = 4; Types[75].Flags2 = 0; Types[76].Flags2 = 0x10004; Types[77].Flags2 = 0x10000;
        }

        /// <summary>지정 슬롯의 WORD 또는 DWORD만 쓴다.</summary>
        public void Put(int sid, int offset, uint value, int width = 4)
        {
            if (width == 2) BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(sid)[offset..], unchecked((ushort)value));
            else BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid)[offset..], value);
        }

        /// <summary>행마다 같은 raw 입력과 탐색 계획으로 되돌린다.</summary>
        public void Prepare(byte extra, uint x, uint y, int scenario, bool post)
        {
            Events.Clear(); _cursor = 0; _dynamic = false;
            // 여섯 원본 입력 객체의 전체 슬롯을 새로 준비한다.
            foreach (int sid in ObjectIds)
            {
                Pool.RawSlot(sid).Clear();
                Pool.RawSlot(sid)[10] = sid is 60 or 63 ? (byte)75 : (byte)74;
                Pool.RawSlot(sid)[11] = sid == 61 ? (byte)2 : (byte)0;
                Pool.RawSlot(sid)[Extra] = sid == 50 ? extra : (byte)0;
                Put(sid, 14, x); Put(sid, 18, y);
            }
            Put(60, 12, 61, 2); Put(60, 8, 62, 2); Put(63, 12, 60, 2); Put(63, 8, 62, 2);
            int[][] plans = [[], [60, 63, 64], [63, 60, 64], [60, 60, 63], [64], [60, 63], [63, 60]];
            _selected = plans[scenario];
            if (scenario == 5) Pool.RawSlot(61)[Extra] = 9;
            if (scenario == 6) Pool.RawSlot(62)[11] = 2;
            if (post)
            {
                int[][] postPlans = [[], [60, 63, 64], [60, 63, 64], [60, 60, 64], [63]];
                _selected = postPlans[scenario];
                Pool.RawSlot(60)[10] = 77; Pool.RawSlot(63)[10] = 75; Pool.RawSlot(64)[10] = 76;
                _dynamic = scenario == 2;
            }
        }

        /// <summary>같은 명시 탐색과 dead/type 변경을 원문 사건 표현으로 기록한다.</summary>
        public BridgeLifecycleHooks Hooks() => new()
        {
            Begin = range => { Events.Add($"Q:{range.Left}:{range.Top}:{range.Right}:{range.Bottom}"); _cursor = 0; return _selected.Length == 0 ? 0 : _selected[0]; },
            Next = () => ++_cursor < _selected.Length ? _selected[_cursor] : 0,
            Emit = effect =>
            {
                switch (effect.Effect)
                {
                    case BridgeLifecycleEffect.DestroyLink: Events.Add($"D:{effect.Sid}:{effect.Flags}"); Pool.RawSlot(effect.Sid)[11] |= 2; break;
                    case BridgeLifecycleEffect.NotifyRemoval: Events.Add($"N:{effect.Sid}"); break;
                    case BridgeLifecycleEffect.FallSound: Events.Add($"S:{BitConverter.SingleToUInt32Bits(effect.X)}:{BitConverter.SingleToUInt32Bits(effect.Y)}"); break;
                    case BridgeLifecycleEffect.FallWalker:
                        Events.Add($"F:{effect.Sid}"); Pool.RawSlot(effect.Sid)[11] |= 2;
                        if (_dynamic && effect.Sid == 60) Pool.RawSlot(63)[10] = 76;
                        break;
                    case BridgeLifecycleEffect.BasePreDestroy: Events.Add($"A:{effect.Sid}:{effect.Flags}"); break;
                    case BridgeLifecycleEffect.BasePostDestroy: Events.Add($"B:{effect.Sid}:{effect.Flags}"); break;
                }
            },
        };

        /// <summary>기대 TYPE/state 외에는 풀 전체의 바이트가 바뀌지 않았는지 검사한다.</summary>
        public void CheckState(string expected, byte[] before)
        {
            // 원본 관측의 명시 변경 두 바이트만 이전 풀 사본에 반영한다.
            foreach (string tuple in expected.Split(';'))
            {
                int[] fields = tuple.Split(',').Select(int.Parse).ToArray();
                before[fields[0] * RawSidPool.Stride + 10] = (byte)fields[1];
                before[fields[0] * RawSidPool.Stride + 11] = (byte)fields[2];
            }
            Assert.True(Pool.Bytes.SequenceEqual(before), "원문 TYPE/state 외의 풀 바이트가 변경됨");
        }
    }

    /// <summary>원본 helper의882개 참조·상태·비대칭 추가 비트 조합을 대조한다.</summary>
    [Fact]
    public void X86LinkReferencesAndAsymmetricExtra()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        var body = new RawBridgeLifecycle(scene.Pool, scene.Types);
        int count = 0;
        // 저장된 Link 관측에 같은 참조 번호와 상태 입력을 넣는다.
        foreach (string[] row in X86Fixture.Read("bridgeeffects"))
        {
            if (row[0] != "Link") continue;
            scene.Prepare(0, 0, 0, 0, false);
            scene.Put(60, 12, X86Fixture.UInt(row[3]), 2); scene.Put(60, 8, X86Fixture.UInt(row[4]), 2);
            scene.Pool.RawSlot(61)[11] = byte.Parse(row[5]); scene.Pool.RawSlot(62)[11] = byte.Parse(row[6]);
            scene.Pool.RawSlot(61)[40] = byte.Parse(row[7]); scene.Pool.RawSlot(62)[40] = byte.Parse(row[8]);
            Assert.Equal(row[9] == "D:60:0", body.LinkNeedsDestroy(60)); count++;
        }
        Assert.Equal(882, count);
    }

    /// <summary>세 PE의 Pre2520·Post1800행과 PATCH Native840행을 같은 순서·풀 전체에 대조한다.</summary>
    [Theory]
    [InlineData("Pre", false, 2520)]
    [InlineData("Post", false, 1800)]
    [InlineData("Pre", true, 840)]
    public void X86LifecycleEffectsAndWholePool(string kind, bool native, int expected)
    {
        int count = 0;
        // 판본별 추가 비트 위치를 실제 원본 입력과 맞춰 각각 재생한다.
        foreach (string name in new[] { "originals", "originalCD", "original1037" })
        {
            if (native && name != "originals") continue;
            var scene = new Scene(name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            var body = new RawBridgeLifecycle(scene.Pool, scene.Types);
            // 해당 판본과 효과 종류에 속한 모든 최초 관측을 읽는다.
            foreach (string[] row in X86Fixture.Read("bridgeeffects"))
            {
                if (row[0] != kind || row[1] != name) continue;
                scene.Prepare(byte.Parse(row[3]), X86Fixture.UInt(row[5]), X86Fixture.UInt(row[6]), int.Parse(row[7]), kind == "Post");
                byte[] before = scene.Pool.Bytes.ToArray();
                if (kind == "Pre")
                {
                    if (native) body.PreDestroyNative(50, X86Fixture.UInt(row[4]), new() { Value = 75 }, scene.Hooks());
                    else body.PreDestroy(50, X86Fixture.UInt(row[4]), 75, scene.Hooks());
                }
                else body.PostDestroy(50, X86Fixture.UInt(row[4]), scene.Hooks());
                Assert.Equal(row[8], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events));
                scene.CheckState(row[9], before); count++;
            }
        }
        Assert.Equal(expected, count);
    }

    /// <summary>Delay726·Pack242개 원본 _ftol 및 signed DWORD→float 변환 결과 비트를 대조한다.</summary>
    [Fact]
    public void X86DelayedFallPayloadIsNumericFloat()
    {
        int delay = 0, pack = 0;
        // 비유한 값·음수·정밀도 경계까지 원본 저장 비트를 그대로 비교한다.
        foreach (string[] row in X86Fixture.Read("bridgeeffects"))
        {
            if (row[0] is not ("Delay" or "Pack")) continue;
            float actual = RawBridgeLifecycle.DelayedFallPayload(BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[3])), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[4])));
            Assert.Equal(X86Fixture.UInt(row[5]), BitConverter.SingleToUInt32Bits(actual));
            if (row[0] == "Delay") delay++; else pack++;
        }
        Assert.Equal(726, delay); Assert.Equal(242, pack);
    }

    /// <summary>선행 삭제가 바꾼 현재 DWORD를 다음 후보에 반영하고 BYTE 별칭을 허용하지 않는다.</summary>
    [Theory]
    [InlineData(75u, true)]
    [InlineData(0x1004bu, false)]
    public void NativeConnectorUsesCurrentFullDword(uint initial, bool deletes)
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Prepare(0, BitConverter.SingleToUInt32Bits(50.5f), BitConverter.SingleToUInt32Bits(50.5f), 1, false);
        var current = new UIntBox { Value = initial };
        var hooks = scene.Hooks(); var emit = hooks.Emit!;
        hooks.Emit = effect => { emit(effect); if (effect.Effect == BridgeLifecycleEffect.DestroyLink && effect.Sid == 60) { current.Value = 76; scene.Pool.RawSlot(63)[10] = 76; } };
        new RawBridgeLifecycle(scene.Pool, scene.Types).PreDestroyNative(50, 0x1234, current, hooks);
        Assert.Equal(deletes, scene.Events.Contains("D:60:0")); Assert.Equal(deletes, scene.Events.Contains("D:63:0"));
        Assert.Equal("A:50:4660", scene.Events[^1]);
    }
}
