using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 전체 Territory·Chaff 조회(<see cref="RawFortRegionSave"/>)를 독립 원본 입력과 대조한다.
/// cpppj <c>FortRegionSaveTests</c>의 전수 930행과 경계 직접 2종을 옮겼다. (F2c-4, 2026-11-03)
/// </summary>
public sealed class FortRegionSaveTests
{
    /// <summary>독립 PE 입력과 같은 고정 stream 토큰이다.</summary>
    private const uint RegionStream = 0x12345678u;

    /// <summary>마지막 빈 칸도 보존해 독립 TSV 열과 명시 효과를 분리한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] RegionFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 각 구분자의 다음 위치에서 원문 그대로 복사한다
        for (; ; )
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>명시 중단 callback과 원본 부분 실행을 같은 실제 영역 읽기로 검증하며 실패와 구별한다.</summary>
    internal sealed class RegionStop : Exception;

    /// <summary>독립 입력을 실제 위치만 공급하고 조회·판정·청크 효과는 제품 몸체에 맡긴다.</summary>
    private sealed class RegionDevices
    {
        /// <summary>현재 지형 너비다.</summary>
        public int Width;

        /// <summary>현재 지형 높이다.</summary>
        public int Height;

        /// <summary>공유가 빌리는 같은 scope다.</summary>
        public uint Scope;

        /// <summary>고정 raw owner다.</summary>
        public uint Owner;

        /// <summary>적을 영역 값이다.</summary>
        public int Region;

        /// <summary>원본에 공급하는 같은 지역 바이트다.</summary>
        public List<byte> Regions = [];

        /// <summary>판정 반환 순서다.</summary>
        public List<uint> Decisions = [];

        /// <summary>호출 경계별 횟수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>입력 효과 목록이다.</summary>
        public List<string[]> Effects = [];

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>실제 위치를 캡처하고 같은 호출 순서를 trace로 남긴다.</summary>
        /// <param name="kind">기록 종류다.</param>
        /// <param name="text">기록 본문이다.</param>
        public void Boundary(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            int count = Calls.GetValueOrDefault(kind) + 1;
            Calls[kind] = count;
            string full = kind + count.ToString(CultureInfo.InvariantCulture);
            // 같은 호출 이름만 원문 입력 순서대로 적용한다
            foreach (string[] effect in Effects)
            {
                if (effect[0] != full)
                {
                    continue;
                }
                long value = long.Parse(effect[2], CultureInfo.InvariantCulture);
                if (effect[1] == "width")
                {
                    Width = unchecked((int)unchecked((uint)value));
                }
                else if (effect[1] == "height")
                {
                    Height = unchecked((int)unchecked((uint)value));
                }
                else if (effect[1] == "scope")
                {
                    Scope = unchecked((uint)value);
                }
                else
                {
                    Assert.Equal("stop", effect[1]);
                    Assert.Equal(1, value);
                    throw new RegionStop();
                }
            }
        }

        /// <summary>열 0만 입력으로 사용하며 나머지는 원본 실행 관측을 분리한다.</summary>
        /// <param name="row">입력 행이다.</param>
        public void Initialize(string[] row)
        {
            Width = int.Parse(row[2], CultureInfo.InvariantCulture);
            Height = int.Parse(row[3], CultureInfo.InvariantCulture);
            Scope = uint.Parse(row[4], CultureInfo.InvariantCulture);
            Owner = uint.Parse(row[5], CultureInfo.InvariantCulture);
            Region = int.Parse(row[6], CultureInfo.InvariantCulture);
            for (int index = 0; index < row[7].Length; index += 2)
            {
                Regions.Add(Convert.ToByte(row[7].Substring(index, 2), 16));
            }
            foreach (string value in RegionFields(row[8], ','))
            {
                Decisions.Add(uint.Parse(value, CultureInfo.InvariantCulture));
            }
            if (row[9] != "-")
            {
                foreach (string effect in RegionFields(row[9], ';'))
                {
                    Effects.Add(RegionFields(effect, '/'));
                }
            }
        }

        /// <summary>getter가 원본 MOV 위치에 캡처한 값을 돌려주고 변경 효과는 다음 getter가 관측한다.</summary>
        public FortRegionSaveHooks Hooks()
        {
            return new FortRegionSaveHooks
            {
                ChunksX = () =>
                {
                    int value = Width;
                    Boundary("x", "X:" + value + ':' + Scope);
                    return value;
                },
                ChunksY = () =>
                {
                    int value = Height;
                    Boundary("y", "Y:" + value + ':' + Scope);
                    return value;
                },
                ChunkRegion = (x, y) =>
                {
                    Assert.True(x >= 0 && x < 8 && y >= 0 && y < 8);
                    byte value = Regions[(y * 8 + x) % Regions.Count];
                    Boundary("cell", "cell:" + x + ':' + y + ':' + value + ':' + Scope);
                    return value;
                },
                Allowed = (region, selectedOwner, selectedRegion) =>
                {
                    uint answer = Decisions[(int)(Calls.GetValueOrDefault("allow") % Decisions.Count)];
                    Boundary("allow", "allow:" + region + ':' + selectedOwner + ':' + selectedRegion + ':' + answer + ':' + Scope);
                    return answer;
                },
                Chunk = (stream, x, y) => Boundary("chunk", "chunk:" + stream + ':' + x + ':' + y + ':' + Scope),
            };
        }
    }

    /// <summary>단일 지점을 명시 실패하는지 확인하며 원본 부분 중단을 구별한다.</summary>
    /// <param name="function">실행할 동작이다.</param>
    private static bool RegionRejects(Action function)
    {
        try
        {
            function();
        }
        catch (InvalidOperationException)
        {
            return true;
        }
        return false;
    }

    /// <summary>몸체 진입·signed 바이트·높이 위치·scope·선택·전체 효과·범위 실패를 원문 930입력으로 비교한다.</summary>
    [Fact]
    public void IndependentNativeTraversal()
    {
        string[][] rows = X86Fixture.Read("fort-region-save");
        int compared = 0;
        // 열 0만 입력으로 쓰고 실행 관측을 분리한다
        foreach (string[] row in rows)
        {
            Assert.Equal(18, row.Length);
            RegionDevices devices = new();
            devices.Initialize(row);
            bool completed = true;
            try
            {
                if (row[1] == "territory")
                {
                    RawFortRegionSave.SaveFortTerritoryChunks(RegionStream, devices.Owner, devices.Region, ref devices.Scope, devices.Hooks());
                }
                else
                {
                    Assert.Equal("chaff", row[1]);
                    RawFortRegionSave.SaveFortChaffChunks(RegionStream, ref devices.Scope, devices.Hooks());
                }
            }
            catch (RegionStop)
            {
                completed = false;
            }
            Assert.Equal(completed, row[10] == "1");
            Assert.Equal(int.Parse(row[11], CultureInfo.InvariantCulture), devices.Width);
            Assert.Equal(int.Parse(row[12], CultureInfo.InvariantCulture), devices.Height);
            Assert.Equal(uint.Parse(row[13], CultureInfo.InvariantCulture), devices.Scope);
            Assert.Equal(devices.Trace.Length == 0 ? "-" : devices.Trace, row[14]);
            if (completed)
            {
                Assert.Equal(0, int.Parse(row[15], CultureInfo.InvariantCulture));
            }
            else
            {
                Assert.True(int.Parse(row[15], CultureInfo.InvariantCulture) < 0);
            }
            Assert.True(uint.Parse(row[16], CultureInfo.InvariantCulture) > 0);
            Assert.True(uint.Parse(row[17], CultureInfo.InvariantCulture) <= 1);
            if (row[1] == "chaff")
            {
                Assert.Equal(0, devices.Calls.GetValueOrDefault("allow"));
            }
            compared++;
        }
        Assert.Equal(930, compared);
    }

    /// <summary>빈 지형도 scope만 쓰고 미등록 getter·지역 판정·쓰기 위치는 강제 요구하지 않는다.</summary>
    [Fact]
    public void EmptyAndMissingEffects()
    {
        uint scope = 77;
        FortRegionSaveHooks hooks = new();
        hooks.ChunksY = () =>
        {
            Assert.Equal(77u, scope);
            return 0;
        };
        RawFortRegionSave.SaveFortTerritoryChunks(RegionStream, 1, 2, ref scope, hooks);
        Assert.Equal(1u, scope);
        scope = 77;
        RawFortRegionSave.SaveFortChaffChunks(RegionStream, ref scope, hooks);
        Assert.Equal(2u, scope);
        scope = 77;
        hooks.ChunksY = null;
        Assert.True(RegionRejects(() => RawFortRegionSave.SaveFortChaffChunks(RegionStream, ref scope, hooks)));
        Assert.Equal(77u, scope);
        hooks.ChunksY = () => 1;
        Assert.True(RegionRejects(() => RawFortRegionSave.SaveFortTerritoryChunks(RegionStream, 1, 2, ref scope, hooks)));
        Assert.Equal(1u, scope);
        int rows = 0;
        hooks.ChunksX = () => 0;
        hooks.ChunksY = () =>
        {
            rows++;
            return 2;
        };
        scope = 77;
        RawFortRegionSave.SaveFortChaffChunks(RegionStream, ref scope, hooks);
        Assert.Equal(2u, scope);
        Assert.Equal(3, rows);
    }

    /// <summary>Chaff는 지역 반환·판정을 묻지 않으며 같은 지역 조회 효과는 다음 기회에 변경을 적용한다.</summary>
    [Fact]
    public void ChaffKeepsLookupEffects()
    {
        uint scope = 77;
        int width = 2;
        int queries = 0;
        int writes = 0;
        FortRegionSaveHooks hooks = new();
        hooks.ChunksY = () => 1;
        hooks.ChunksX = () => width;
        hooks.ChunkRegion = (x, y) =>
        {
            Assert.Equal(0, x);
            Assert.Equal(0, y);
            Assert.Equal(2u, scope);
            scope = 9;
            width = 0;
            queries++;
            return (byte)255;
        };
        hooks.Allowed = (region, owner, territory) => throw new InvalidOperationException("Chaff에서 지역 판정을 호출했다");
        hooks.Chunk = (stream, x, y) =>
        {
            Assert.Equal(RegionStream, stream);
            Assert.Equal(0, x);
            Assert.Equal(0, y);
            Assert.Equal(9u, scope);
            writes++;
        };
        RawFortRegionSave.SaveFortChaffChunks(RegionStream, ref scope, hooks);
        Assert.Equal(9u, scope);
        Assert.Equal(1, queries);
        Assert.Equal(1, writes);
    }
}
