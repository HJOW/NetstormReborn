using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공통 후처리(<see cref="RawPostPop"/>)와 다리 접두(<see cref="RawPostPop.BridgePrefix"/>)를
/// 원본 x86 기대값에 대조한다.
/// 기대값은 postpop-x86.tsv(공통 postPop 직접 호출·Pop·Unpop 984회 중 10.78 몫 492회)와
/// bridgepostpop-x86.tsv(다리 접두 240행 중 10.78 몫 80행)다.
/// 각 호출 뒤 비용·알림·깊이·세 통계표·목록 개수와 목록 전체 바이트·대상 슬롯 50바이트·표시 변경 표를 비교한다.
/// 열 구성은 cpppj/tests/PostPopTests.cpp 와 같다.
/// CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 PostPop 조각, 2026-10-20).
/// </summary>
public sealed class X86RawPostPopTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>마나 표시 경로의 가상 표 2개. 이 조각의 fixture 에는 나오지 않으나 표시 대역의 분기에 쓴다.</summary>
    private const uint ManaVtable0 = 0x0050C3B0;

    /// <summary>마나 표시 경로의 둘째 가상 표.</summary>
    private const uint ManaVtable1 = 0x00509810;

    /// <summary>타입 레코드 188개를 생성자 주소·발자국 1×1·원본 합성 입력의 HP·zOrder 로 채워 만든다.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord
            {
                ConstructorAddress = OriginalConstructors.Address(i),
                FootX = 1,
                FootY = 1,
                MaxHitPoints = 91,
                ZOrder = 7,
            };
        }
        return types;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>uint 표 전체를 리틀 엔디언 바이트로 이어 붙인다(stale 포함).</summary>
    /// <param name="values">직렬화할 표.</param>
    private static byte[] Words(uint[] values)
    {
        var bytes = new byte[values.Length * 4];
        // 각 값의 낮은 바이트부터 원본 메모리 순서로 쓴다
        for (int i = 0; i < values.Length; i++)
        {
            bytes[i * 4] = (byte)values[i];
            bytes[i * 4 + 1] = (byte)(values[i] >> 8);
            bytes[i * 4 + 2] = (byte)(values[i] >> 16);
            bytes[i * 4 + 3] = (byte)(values[i] >> 24);
        }
        return bytes;
    }

    /// <summary>네 단계 공간 해시의 모든 머리를 큰 배열부터 16비트 리틀 엔디언으로 이어 붙인다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static byte[] HashBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 0 단계부터 3 단계까지
        foreach (int level in Enumerable.Range(0, RawSquidHash.Levels))
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>
    /// 기대값의 표시 변경 표를 읽는다. "-" 는 빈 표, 그 밖은 "왼쪽,위,오른쪽,아래,플래그"를 ";" 로 이은 목록이다.
    /// </summary>
    /// <param name="cell">fixture 칸.</param>
    private static int[][] Rects(string cell)
    {
        // 빈 표는 빈 목록이다
        if (cell == "-")
        {
            return [];
        }
        // 각 사각형의 다섯 수를 순서대로 읽는다
        return cell.Split(';').Select(part => part.Split(',').Select(int.Parse).ToArray()).ToArray();
    }

    /// <summary>
    /// 이 fixture 의 고정 SHP 모양(cpppj PostPopTests.cpp 의 shapes)으로 공통 표시 갱신을 흉내 내는 대역.
    /// 원본 SquidDisplay::Validate·Update 의 일반 경로(마나·소용돌이 특수 경로 제외)를
    /// 이미 검증된 <see cref="DisplayBounds"/> 로 재현하고, 변경 표 수집은 원본 DirtyRegions::Add 의
    /// 병합 규칙(합집합 면적의 1/8 허용치, 첫 병합 대상에서 반환)으로 합친다. 모양이 고정이라 W14 표시 이식과는 별개다.
    /// </summary>
    private sealed class FixtureDisplay(OriginalTypeRecord[] types) : IRawUnpopDisplay
    {
        /// <summary>첫 프레임의 표시 폭·높이·hotspot.</summary>
        private static readonly short[] FirstFrame = [11, 17, 12, 16];

        /// <summary>그림자 프레임의 표시 폭·높이·hotspot.</summary>
        private static readonly short[] ShadowFrame = [19, 23, -3, 5];

        /// <summary>표시 영역(원본 640×480 viewport).</summary>
        private static readonly DisplayRect Viewport = new(0, 0, 640, 480);

        /// <summary>타입별 플래그·HP 를 읽는 타입 표.</summary>
        private readonly OriginalTypeRecord[] _types = types;

        /// <summary>갱신한 변경 표(사각형·플래그 순서대로).</summary>
        public List<(DisplayRect Rect, uint Flags)> Entries { get; } = [];

        /// <summary>전체 갱신 여부. 이 fixture 에서는 항목이 적어 항상 거짓이다.</summary>
        public bool FullRedraw => false;

        /// <summary>변경 표에 사각형을 넣는다. 원본 DirtyRegions::Add 의 병합 규칙을 따른다.</summary>
        /// <param name="rect">자를 필요 없는 화면 사각형.</param>
        /// <param name="flags">출력 플래그.</param>
        private void Add(DisplayRect rect, uint flags)
        {
            // 화면 밖 사각형은 무시한다
            if (!(rect.Left < 640 && rect.Top < 480 && rect.Right > 0 && rect.Bottom > 0))
            {
                return;
            }
            long area = (long)(rect.Right - rect.Left) * (rect.Bottom - rect.Top);
            // 원본은 첫 번째 병합 대상에서 반환하며 뒤 항목과 다시 병합하지 않는다
            foreach (int i in Enumerable.Range(0, Entries.Count))
            {
                DisplayRect old = Entries[i].Rect;
                long oldArea = (long)(old.Right - old.Left) * (old.Bottom - old.Top);
                DisplayRect united = new(
                    Math.Min(old.Left, rect.Left), Math.Min(old.Top, rect.Top),
                    Math.Max(old.Right, rect.Right), Math.Max(old.Bottom, rect.Bottom));
                long unitedArea = (long)(united.Right - united.Left) * (united.Bottom - united.Top);
                // 합집합 면적의 1/8 허용치 안에 두 면적의 합이 들어가지 않으면 병합한다
                if (unitedArea - unitedArea / 8 < oldArea + area)
                {
                    Entries[i] = (united, Entries[i].Flags);
                    return;
                }
            }
            Entries.Add((rect, flags));
        }

        /// <summary>공간 쓰기 전에 미지원 가상 표·물리 프레임 범위를 확인한다.</summary>
        /// <param name="slot">대상 슬롯 바이트.</param>
        /// <param name="flags">호출 플래그.</param>
        public void Validate(ReadOnlySpan<byte> slot, uint flags)
        {
            uint vtable = Read(slot, 0, 4);
            // 파생 표시 갱신은 지원 표에 있을 때만 공통 경로로 간다
            if (!OriginalUnpopDisplay.Supports(vtable, flags))
            {
                throw new InvalidOperationException("파생 표시 갱신 미복원");
            }
            // 마나의 WORD0 경로는 타입·모양·프레임을 읽지 않는다
            if (vtable == ManaVtable0 || vtable == ManaVtable1)
            {
                return;
            }
            int type = slot[RawSidPool.TypeOffset];
            // 타입 번호가 표를 벗어나면 거부한다
            if (type >= _types.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(slot), "표시 타입 번호 범위 오류");
            }
            int frame = (int)Read(slot, 36, 4);
            int frameCount = 1;
            int doubled = (_types[type].Flags1 & 0x440000) != 0 ? 2 : 1;
            // 패치의 frameCheck 조건을 만족하지 않으면 갱신 없이 돌아간다
            if (frame < 0 || frame >= frameCount * doubled || _types[type].MaxHitPoints == -0x22222223 || _types[type].MaxHitPoints == -0x32323233)
            {
                return;
            }
            int last = frame + ((_types[type].Flags1 & DisplayBounds.ShadowFlag1) != 0 ? frameCount : 0);
            // 물리 프레임이 모양(본그림·그림자 2개)을 벗어나면 거부한다
            if (frame < 0 || last < 0 || last >= 2)
            {
                throw new ArgumentOutOfRangeException(nameof(slot), "표시 물리 프레임 범위 오류");
            }
        }

        /// <summary>이전 위치의 변경 표를 공통 표시 경로로 전달한다.</summary>
        /// <param name="slot">대상 슬롯 바이트.</param>
        /// <param name="flags">호출 플래그.</param>
        public void Update(ReadOnlySpan<byte> slot, uint flags)
        {
            Validate(slot, flags);
            uint vtable = Read(slot, 0, 4);
            // 마나 경로는 오른쪽·아래에 10픽셀을 더한 본그림만 갱신한다
            if (vtable == ManaVtable0 || vtable == ManaVtable1)
            {
                bool bolt = vtable == ManaVtable1;
                if ((!bolt && Read(slot, 12, 2) == 0) || FullRedraw)
                {
                    return;
                }
                DisplayRect mana = Bounds(FirstFrame, slot);
                mana = new DisplayRect(mana.Left, mana.Top, mana.Right + 10, mana.Bottom + 10);
                mana = DisplayBounds.Clip(mana, Viewport);
                // 양의 면적만 변경 표에 올린다
                if (mana.HasArea)
                {
                    Add(mana, 0);
                }
                return;
            }
            int type = slot[RawSidPool.TypeOffset];
            int frame = (int)Read(slot, 36, 4);
            int frameCount = 1;
            int doubled = (_types[type].Flags1 & 0x440000) != 0 ? 2 : 1;
            // frameCheck 을 통과하지 못하거나 전체 갱신이면 등록하지 않는다
            if (frame < 0 || frame >= frameCount * doubled || _types[type].MaxHitPoints == -0x22222223 || _types[type].MaxHitPoints == -0x32323233 || FullRedraw)
            {
                return;
            }
            uint marker = (slot[40] & 4) != 0 ? 0x10U : 0U;
            DisplayRect main = DisplayBounds.Clip(Bounds(FirstFrame, slot), Viewport);
            // 선택 표시는 잘라낸 뒤에 넓히며 다시 자르지 않는다
            if (marker != 0)
            {
                main = DisplayBounds.ExpandForSelection(main, DisplayEdition.Patch1078);
            }
            // 양의 면적만 변경 표에 올린다
            if (main.HasArea)
            {
                Add(main, marker);
            }
            // 그림자 타입은 두 번째 프레임을 확장 없이 갱신한다
            if ((_types[type].Flags1 & DisplayBounds.ShadowFlag1) != 0)
            {
                DisplayRect shadow = DisplayBounds.Clip(Bounds(ShadowFrame, slot), Viewport);
                if (shadow.HasArea)
                {
                    Add(shadow, marker);
                }
            }
        }

        /// <summary>프레임 모양과 슬롯 좌표에서 화면 경계를 계산한다.</summary>
        /// <param name="frame">표시 폭·높이·hotspot 네 short.</param>
        /// <param name="slot">대상 슬롯 바이트.</param>
        private static DisplayRect Bounds(short[] frame, ReadOnlySpan<byte> slot) => DisplayBounds.Bounds(
            frame[0], frame[1], frame[2], frame[3],
            BitConverter.UInt32BitsToSingle(Read(slot, 14, 4)), BitConverter.UInt32BitsToSingle(Read(slot, 18, 4)),
            0, 0, DisplayBounds.ZoomOne);
    }

    /// <summary>
    /// 결과 열의 비용·알림·깊이·전체 표·목록·raw 슬롯·표시 변경 표를 대조한다.
    /// </summary>
    /// <param name="row">fixture 행.</param>
    /// <param name="start">결과 첫 열.</param>
    /// <param name="state">후처리 상태.</param>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">대상 번호.</param>
    /// <param name="display">표시 대역.</param>
    private static void CheckResult(string[] row, int start, RawPostPopState state, RawSidPool pool, int sid, FixtureDisplay display)
    {
        Assert.Equal(X86Fixture.Int(row[start]), state.TotalCost);
        Assert.Equal(X86Fixture.UInt(row[start + 1]), state.ProductionDirty);
        Assert.Equal(X86Fixture.UInt(row[start + 2]), state.Depth);
        uint[][] tables = [state.LocalCounts, state.LocalSecondaryCounts, state.GlobalCounts];
        // 실제 세 Totalmade 배열 전체를 checksum 으로 대조한다
        for (int i = 0; i < tables.Length; i++)
        {
            Assert.Equal(X86Fixture.UInt(row[start + 3 + i]), X86Fixture.Adler32(Words(tables[i])));
        }
        uint[] counts = row[start + 6].Split(',').Select(uint.Parse).ToArray();
        Assert.Equal(11, counts.Length);
        Assert.Equal(counts[0], state.Providers.Count);
        Assert.Equal(counts[1], state.Factories.Count);
        // 중립 0번을 포함한 아홉 소유자 목록의 개수를 순서대로 대조한다
        for (int i = 0; i < state.OwnerFactories.Length; i++)
        {
            Assert.Equal(counts[i + 2], state.OwnerFactories[i].Count);
        }
        var lists = new List<byte>();
        lists.AddRange(Words(state.Providers.Entries));
        lists.AddRange(Words(state.Factories.Entries));
        // 모든 소유자 목록의 stale 메모리까지 포함한다
        foreach (RawPostPopList list in state.OwnerFactories)
        {
            lists.AddRange(Words(list.Entries));
        }
        Assert.Equal(X86Fixture.UInt(row[start + 7]), X86Fixture.Adler32(lists.ToArray()));
        Assert.Equal(row[start + 8], Convert.ToHexStringLower(pool.Slot(sid)));
        Assert.Equal(row[start + 9] != "0", display.FullRedraw);
        int[][] expected = Rects(row[start + 10]);
        Assert.Equal(expected.Length, display.Entries.Count);
        // 무시된 항목도 플래그·순서·좌표까지 확인한다
        for (int i = 0; i < expected.Length; i++)
        {
            Assert.Equal(expected[i][0], display.Entries[i].Rect.Left);
            Assert.Equal(expected[i][1], display.Entries[i].Rect.Top);
            Assert.Equal(expected[i][2], display.Entries[i].Rect.Right);
            Assert.Equal(expected[i][3], display.Entries[i].Rect.Bottom);
            Assert.Equal((uint)expected[i][4], display.Entries[i].Flags);
        }
    }

    /// <summary>
    /// 공통 후처리·Pop·Unpop fixture 의 10.78 몫을 재생한다. Begin/Type/State/Fill 은 합성 입력이며 호출 수에 넣지 않는다.
    /// 호출마다 비용·표·목록·슬롯·표시를 대조한다.
    /// </summary>
    [Fact]
    public void PostPopPopAndUnpopMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("postpop");
        RawSidPool? pool = null;
        RawSquidHash? hash = null;
        RawPostPop? post = null;
        RawPop? pop = null;
        RawUnpop? unpop = null;
        FixtureDisplay? display = null;
        RawPostPopState? state = null;
        RawSquidFactory? factory = null;
        OriginalTypeRecord[] types = [];
        byte[] spots = [];
        bool patch = false;
        int sid = 0;
        uint vtable = 0;
        int number = 74;
        int begins = 0;
        var calls = new Dictionary<string, int>();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                Assert.Equal(4, row.Length);
                patch = row[1] == PatchEdition;
                if (patch)
                {
                    pool = new RawSidPool(32768, false);
                    hash = new RawSquidHash();
                    spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
                    types = Types();
                    factory = new RawSquidFactory(pool, types);
                    sid = factory.Create(74, SidAllocation.Client);
                    Assert.Equal(X86Fixture.Int(row[3]), sid);
                    vtable = Read(pool.Slot(sid), 0, 4);
                    begins++;
                }
                return true;
            }
            if (!patch)
            {
                return true;
            }
            switch (row[0])
            {
                case "Type":
                {
                    Assert.Equal(5, row.Length);
                    number = X86Fixture.Int(row[1]);
                    OriginalTypeRecord type = types[number];
                    type.Flags1 = X86Fixture.UInt(row[2]);
                    type.Flags2 = X86Fixture.UInt(row[3]);
                    type.Cost = X86Fixture.FloatBits(row[4]);
                    return true;
                }
                case "State":
                {
                    Assert.Equal(12, row.Length);
                    state = new RawPostPopState();
                    uint pc = X86Fixture.UInt(row[1]);
                    uint fc = X86Fixture.UInt(row[2]);
                    uint oc = X86Fixture.UInt(row[3]);
                    uint initial = X86Fixture.UInt(row[4]);
                    state.Providers.Entries = new uint[pc];
                    state.Factories.Entries = new uint[fc];
                    state.Providers.Count = Math.Min(initial, pc);
                    state.Factories.Count = Math.Min(initial, fc);
                    // 일반 목록의 첫 항목은 현재 SID, 나머지는 합성 stale 값이다
                    foreach (RawPostPopList list in new[] { state.Providers, state.Factories })
                    {
                        // 목록 항목을 순서대로 돌며 첫 항목에는 현재 SID, 나머지에는 합성 stale 값을 넣는다
                        for (int i = 0; i < list.Entries.Length; i++)
                        {
                            list.Entries[i] = i == 0 ? (uint)sid : 0x60000000U + (uint)i;
                        }
                    }
                    // 소유자 0번도 원본 메모리 보존 검사에 포함한다
                    for (int owner = 0; owner < state.OwnerFactories.Length; owner++)
                    {
                        RawPostPopList list = state.OwnerFactories[owner];
                        list.Entries = new uint[oc];
                        list.Count = Math.Min(initial, oc);
                        // 목록 항목 전부에 소유자와 번호로 정한 합성 값을 넣는다
                        for (int i = 0; i < list.Entries.Length; i++)
                        {
                            list.Entries[i] = 0x70000000U + (uint)(owner * 100 + i);
                        }
                    }
                    state.TotalCost = X86Fixture.Int(row[5]);
                    state.ProductionDirty = X86Fixture.UInt(row[6]);
                    uint seed = X86Fixture.UInt(row[7]);
                    // 세 통계 배열 전체를 원본과 같은 서로 다른 값으로 채운다
                    for (int i = 0; i < RawPostPopState.TableSize; i++)
                    {
                        uint value = unchecked(seed + (uint)i * 2654435761U);
                        state.LocalCounts[i] = value;
                        state.LocalSecondaryCounts[i] = unchecked(value + 17);
                        state.GlobalCounts[i] = unchecked(value + 34);
                    }
                    state.LocalOwner = (byte)X86Fixture.UInt(row[8]);
                    state.GraphsEnabled = row[9] != "0";
                    state.Suppressed = row[10] != "0";
                    state.Depth = X86Fixture.UInt(row[11]);
                    post = new RawPostPop(pool!, types, state);
                    display = new FixtureDisplay(types);
                    pop = new RawPop(pool!, hash!, spots, display, post);
                    unpop = new RawUnpop(pool!, hash!, spots, display);
                    return true;
                }
                case "Fill":
                {
                    Assert.Equal(3, row.Length);
                    hash!.Reset();
                    Span<byte> bytes = pool!.AllocatedBytes(sid);
                    bytes.Clear();
                    Put(bytes, 0, vtable);
                    bytes[RawSidPool.TypeOffset] = (byte)number;
                    bytes[RawSidPool.StateOffset] = 4;
                    Put(bytes, 14, BitConverter.SingleToUInt32Bits(20f));
                    Put(bytes, 18, BitConverter.SingleToUInt32Bits(20f));
                    bytes[34] = (byte)X86Fixture.UInt(row[1]);
                    bytes[40] = (byte)X86Fixture.UInt(row[2]);
                    bytes[30] = 17;
                    return true;
                }
                case "PostPop":
                    post!.PostPop(sid, X86Fixture.UInt(row[1]));
                    calls[row[0]] = calls.GetValueOrDefault(row[0]) + 1;
                    CheckResult(row, 2, state!, pool!, sid, display!);
                    return true;
                case "Pop":
                {
                    float x = X86Fixture.FloatBits(row[2]);
                    float y = X86Fixture.FloatBits(row[3]);
                    Assert.Equal(RawPopResult.Registered, pop!.Pop(sid, types[number], 1, 1, x, y, X86Fixture.UInt(row[1])));
                    calls[row[0]] = calls.GetValueOrDefault(row[0]) + 1;
                    CheckResult(row, 4, state!, pool!, sid, display!);
                    return true;
                }
                case "Unpop":
                    unpop!.Unpop(sid, types[number], X86Fixture.UInt(row[1]));
                    calls[row[0]] = calls.GetValueOrDefault(row[0]) + 1;
                    CheckResult(row, 2, state!, pool!, sid, display!);
                    return true;
                default:
                    return false;
            }
        });
        Assert.Equal(2, begins);
        Assert.Equal(
            new Dictionary<string, int> { ["PostPop"] = 384, ["Pop"] = 72, ["Unpop"] = 36 }.OrderBy(pair => pair.Key).ToArray(),
            calls.OrderBy(pair => pair.Key).ToArray());
    }

    /// <summary>실제 다리 postPop 의 접두 관찰 중 10.78 몫 80행을 재생한다. 후속 연결·공통 함수는 관찰 경계로 둔다.</summary>
    [Fact]
    public void BridgePrefixFlagsAndExtraMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("bridgepostpop");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            Assert.Equal(5, row.Length);
            var pool = new RawSidPool(24000, false);
            // 원본 fixture 의 번호 50 을 클라이언트 영역에 확보한다
            for (int id = 5; id <= 50; id++)
            {
                Assert.Equal(id, pool.Allocate(SidAllocation.Client));
            }
            const int target = 50;
            uint flags = X86Fixture.UInt(row[1]);
            pool.AllocatedBytes(target)[40] = (byte)X86Fixture.UInt(row[2]);
            byte[] expected = pool.Slot(target).ToArray();
            expected[40] = (byte)X86Fixture.UInt(row[3]);
            string calls = string.Empty;
            RawPostPop.BridgePrefix(pool, target, flags, sid =>
            {
                calls += $"L:{sid}:0:0;";
                // 최초 등록 비트는 연결 함수를 부르기 전에 이미 기록되어야 한다
                Assert.Equal(expected, pool.Slot(sid).ToArray());
            });
            string common = row[4][row[4].IndexOf("B:", StringComparison.Ordinal)..];
            Assert.Equal(row[4][..^common.Length], calls);
            Assert.Equal(expected, pool.Slot(target).ToArray());
            count++;
            return true;
        });
        Assert.Equal(80, count);
    }
}
