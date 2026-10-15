using System.Globalization;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 10.78 Totalmade(허용 표·세 누적 장부·부류 합산)의 실제 기계어 관찰 52개를 <see cref="TotalMade"/>의
/// 반환값·assert 횟수·표 4개 전체 1,024바이트로 대조한다.
/// </summary>
/// <remarks>
/// 기대값은 cpppj/tests/fixtures/tutorial-one-x86.tsv 의 <c>Total</c> 행(원본 PE 독립 실행, 링크 공유)이며 C# 에서 계산하지 않는다.
/// 입력 표는 C++ 검사와 같은 식으로 만든다. CD 10.72 행 52개는 이 프로젝트의 대상이 아니라 개수만 확인한다. 2026-10-15 감사 D14 에서 추가.
/// </remarks>
public sealed class X86TotalMadeTests
{
    /// <summary>10.78(originals) 고유 입력 개수.</summary>
    private const int ExpectedPatchRows = 52;

    /// <summary>CD 10.72(originalCD) 고유 입력 개수. 재생하지 않고 fixture 가 줄지 않았는지만 본다.</summary>
    private const int ExpectedCdRows = 52;

    /// <summary>fixture 열 수: 종류·판본·연산·인자1·인자2·반환·assert 수·표 4개.</summary>
    private const int ColumnCount = 11;

    /// <summary>원본 실행기에 넘긴 독립 카운터 입력(기대 결과를 계산하지 않는다). 종류: 0 현재, 1 누적, 2 전체, 3 금지.</summary>
    private static uint InputCount(int index, int kind) => unchecked((uint)((index + 1) * (0x1020304 + kind * 17)));

    /// <summary>표 전체를 낮은 바이트부터 소문자 16진수로 만든다(1,024바이트).</summary>
    private static string TableHex(uint[] table)
    {
        var bytes = new byte[table.Length * 4];
        // 모든 DWORD 를 주변 바이트와 섞지 않고 기록한다.
        for (int i = 0; i < table.Length; i++)
            for (int b = 0; b < 4; b++) bytes[i * 4 + b] = (byte)(table[i] >> (8 * b));
        return Convert.ToHexStringLower(bytes);
    }

    /// <summary>입력 칸을 부호 있는 32비트로 읽는다.</summary>
    private static int Int(string text) => unchecked((int)long.Parse(text, CultureInfo.InvariantCulture));

    /// <summary>원본 10.78 52개 입력의 반환값·assert 수·세 장부와 허용 표 전체를 대조한다.</summary>
    [Fact]
    public void NativeTotalMade_MatchesReturnsAssertionsAndAllTables()
    {
        string[][] rows = X86Fixture.Read("tutorial-one").Where(row => row[0] == "Total").ToArray();
        Assert.Equal(ExpectedPatchRows, rows.Count(row => row[1] == "originals"));
        Assert.Equal(ExpectedCdRows, rows.Count(row => row[1] == "originalCD"));
        var operations = new Dictionary<int, int>();
        // 각 행은 독립 native 와 같은 새 표에서 시작한다.
        bool Matches(string[] row)
        {
            Assert.Equal(ColumnCount, row.Length);
            operations[Int(row[2])] = operations.GetValueOrDefault(Int(row[2])) + 1;
            // 원본 실행기와 같은 TYPE 부류 입력: 타입 i 는 플래그2 의 (i % 6) 번째 비트를 가진다.
            uint[] flags = new uint[TotalMade.NativeTypeCount];
            for (int i = 0; i < flags.Length; i++) flags[i] = 1U << (i % 6);
            int assertions = 0;
            var totals = new TotalMade(flags, _ => assertions++);
            // 같은 공유 장부와 허용 표를 독립 입력값으로 채운다.
            for (int i = 0; i < TotalMade.TableSize; i++)
            {
                totals.LocalCounts[i] = InputCount(i, 0);
                totals.LocalSecondaryCounts[i] = InputCount(i, 1);
                totals.GlobalCounts[i] = InputCount(i, 2);
                totals.Restrictions[i] = InputCount(i, 3);
            }
            int first = Int(row[3]), second = Int(row[4]);
            uint result = 0;
            switch (Int(row[2]))
            {
                case 0: totals.AllowAll(first); break;
                case 1: totals.Allow(first, second); break;
                case 2: result = totals.Allowed(first) ? 1U : 0U; break;
                case 3: totals.ResetCounts(); break;
                case 4: result = totals.Local(first); break;
                case 5: result = totals.Built(first); break;
                case 6: result = totals.Global(first); break;
                case 7: result = unchecked((uint)totals.BuiltForGenus(unchecked((uint)first))); break;
                default: throw new InvalidDataException("알 수 없는 Totalmade fixture 연산입니다.");
            }
            return result == uint.Parse(row[5], CultureInfo.InvariantCulture) && assertions == int.Parse(row[6], CultureInfo.InvariantCulture)
                && TableHex(totals.LocalCounts) == row[7] && TableHex(totals.LocalSecondaryCounts) == row[8]
                && TableHex(totals.GlobalCounts) == row[9] && TableHex(totals.Restrictions) == row[10];
        }
        X86Fixture.CheckRows(rows.Where(row => row[1] == "originals").ToArray(), Matches);
        // 여덟 연산 모두 재생됐는지(한 연산만 통과하거나 행이 조용히 줄지 않았는지) 확인한다.
        Assert.Equal(8, operations.Count);
    }

    /// <summary>
    /// 장부 기록: 로컬 소유자 객체는 현재 수와 누적 제작 수를, 모든 객체는 전체 현재 수를 올린다.
    /// 삭제는 현재 수만 내리고 누적 제작 수는 유지하며, 등록된 적 없는 객체를 내리면 DWORD 가 감긴다.
    /// </summary>
    [Fact]
    public void RecordPoppedAndDestroyed_FollowOriginalLedgerRules()
    {
        var totals = new TotalMade(new uint[TotalMade.NativeTypeCount], _ => { });
        totals.RecordPopped(82, ownerIsLocal: true);
        totals.RecordPopped(82, ownerIsLocal: true);
        totals.RecordPopped(82, ownerIsLocal: false);
        Assert.Equal((2u, 2u, 3u), (totals.Local(82), totals.Built(82), totals.Global(82)));
        totals.RecordDestroyed(82, ownerIsLocal: true);
        totals.RecordDestroyed(82, ownerIsLocal: false);
        Assert.Equal((1u, 2u, 1u), (totals.Local(82), totals.Built(82), totals.Global(82)));
        // 등록되지 않은 타입을 내리면 DWORD 가 감긴다(원본 --localCounts).
        totals.RecordDestroyed(90, ownerIsLocal: true);
        Assert.Equal(uint.MaxValue, totals.Local(90));
        Assert.Equal(0u, totals.Built(90));
        // 물리 표 밖 번호는 원본의 무검사 접근 대신 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => totals.RecordPopped(256, true));
        Assert.Throws<ArgumentOutOfRangeException>(() => totals.RecordDestroyed(-1, true));
    }

    /// <summary>부류 합산은 타입 70부터 한 비트라도 겹치는 타입의 누적 제작 수를 더하며, 내장 타입(0~69)과 비트 불일치는 제외한다.</summary>
    [Fact]
    public void BuiltForGenus_SumsOverlappingAssetTypesFromSeventy()
    {
        uint[] flags = new uint[TotalMade.NativeTypeCount];
        flags[69] = 4;
        flags[70] = 4;
        flags[71] = 4 | 8;
        flags[72] = 8;
        var totals = new TotalMade(flags, _ => { });
        totals.LocalSecondaryCounts[69] = 100;
        totals.LocalSecondaryCounts[70] = 3;
        totals.LocalSecondaryCounts[71] = 5;
        totals.LocalSecondaryCounts[72] = 7;
        Assert.Equal(8, totals.BuiltForGenus(4));
        Assert.Equal(12, totals.BuiltForGenus(8));
        Assert.Equal(15, totals.BuiltForGenus(4 | 8));
        // 감김: 합이 부호 있는 32비트를 넘으면 음수로 해석한다.
        totals.LocalSecondaryCounts[70] = int.MaxValue;
        totals.LocalSecondaryCounts[71] = 1;
        Assert.Equal(int.MinValue, totals.BuiltForGenus(4));
    }

    /// <summary>생성자는 타입 수가 다른 표를 거부하고, 진단 경계가 없을 때 원본 assert 에 닿으면 숨기지 않는다.</summary>
    [Fact]
    public void Construction_RejectsWrongTypeCountAndMissingAssertionBoundary()
    {
        Assert.Throws<ArgumentException>(() => new TotalMade(new uint[171]));
        var bare = new TotalMade(new uint[TotalMade.NativeTypeCount]);
        Assert.Equal(0u, bare.Built(1));
        Assert.Throws<InvalidOperationException>(() => bare.Built(0));
        Assert.Throws<InvalidOperationException>(() => bare.BuiltForGenus(0));
        Assert.False(new TotalMade(new uint[TotalMade.NativeTypeCount], _ => { }).Allowed(500));
    }
}
