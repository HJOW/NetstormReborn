// 원본 전체 배치 행동의 관문/현재 인자·로그/난수·부분 효과를 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 행동을 검사하는 클래스다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.
// 근거: cpppj tests/AiDropActionsTests.cpp의 15건 전부다.
public sealed class RawAiDropActionsTests
{
    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>실제 배치 경계에서 받은 원래 여섯 인자다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    /// <param name="Type">시설 TYPE이다.</param>
    /// <param name="Point">후보 XY다.</param>
    /// <param name="Argument">argument 0이다.</param>
    /// <param name="Direction">현재 방향이다.</param>
    private sealed record DropActionRequest(uint Type, RawPathPoint Point, uint Argument, uint Direction);

    /// <summary>같은 실제 본체/풀과 기존 실제 후보 압축기를 명시 원문 효과에 연결한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    private sealed class DropActionScene
    {
        /// <summary>실제 본체 저장소다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x55000000u, bytes => bytes.Fill(0));

        /// <summary>현재 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>같은 실제 풀/현재 TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>현재 frame 표다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>실제 후보 압축기의 같은 입력이다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>실제 후보 압축기의 같은 입력이다.</summary>
        public readonly ushort[] Surfaces = new ushort[65536];

        /// <summary>실제 후보 압축기의 같은 입력이다.</summary>
        public readonly uint[] Allies = new uint[81];

        /// <summary>현재 CRT 스캔 경계다.</summary>
        public AiBuildSearchBounds Bounds = new(10, 10, 246, 246);

        /// <summary>현재 다리 TYPE다.</summary>
        public uint Bridge = 82;

        /// <summary>원본 공용 난수다.</summary>
        public readonly GameRandom Random = new(1u);

        /// <summary>현재 시각이다.</summary>
        public double Now = 100;

        /// <summary>현재 지도 상한이다.</summary>
        public int Limit = 256;

        /// <summary>현재 진단 값이다.</summary>
        public bool CheckingValue;

        /// <summary>명시 생산 허용 여부다.</summary>
        public bool ProductOk = true;

        /// <summary>명시 스캔 미완료 여부다.</summary>
        public bool Pending;

        /// <summary>명시 구매 허용 여부다.</summary>
        public bool Afford = true;

        /// <summary>명시 선택 TYPE다.</summary>
        public uint SelectedType = 128;

        /// <summary>선택된 실제 하위다.</summary>
        public AiDropActionHooks Hooks = new();

        /// <summary>같은 실제 후보 압축기다.</summary>
        public readonly RawAiDropScan Scan;

        /// <summary>원문 호출 기록이다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>원문 생산 조회 인자다.</summary>
        public readonly List<uint> Products = [];

        /// <summary>원문 스캔 인자다.</summary>
        public readonly List<uint> ScanTypes = [];

        /// <summary>원문 스캔 인자다.</summary>
        public readonly List<int> ScanDirections = [];

        /// <summary>원문 배치 인자다.</summary>
        public readonly List<CanonPlacementQuery> Queries = [];

        /// <summary>원문 배치 허용 목록이다.</summary>
        public List<bool> Legal = [];

        /// <summary>원문 배치 인자다.</summary>
        public readonly List<DropActionRequest> Deployments = [];

        /// <summary>원문 알림 인자다.</summary>
        public readonly List<AiDropActionNotice> Notices = [];

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action<ulong>? DuringProduct;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action<ulong>? DuringPlace;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action<ulong>? DuringRemove;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action? DuringSelect;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action? DuringScan;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action? DuringAfford;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action? DuringDeploy;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action? DuringAssertion;

        /// <summary>각 실제 효과 뒤의 명시 현재 상태 변경이다.</summary>
        public Action<AiDropActionLog>? DuringLog;

        /// <summary>기본은 TYPE 128/스캔 완료/지갑 2000/빈 후보이며 이전 성공·표적·집중값은 남겨 둔다.</summary>
        public DropActionScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            // 생산 조각과 두 표적 슬롯을 준비하는 반복문이다.
            for (ushort sid = 5; sid <= 7; sid++)
            {
                Span<byte> raw = Pool.RawSlot(sid);
                raw.Fill(0);
                raw[RawSidPool.StateOffset] &= unchecked((byte)~RawSidPool.FreeBit);
                raw[10] = sid == 5 ? (byte)6 : (byte)(sid == 6 ? 150 : 151);
                if (sid != 5)
                {
                    Put(raw, 14, BitConverter.SingleToUInt32Bits(sid == 6 ? 71.8f : 93.9f));
                    Put(raw, 18, BitConverter.SingleToUInt32Bits(sid == 6 ? 82.7f : 104.1f));
                }
            }

            Field(0x260, 1);
            Field(0x3e0, 128);
            Field(0x3e4, 2);
            Field(0x3ec, 1);
            Field(0x2d8, 2000);
            Field(0x3f4, 77);
            Field(0x3fc, 88);
            Field(0x3f8, 99);
            Field(0xb00, 1);
            Scan = new RawAiDropScan(Memory, Pool, Types, Frames, Spots, Surfaces, Allies, Random, new AiDropScanGlobals
            {
                BridgeType = () => Bridge,
                Bounds = () => Bounds,
            }, new AiDropScanHooks
            {
                Assertion = (_, _) => throw new InvalidOperationException("예상하지 않은 빈 후보 압축"),
            });
            Hooks.Product = (brain, type) =>
            {
                Assert.Equal(Token, brain);
                ulong call = (ulong)Products.Count;
                Products.Add(type);
                Trace.Add("product");
                Sid result = ProductOk ? new Sid(5) : new Sid(0);
                DuringProduct?.Invoke(call);
                return result;
            };
            Hooks.Select = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("select");
                Field(0x3e0, SelectedType);
                DuringSelect?.Invoke();
            };
            Hooks.Scan = (brain, type, direction) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("scan");
                ScanTypes.Add(type);
                ScanDirections.Add(direction);
                DuringScan?.Invoke();
                return Pending;
            };
            Hooks.CanAfford = (brain, type) =>
            {
                Assert.Equal(Token, brain);
                Assert.Equal(Field(0x3e0), type);
                Trace.Add("afford");
                DuringAfford?.Invoke();
                return Afford;
            };
            Hooks.MayPlace = query =>
            {
                int call = Queries.Count;
                Queries.Add(query);
                Trace.Add("place");
                bool result = Legal.Count == 0 || Legal[(int)Math.Min(call, Legal.Count - 1)];
                DuringPlace?.Invoke((ulong)call);
                return result;
            };
            Hooks.Deploy = (brain, type, point, argument, direction) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("deploy");
                Deployments.Add(new DropActionRequest(type, point, argument, direction));
                DuringDeploy?.Invoke();
            };
            Hooks.Fake = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("fake");
            };
            Hooks.Seed = brain =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("seed");
            };
            Hooks.Remove = (brain, index) =>
            {
                Assert.Equal(Token, brain);
                Trace.Add("remove");
                Scan.Remove(brain, index);
                DuringRemove?.Invoke((ulong)index);
            };
            Hooks.Assertion = (text, line) =>
            {
                Assert.Equal("drop.value == 0", text);
                Assert.Equal(0xf8bu, line);
                Trace.Add("assert");
                DuringAssertion?.Invoke();
            };
            Hooks.Log = (brain, notice) =>
            {
                Assert.Equal(Token, brain);
                string[] names = ["lost", "complete", "partial", "cannot", "bad", "lay", "target"];
                Trace.Add(names[(int)notice.Action]);
                Notices.Add(notice);
                DuringLog?.Invoke(notice.Action);
            };
        }

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>지정 본체 DWORD를 쓴다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>QWORD 기한을 float 축소 없이 저장한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 기한이다.</param>
        public void Time(int at, double value)
        {
            ulong bits = BitConverter.DoubleToUInt64Bits(value);
            Field(at, (uint)bits);
            Field(at + 4, (uint)(bits >> 32));
        }

        /// <summary>실제 선택 배열에 후보와 표적 DWORD를 기록한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
        /// <param name="index">쓸 후보 번호다.</param>
        /// <param name="x">쓸 X 좌표다.</param>
        /// <param name="y">쓸 Y 좌표다.</param>
        /// <param name="target">쓸 표적 번호다.</param>
        public void Choice(int index, float x, float y, uint target = 6)
        {
            Field(0x408 + index * 8, BitConverter.SingleToUInt32Bits(x));
            Field(0x40c + index * 8, BitConverter.SingleToUInt32Bits(y));
            Field(0x4f8 + index * 4, target);
        }

        /// <summary>같은 실제 본체/풀/공용 난수와 명시 원문 하위로 행동한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
        public void Run()
        {
            new RawAiDropActions(Memory, Pool, Types, Random, new AiDropActionGlobals
            {
                Now = () => Now,
                MapLimit = () => Limit,
                Checking = new RawCheckingFlag(CheckingValue),
            }, Hooks).Run(Token);
        }
    }

    /// <summary>초기 TYPE 0에서는 생산 조회를 하지 않으며 선택 뒤 TYPE 0이면 뒤 스캔/관문도 생략한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectionZeroStopsAtNativeBoundary()
    {
        var scene = new DropActionScene();
        scene.Field(0x3e0, 0);
        scene.SelectedType = 0;
        scene.Run();
        Assert.Equal(["select"], scene.Trace);
        Assert.Equal(77u, scene.Field(0x3f4));
        Assert.Equal(88u, scene.Field(0x3fc));
        Assert.Equal(0u, scene.Field(0x3f0));
        Assert.Equal(1u, scene.Random.State());
    }

    /// <summary>생산 능력 상실 알림 자체는 TYPE를 지우거나 재선택을 강제하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void LostAbilityDoesNotForceSelection()
    {
        var scene = new DropActionScene();
        scene.ProductOk = false;
        scene.Pending = true;
        scene.Field(0x3ec, 0);
        scene.Run();
        Assert.Equal(["product", "lost", "scan", "product"], scene.Trace);
        Assert.Equal(128u, scene.Field(0x3e0));
        Assert.Equal(0u, scene.Field(0x3f0));
        Assert.Equal(0u, scene.Field(0x3ec));
        Assert.Equal(77u, scene.Field(0x3f4));
    }

    /// <summary>스캔 입력은 선택 직후 snapshot이고 완료 로그는 스캔 뒤 현재 TYPE/방향과 이미 쓴 완료 flag다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void ScanSnapshotAndCompletionBeforeGates()
    {
        var scene = new DropActionScene();
        scene.Field(0x3e0, 0);
        scene.Field(0x3ec, 0);
        scene.DuringScan = () =>
        {
            scene.Field(0x3e0, 129);
            scene.Field(0x3e4, 4);
        };
        scene.DuringLog = action =>
        {
            if (action == AiDropActionLog.ScanComplete)
            {
                Assert.Equal(1u, scene.Field(0x3ec));
                scene.Time(0x5a8, 101);
            }
        };
        scene.Run();
        Assert.Equal(["select", "scan", "complete"], scene.Trace);
        Assert.Equal([128u], scene.ScanTypes);
        Assert.Equal([2], scene.ScanDirections);
        Assert.Equal(129u, scene.Notices[0].Type);
        Assert.Equal(4, scene.Notices[0].Direction);
    }

    /// <summary>미래/NaN의 두 기한과 NaN 현재 시각은 스캔 뒤 관문에서 ordered 미도달이다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void OrderedQwordDeadlines()
    {
        double nan = double.NaN;
        // 두 기한을 각각 검증하고 equality는 통과시키며 float로 차이를 지우지 않는 반복문이다.
        foreach (int at in new int[] { 0x5a8, 0x5e8 })
        {
            foreach (double due in new double[] { 100.00000001, nan })
            {
                var scene = new DropActionScene();
                scene.Time(at, due);
                scene.Run();
                Assert.Equal(["product"], scene.Trace);
            }
        }

        var now = new DropActionScene();
        now.Now = nan;
        now.Run();
        Assert.Equal(["product"], now.Trace);
        var equal = new DropActionScene();
        equal.Time(0x5a8, 100);
        equal.Time(0x5e8, 100);
        equal.Run();
        Assert.Equal(["product", "product", "seed"], equal.Trace);
    }

    /// <summary>지갑은 signed 1200 이상일 때만 두 번째 생산 조회/후보 행동을 진행한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void SignedWalletMinimum()
    {
        // 1199/음수 DWORD/정확한 1200을 구분하는 반복문이다.
        foreach (uint wallet in new uint[] { 1199u, 0xffffffffu, 0x80000000u, 1200u })
        {
            var scene = new DropActionScene();
            scene.Field(0x2d8, wallet);
            scene.Run();
            Assert.Equal(wallet == 1200 ? new List<string> { "product", "product", "seed" } : new List<string> { "product" }, scene.Trace);
        }
    }

    /// <summary>최초 시도 400은 완료를 기다리고 재사용 시도는 스캔 미완료에서도 원래 Partial 알림 뒤 진행한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void FirstAttemptAndPartialScan()
    {
        var first = new DropActionScene();
        first.Field(0x400, 1);
        first.Field(0x3ec, 0);
        first.Pending = true;
        first.Run();
        Assert.Equal(["product", "scan"], first.Trace);
        Assert.Equal(0u, first.Field(0x3f0));
        var reuse = new DropActionScene();
        reuse.Field(0x3ec, 0);
        reuse.Pending = true;
        reuse.Run();
        Assert.Equal(["product", "scan", "product", "partial", "seed"], reuse.Trace);
        Assert.Equal(1u, reuse.Field(0x3f0));
    }

    /// <summary>현재 생산 조각의 WORD 100만 거르고 free/dead나 다른 pending 비트를 추가 필터하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentPendingBitOnly()
    {
        var scene = new DropActionScene();
        scene.Pool.RawSlot(new Sid(5).Value)[21] = 1;
        scene.Run();
        Assert.Equal(["product", "product"], scene.Trace);
        var other = new DropActionScene();
        other.Pool.RawSlot(new Sid(5).Value)[21] = 2;
        other.Pool.RawSlot(new Sid(5).Value)[11] |= 2;
        other.Run();
        Assert.Equal(["product", "product", "seed"], other.Trace);
    }

    /// <summary>사제 탐색은 signed 진행 비교이고 유효한 목표 좌표만 배치를 막는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void PriestProgressAndStrictTarget()
    {
        var searching = new DropActionScene();
        searching.Field(0x5a0, 2);
        searching.Field(0x5a4, 1);
        searching.Run();
        Assert.Equal(["product", "product"], searching.Trace);
        var signedProgress = new DropActionScene();
        signedProgress.Field(0x5a0, 0xffffffffu);
        signedProgress.Run();
        Assert.Equal(1u, signedProgress.Field(0x3f0));
        // strict 유효 좌표/0/NaN의 목표 관문을 서로 구분하는 반복문이다.
        foreach (float x in new float[] { 10.0f, 0.0f, float.NaN })
        {
            var scene = new DropActionScene();
            scene.Field(0xad0, BitConverter.SingleToUInt32Bits(x));
            scene.Field(0xad4, BitConverter.SingleToUInt32Bits(10.0f));
            scene.Run();
            Assert.Equal(x == 10.0f ? 0u : 1u, scene.Field(0x3f0));
        }
    }

    /// <summary>빈 후보 진단에서 복귀하면 현재 TYPE flags를 다시 읽어 fake/seed를 선택하며 직전 성공은 보존한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void NoChoiceAssertionUsesCurrentType()
    {
        var scene = new DropActionScene();
        scene.CheckingValue = true;
        scene.Field(0x3e8, 9);
        scene.Types[136].Flags2 = 0x800;
        scene.DuringAssertion = () => scene.Field(0x3e0, 136);
        scene.Run();
        Assert.Equal(["product", "product", "assert", "fake"], scene.Trace);
        Assert.Equal(1u, scene.Field(0x3f0));
        Assert.Equal(77u, scene.Field(0x3f4));
        Assert.Equal(88u, scene.Field(0x3fc));
        Assert.Equal(1u, scene.Random.State());
    }

    /// <summary>비용 거부는 기존 성공/표적·집중/후보를 지우기 전이며 현재 TYPE 이름으로 알린다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CannotAffordPreservesPriorState()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 1);
        scene.Choice(0, 11, 12);
        scene.Afford = false;
        scene.DuringAfford = () => scene.Field(0x3e0, 129);
        scene.Run();
        Assert.Equal(["product", "product", "afford", "cannot"], scene.Trace);
        Assert.Equal(129u, scene.Notices[0].Type);
        Assert.Equal(77u, scene.Field(0x3f4));
        Assert.Equal(88u, scene.Field(0x3fc));
        Assert.Equal(99u, scene.Field(0x3f8));
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(1u, scene.Field(0x3f0));
        Assert.Equal(1u, scene.Random.State());
    }

    /// <summary>공용 난수를 현재 count로 소비하고 실패 후보도 기존 실제 압축기로 제거한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void RandomRejectionsUseActualCompression()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 3);
        scene.Choice(0, 11, 12);
        scene.Choice(1, 21, 22);
        scene.Choice(2, 31, 32);
        scene.Legal = [false, false, true];
        var expected = new GameRandom(1u);
        var remaining = new List<float> { 11, 21, 31 };
        var chosen = new List<float>();
        // 실제 원본 난수로 매 압축 전 index를 계산하며 기대 후보의 상대 순서를 유지하는 반복문이다.
        while (remaining.Count != 0)
        {
            int index = (int)expected.Next((uint)remaining.Count);
            chosen.Add(remaining[index]);
            remaining.RemoveAt(index);
        }

        scene.Run();
        Assert.Equal(3, scene.Queries.Count);
        Assert.Single(scene.Deployments);
        Assert.Equal(expected.State(), scene.Random.State());
        Assert.Equal(0u, scene.Field(0x570));
        Assert.Equal(1u, scene.Field(0x3f4));
        Assert.Equal(6u, scene.Field(0x3fc));
        Assert.Equal(99u, scene.Field(0x3f8));
        Assert.Equal(1u, scene.Field(0x3f0));
        for (int index = 0; index < 3; index++)
        {
            Assert.Equal(chosen[index], scene.Queries[index].X);
        }

        Assert.Equal(chosen[^1], scene.Deployments[0].Point.X);
        Assert.Equal(AiDropActionLog.Target, scene.Notices[^1].Action);
        Assert.Equal(150u, scene.Notices[^1].Type);
        Assert.Equal(71, scene.Notices[^1].X);
        Assert.Equal(82, scene.Notices[^1].Y);
    }

    /// <summary>모든 후보 거부는 이전 성공/표적만 지우고 집중 표적을 건드리지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void AllChoicesInvalidKeepFocus()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 2);
        scene.Choice(0, 11, 12);
        scene.Choice(1, 21, 22);
        scene.Legal = [false, false];
        scene.Run();
        Assert.Equal(2, scene.Queries.Count);
        Assert.Empty(scene.Deployments);
        Assert.Equal(0u, scene.Field(0x570));
        Assert.Equal(0u, scene.Field(0x3f4));
        Assert.Equal(0u, scene.Field(0x3fc));
        Assert.Equal(99u, scene.Field(0x3f8));
        Assert.Equal(1u, scene.Field(0x3f0));
    }

    /// <summary>승인/로그/실제 배치 뒤의 현재 후보·TYPE/방향·표적과 하위 BYTE owner/focus를 각각 읽는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentQueriesLogsAndDeployArguments()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 1);
        scene.Field(0x260, 0x101);
        scene.Choice(0, 11.8f, 12.8f);
        scene.DuringPlace = _ =>
        {
            scene.Field(0x3e0, 129);
            scene.Field(0x3e4, 6);
            scene.Choice(0, 21.9f, 22.9f);
        };
        scene.DuringLog = action =>
        {
            if (action == AiDropActionLog.Laying)
            {
                scene.Field(0x4f8, 7);
            }

            if (action == AiDropActionLog.Target)
            {
                scene.Field(0x3e0, 130);
                scene.Field(0x3e4, 8);
                scene.Choice(0, 31.2f, 32.2f, 7);
            }
        };
        scene.DuringDeploy = () =>
        {
            scene.Field(0x4f8, 6);
            scene.Field(0x248, 0x40);
        };
        scene.Run();
        var query = scene.Queries[0];
        var deployed = scene.Deployments[0];
        Assert.Equal(128u, query.Type);
        Assert.Equal(1u, query.Owner);
        Assert.Equal(0u, query.Argument);
        Assert.Equal(0u, query.Flags);
        Assert.Equal(1u, query.Mode);
        Assert.Equal(11.8f, query.X);
        Assert.Equal(12.8f, query.Y);
        Assert.Equal(129u, scene.Notices[0].Type);
        Assert.Equal(21, scene.Notices[0].X);
        Assert.Equal(22, scene.Notices[0].Y);
        Assert.Equal(151u, scene.Notices[1].Type);
        Assert.Equal(93, scene.Notices[1].X);
        Assert.Equal(104, scene.Notices[1].Y);
        Assert.Equal(130u, deployed.Type);
        Assert.Equal(31.2f, deployed.Point.X);
        Assert.Equal(32.2f, deployed.Point.Y);
        Assert.Equal(0u, deployed.Argument);
        Assert.Equal(8u, deployed.Direction);
        Assert.Equal(6u, scene.Field(0x3fc));
        Assert.Equal(6u, scene.Field(0x3f8));
    }

    /// <summary>집중은 BYTE 40h에서만 선택하며 상위 BYTE 40h는 이전 집중 표적을 유지한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void FocusUsesLowByteOnly()
    {
        // 실제 현재 능력값 두 가지로 원문 BYTE 검사를 구분하는 반복문이다.
        foreach (uint capability in new uint[] { 0x40u, 0x4000u })
        {
            var scene = new DropActionScene();
            scene.Field(0x248, capability);
            scene.Field(0x570, 1);
            scene.Choice(0, 11, 12);
            scene.Run();
            Assert.Equal(capability == 0x40 ? 6u : 99u, scene.Field(0x3f8));
        }
    }

    /// <summary>중단은 승인 결과의 local이 아니라 실제 압축 효과 뒤 현재 3f4로만 판단한다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void RemoveEffectControlsStopping()
    {
        var invalid = new DropActionScene();
        invalid.Field(0x570, 1);
        invalid.Choice(0, 11, 12);
        invalid.Legal = [false];
        invalid.DuringRemove = _ => invalid.Field(0x3f4, 1);
        invalid.Run();
        Assert.Empty(invalid.Deployments);
        Assert.Equal(1u, invalid.Field(0x3f4));
        Assert.Equal(1u, invalid.Field(0x3f0));
        var twice = new DropActionScene();
        twice.Field(0x570, 2);
        twice.Choice(0, 11, 12);
        twice.Choice(1, 21, 22);
        twice.DuringRemove = _ =>
        {
            if (twice.Field(0x570) != 0)
            {
                twice.Field(0x3f4, 0);
            }
        };
        twice.Run();
        Assert.Equal(2, twice.Deployments.Count);
        Assert.Equal(0u, twice.Field(0x570));
        Assert.Equal(1u, twice.Field(0x3f4));
    }

    /// <summary>비용 비교가 count를 0으로 바꿔도 이미 들어온 후보 분기는 이전 성공/표적을 지운다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void AffordMutationClearsWithoutRandom()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 1);
        scene.Choice(0, 11, 12);
        scene.DuringAfford = () => scene.Field(0x570, 0);
        scene.Run();
        Assert.Equal(["product", "product", "afford"], scene.Trace);
        Assert.Equal(0u, scene.Field(0x3f4));
        Assert.Equal(0u, scene.Field(0x3fc));
        Assert.Equal(1u, scene.Field(0x3f0));
        Assert.Equal(1u, scene.Random.State());
    }

    /// <summary>실제 배치/압축 경계 누락은 이미 수행한 난수/로그/성공 쓰기를 취소하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingEffectsPreservePartialWrites()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 1);
        scene.Choice(0, 11, 12);
        scene.Hooks.Deploy = null;
        Assert.ThrowsAny<Exception>(() => scene.Run());
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(0u, scene.Field(0x3f4));
        Assert.Equal(0u, scene.Field(0x3fc));
        Assert.Equal(0u, scene.Field(0x3f0));
        Assert.NotEqual(1u, scene.Random.State());
        Assert.Equal(2, scene.Notices.Count);
        var removed = new DropActionScene();
        removed.Field(0x570, 1);
        removed.Choice(0, 11, 12);
        removed.Hooks.Remove = null;
        Assert.ThrowsAny<Exception>(() => removed.Run());
        Assert.Equal(1u, removed.Field(0x3f4));
        Assert.Equal(6u, removed.Field(0x3fc));
        Assert.Equal(1u, removed.Field(0x570));
        Assert.Equal(0u, removed.Field(0x3f0));
    }

    /// <summary>로그가 꺼져도 승인 뒤 현재 표적/TYPE 조회는 생략하지 않는다. 2026-11-25 R4-5b 배치 행동 이식에서 추가됐다.</summary>
    [Fact]
    public void DisabledLogStillReadsPhysicalTarget()
    {
        var scene = new DropActionScene();
        scene.Field(0x570, 1);
        scene.Choice(0, 11, 12, 65535);
        scene.Field(0xb00, 0);
        scene.Hooks.Log = null;
        Assert.ThrowsAny<Exception>(() => scene.Run());
        Assert.Empty(scene.Deployments);
        Assert.Equal(1u, scene.Field(0x570));
        Assert.Equal(0u, scene.Field(0x3f4));
    }
}
