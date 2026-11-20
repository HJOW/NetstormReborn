using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 Player 내용물 접두·현재 전역·Gump 통지 순서를 독립 관찰 자료로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원본88입력과 실제 SP 저장소의 늦은 초기화/현재 추적 뷰를 검사한다.</summary>
public sealed class RawPlayerContentNoticeTests
{
    /// <summary>현재 물리 슬롯에 지정 폭의 입력만 쓴다.</summary>
    private static void Put(Span<byte> raw, int at, uint value, bool word = false)
    {
        if (word) BinaryPrimitives.WriteUInt16LittleEndian(raw[at..], unchecked((ushort)value));
        else BinaryPrimitives.WriteUInt32LittleEndian(raw[at..], value);
    }
    /// <summary>원본과 같은 부모·세 종속의 전체 물리 입력을 공급한다.</summary>
    private static void Slots(RawSidPool pool, uint layout)
    {
        // 원문 입력은 free/dead와 무관하게 네 물리 슬롯을 직접 구성한다.
        foreach (int sid in new[] { 50, 60, 61, 62 }) pool.RawSlot(sid).Clear();
        Put(pool.RawSlot(50), 0, 0x50ee08); pool.RawSlot(50)[10] = 152;
        if (layout == 0) return;
        Put(pool.RawSlot(50), 6, 60, true);
        // 세 종속의 현재 type/state/등록ID와 next는 원본 실행기와 같은 명시 입력이다.
        foreach (int sid in new[] { 60, 61, 62 })
        {
            Span<byte> raw = pool.RawSlot(sid); Put(raw, 4, sid == 62 ? 0 : (uint)(sid + 1), true);
            raw[10] = (byte)(sid == 61 || (layout == 1 && sid == 60) ? 6 : layout == 5 ? 2 : 3);
            raw[11] = layout == 4 ? (byte)3 : (byte)0; Put(raw, 18, sid == 60 ? 7u : sid == 61 ? 8u : 9u);
        }
    }
    /// <summary>원본88행의 현재11전역·전체200바이트·가상 통지 인자/순서를 대조한다.</summary>
    [Fact]
    public void OriginalNotice88()
    {
        string[][] rows = X86Fixture.Read("player-content-notice"); Assert.Equal(88, rows.Length);
        // 각 독립 입력에 새 실제 풀·현재 전역·외부 등록표를 연결한다.
        foreach (string[] row in rows)
        {
            var pool = new RawSidPool(32768, true, null, OriginalEdition.Patch1078); uint layout = X86Fixture.UInt(row[12]), mutation = X86Fixture.UInt(row[13]);
            Slots(pool, layout); uint[] current = row.Take(11).Select(X86Fixture.UInt).ToArray(); short first = unchecked((short)(ushort)current[8]);
            var problem = new UIntBox { Value = current[2] }; var checks = new UIntBox { Value = current[4] }; var trace = new List<string>();
            var registry = new Dictionary<uint, uint> { [7] = layout == 3 ? 0u : 7u, [8] = layout == 3 ? 0u : 8u, [9] = layout == 3 ? 0u : 9u };
            var notice = new RawPlayerContentNotice(pool, new()
            {
                CursorA = () => current[0], CursorB = () => current[1], ProblemCode = problem, FallbackCode = () => current[3], MoneyChecks = checks,
                BattleShown = () => current[5], LocalPlayer = () => current[6], HasFirstSpPool = () => current[7] != 0,
                FirstPermutation = () => first, GumpFormType = () => current[9], ContentEvent = () => current[10],
            }, new()
            {
                Money = owner => { trace.Add("M:" + owner); if (mutation == 1) { current[3] = 91; checks.Value = 0; } return BitConverter.UInt32BitsToSingle(X86Fixture.UInt(row[11])); },
                Gump = id => registry.GetValueOrDefault(id),
                Handle = (token, ev, form, reason) =>
                {
                    trace.Add($"H:{token}:{ev}:{form}:{reason}");
                    if (mutation == 2) { Put(pool.RawSlot(60), 4, 0, true); current[10] = 0x11223344; registry[9] = 9; }
                    else if (mutation == 3) current[9] = 6;
                },
            });
            notice.Notify(50, X86Fixture.UInt(row[14]), X86Fixture.UInt(row[15])); current[2] = problem.Value; current[4] = checks.Value; current[8] = unchecked((ushort)first);
            Assert.Same(pool, notice.Pool); Assert.Equal(row[17], string.Join(',', current)); Assert.Equal(row[18], trace.Count == 0 ? "-" : string.Join('|', trace));
            Assert.Equal(row[19], string.Join(',', new[] { 50, 60, 61, 62 }.Select(sid => Convert.ToHexString(pool.Slot(sid)).ToLowerInvariant())));
        }
    }
    /// <summary>동일 SP의 늦은 초기화·조건부 현재 소유자 조회·정확한7000 초과·회전/추적 불일치를 확인한다.</summary>
    [Fact]
    public void NoticeReadsActualCurrentSpStorage()
    {
        var pool = new RawSidPool(32768, true, null, OriginalEdition.Patch1078); pool.RawSlot(50).Clear();
        var tracking = new IntBox(); var sp = new RawSpStore(new GameRandom(), () => 100); sp.BindTrackingOffset(tracking);
        var problem = new UIntBox { Value = 99 }; var checks = new UIntBox(); uint fallback = 7, shown = 0; int moneyCalls = 0;
        var notice = new RawPlayerContentNotice(pool, new()
        {
            CursorA = () => sp.OffsetCurrent, CursorB = () => unchecked((uint)tracking.Value), ProblemCode = problem, FallbackCode = () => fallback,
            MoneyChecks = checks, BattleShown = () => shown, LocalPlayer = () => 1, HasFirstSpPool = () => sp.FirstPoolCurrent is not null,
            FirstPermutation = () => sp.Permutation[0], GumpFormType = () => 3, ContentEvent = () => 42,
        }, new()
        {
            Money = owner => { moneyCalls++; return BitConverter.UInt32BitsToSingle(sp.Get(owner)); },
            Gump = _ => throw new InvalidOperationException("빈 부모의 등록표 조회"), Handle = (_, _, _, _) => throw new InvalidOperationException("빈 부모의 UI 처리"),
        });
        notice.Notify(50, 6, 1); Assert.Equal(7u, problem.Value); Assert.Null(sp.FirstPoolCurrent); Assert.Equal(0, moneyCalls);
        checks.Value = 1; shown = 1; problem.Value = 99; notice.Notify(50, 6, 1); Assert.Equal(99u, problem.Value); Assert.NotNull(sp.FirstPoolCurrent); Assert.Equal(1, moneyCalls); Assert.Equal(0u, checks.Value);
        sp.Set(1, BitConverter.SingleToUInt32Bits(7000.5f)); checks.Value = 1; notice.Notify(50, 6, 1); Assert.Equal(7u, problem.Value);
        problem.Value = 99; fallback = 8; sp.Rotate(); notice.Notify(50, 6, 2); Assert.Equal(8u, problem.Value);
        tracking.Value = 3; problem.Value = 99; notice.Notify(50, 6, 2); Assert.Equal(99u, problem.Value);
        checks.Value = 1; shown = 0; notice.Notify(50, 6, 2); Assert.Equal(2, moneyCalls); Assert.Equal(0u, checks.Value);
    }
}
