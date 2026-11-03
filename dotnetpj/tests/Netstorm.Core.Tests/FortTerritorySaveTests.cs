using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Territory 저장 부모·지역 getter·seed를 독립 원본 입력과 대조한다.
/// cpppj <c>FortTerritorySaveTests</c>의 부모 전수 638행·지역 조회 전수 20162행·seed 전수 188행을 옮겼다.
/// 저장소 수명 부분(4번째 검사의 후반)은 Client 프로젝트에 둔다. (F2c-7, 2026-11-03)
/// </summary>
public sealed class FortTerritorySaveTests
{
    /// <summary>마지막 빈 필드까지 지정 구분자로 분리한다.</summary>
    /// <param name="source">자를 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static List<string> Split(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 원문의 각 경계를 순서대로 읽는다
        for (; ; )
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return result;
            }
            start = at + 1;
        }
    }

    /// <summary>명시 raw 필드의 little endian 값을 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 너비다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        uint value = 0;
        // 지정 폭만 조합한다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>지정 폭의 원시 필드만 변경한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비다.</param>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 주변 필드는 그대로 둔다
        for (int index = 0; index < width; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>실제 바이트 자료를 독립 관측과 같은 소문자 hex로 변환한다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 바이트당 두 자리를 기록한다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>관측 입력의 hex를 그대로 바이트로 복원한다.</summary>
    /// <param name="text">읽을 hex다.</param>
    private static byte[] Bytes(string text)
    {
        byte[] result = new byte[text.Length / 2];
        // 각 입력 두 자리만 해석한다
        for (int at = 0; at < text.Length; at += 2)
        {
            result[at / 2] = byte.Parse(text.Substring(at, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }
        return result;
    }

    /// <summary>부호 있는 십진수와 전체 DWORD 입력을 같은 원시 비트로 읽는다.</summary>
    /// <param name="text">읽을 십진수다.</param>
    private static uint Number(string text)
    {
        return unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    }

    /// <summary>DWORD 비트를 바꾸지 않고 원본 signed 비교에 쓴다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    private static int Signed(uint value)
    {
        return unchecked((int)value);
    }

    /// <summary>실제 진단 도달에서 중단하는 검사 경계다. 원본 예외 unwind를 뜻하지 않는다.</summary>
    internal sealed class TerritoryStop : Exception;

    /// <summary>부모 입력에 지정한 외부 장치와 callback 효과만 소유한다.</summary>
    private sealed class TerritoryDevices
    {
        /// <summary>고정 Template 슬롯이다.</summary>
        public int Slot;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>고정 Template 슬롯과 현재 manager의 두 current다.</summary>
        public uint[] Currents = new uint[2];

        /// <summary>special·fort·mine·noRegion의 현재 raw DWORD다.</summary>
        public uint[] Globals = new uint[4];

        /// <summary>두 실제 stream이다.</summary>
        public byte[][] Streams = [new byte[16], new byte[16]];

        /// <summary>두 실제 stream과 data 변경을 구별하는 물리 자료다.</summary>
        public byte[][] Buffers = [new byte[16384], new byte[16384]];

        /// <summary>getter 입력 120바이트다.</summary>
        public byte[] Input = new byte[120];

        /// <summary>부모 로컬 기록의 마지막 관측 사본이다.</summary>
        public byte[] Saved = new byte[120];

        /// <summary>부모 로컬 배열을 빌린 보기다. 비어 있으면 관측 사본을 보관하지 않는다.</summary>
        public byte[]? Source;

        /// <summary>일반·Editor의 명시 지역 반환표다.</summary>
        public Dictionary<int, int[]> Regions = [];

        /// <summary>경계 호출 횟수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>입력 효과 목록이다.</summary>
        public List<string[]> Effects = [];

        /// <summary>실제 외부 trace다.</summary>
        public string Trace = "";

        /// <summary>
        /// 부모 로컬 배열이 살아 있을 때만 마지막 관측 사본을 보관한다.
        /// </summary>
        public void Capture()
        {
            if (Source is not null)
            {
                Array.Copy(Source, Saved, Saved.Length);
            }
        }

        /// <summary>
        /// 외부 경계를 기록하고 입력에서 지정한 현재 값만 바꾼다.
        /// </summary>
        /// <param name="kind">기록 종류다.</param>
        /// <param name="text">기록 본문이다.</param>
        public void Boundary(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            string current = kind + (Calls.TryGetValue(kind, out int count) ? count + 1 : 1);
            Calls[kind] = Calls.TryGetValue(kind, out int before) ? before + 1 : 1;
            // 같은 경계의 입력 변경을 작성 순서대로 적용한다
            foreach (string[] effect in Effects)
            {
                if (effect[0] != current)
                {
                    continue;
                }
                string target = effect[1];
                string field = effect[2];
                uint value = Number(effect[3]);
                if (target == "global")
                {
                    Globals[field == "special" ? 0 : field == "fort" ? 1 : field == "mine" ? 2 : 3] = value;
                }
                else if (target == "header")
                {
                    Assert.Equal("manager", field);
                    Manager = value;
                }
                else if (target.StartsWith("manager", StringComparison.Ordinal))
                {
                    Assert.Equal("current", field);
                    Currents[uint.Parse(target.Substring(7), CultureInfo.InvariantCulture) == 11 ? 0 : 1] = value;
                }
                else if (target.StartsWith("stream", StringComparison.Ordinal))
                {
                    Put(Streams[int.Parse(target.Substring(6), CultureInfo.InvariantCulture)], field == "data" ? 0 : field == "position" ? 4 : field == "end" ? 8 : 12, value);
                }
                else if (target == "source")
                {
                    Source![int.Parse(field.Substring(4), CultureInfo.InvariantCulture)] = (byte)value;
                }
                else
                {
                    Assert.Equal("control", target);
                    Assert.Equal("stop", field);
                    Assert.Equal(1u, value);
                    Capture();
                    throw new TerritoryStop();
                }
            }
            Capture();
        }

        /// <summary>
        /// 명시 getter·지도·seed·생성 장치만 연결하고 부모 판단·쓰기는 제품 본체가 수행한다.
        /// </summary>
        public FortTerritorySaveHooks Hooks()
        {
            return new FortTerritorySaveHooks
            {
                Load = (selected, output) =>
                {
                    Source = output;
                    Array.Copy(Input, output, Input.Length);
                    Boundary("get", "get:" + selected);
                },
                Special = () => Globals[0],
                NoRegion = () => Signed(Globals[3]),
                Island = (owner, region) =>
                {
                    int answer = Regions.TryGetValue(region, out int[]? pair) ? pair[0] : Signed(Globals[3]);
                    Boundary("region", "region:" + Globals[1] + ':' + owner + ':' + region);
                    return answer;
                },
                AnyIsland = region =>
                {
                    int answer = Regions.TryGetValue(region, out int[]? pair) ? pair[1] : Signed(Globals[3]);
                    Boundary("any", "any:" + region);
                    return answer;
                },
                Record = island =>
                {
                    byte[] result = new byte[6];
                    // 독립 PE에 공급한 외부 지도 반환값이다
                    for (int index = 0; index < 6; index++)
                    {
                        result[index] = (byte)(island * 17 + index * 29 + 7);
                    }
                    Boundary("record", "record:" + island);
                    return result;
                },
                Seed = (record, index) =>
                {
                    // 독립 부모 관측의 명시 seed 효과다. 하위 seed는 별도로 검증한다
                    record[0] = (byte)((record[0] & 0xc0u) | (index & 0x3fu));
                    record[1] = (byte)((record[1] & 0xf7u) | 3u);
                    record[3] = (byte)((index * 37u + 11u) % 254u + 1u);
                    Boundary("seed", "seed:" + index);
                },
                Manager = selected =>
                {
                    Assert.Equal(Slot, selected);
                    return Manager;
                },
                Create = selected => Boundary("create", "create:" + selected + ":Territory"),
                Current = selected =>
                {
                    Assert.True(selected == 11 || selected == 22);
                    return Currents[selected == 11 ? 0 : 1];
                },
                Assertion = kind => Boundary("assert", "assert:" + (kind == FortTerritorySaveAssertion.MissingManager ? 521 : 36)),
                Access = (selected, operation) =>
                {
                    MemStreamHooks memory = new()
                    {
                        Buffer = pointer => Buffers[(int)pointer],
                    };
                    operation(Streams[(int)selected], memory);
                    Capture();
                },
            };
        }

        /// <summary>
        /// 부모 TSV의 앞 15열만 장치 입력으로 배치한다.
        /// </summary>
        /// <param name="row">입력 행이다.</param>
        public void Initialize(string[] row)
        {
            Slot = int.Parse(row[1], CultureInfo.InvariantCulture);
            Manager = Number(row[2]);
            Currents[0] = Number(row[3]);
            Currents[1] = Number(row[4]);
            // 현재 네 전역을 원시 입력대로 놓는다
            for (int index = 0; index < 4; index++)
            {
                Globals[index] = Number(row[5 + index]);
            }
            // 두 stream은 동일 cursor와 서로 다른 초기 data를 갖는다
            for (int index = 0; index < 2; index++)
            {
                Put(Streams[index], 0, (uint)index);
                Put(Streams[index], 4, Number(row[9]));
                Put(Streams[index], 8, Number(row[10]));
                Put(Streams[index], 12, Number(row[11]));
                Array.Fill(Buffers[index], (byte)0xcc);
            }
            // 명시 후보표만 파싱한다
            if (row[12] != "-")
            {
                foreach (string record in Split(row[12], ';'))
                {
                    List<string> fields = Split(record, ':');
                    Regions[int.Parse(fields[0], CultureInfo.InvariantCulture)] = [int.Parse(fields[1], CultureInfo.InvariantCulture), int.Parse(fields[2], CultureInfo.InvariantCulture)];
                }
            }
            byte[] bytes = Bytes(row[13]);
            Assert.Equal(120, bytes.Length);
            Array.Copy(bytes, Input, Input.Length);
            // 기존 입력·순서 효과를 그대로 보관한다
            if (row[14] != "-")
            {
                foreach (string record in Split(row[14], ';'))
                {
                    Effects.Add([.. Split(record, '/')]);
                }
            }
        }

        /// <summary>
        /// 현재 두 stream의 data·pos·end·capacity를 입력과 같은 순서로 출력한다.
        /// </summary>
        public string StreamOutput()
        {
            string result = "";
            // callback 뒤 현재 값을 전부 대조한다
            foreach (byte[] stream in Streams)
            {
                if (result.Length != 0)
                {
                    result += ';';
                }
                result += $"{Read(stream, 0)}:{Read(stream, 4)}:{Read(stream, 8)}:{Read(stream, 12)}";
            }
            return result;
        }
    }

    /// <summary>
    /// 일반·Editor·현재 manager·current와 이미 고정한 stream의 전체 출력·부분 실패를 원문과 대조한다.
    /// </summary>
    [Fact]
    public void IndependentNativeParent()
    {
        string[][] rows = X86Fixture.Read("fort-territory-save");
        int compared = 0;
        // 앞 15열은 입력이고 뒤 11열은 실제 PE 관측이다
        foreach (string[] row in rows)
        {
            Assert.Equal(26, row.Length);
            TerritoryDevices devices = new();
            devices.Initialize(row);
            bool completed = true;
            try
            {
                RawFortTerritorySave.SnapshotFortTerritory(devices.Slot, devices.Hooks());
            }
            catch (TerritoryStop)
            {
                completed = false;
            }
            Assert.Equal(completed, row[15] == "1");
            Assert.Equal(devices.Manager, Number(row[16]));
            Assert.Equal(devices.Currents[0] + ":" + devices.Currents[1], row[17]);
            Assert.Equal(devices.Globals[0] + ":" + devices.Globals[1] + ":" + devices.Globals[2] + ":" + devices.Globals[3], row[18]);
            Assert.Equal(devices.StreamOutput(), row[19]);
            uint end = Math.Max(Read(devices.Streams[0], 8), Read(devices.Streams[1], 8));
            Assert.Equal(Hex(devices.Buffers[0].AsSpan(0, (int)end)), row[20]);
            Assert.Equal(Hex(devices.Buffers[1].AsSpan(0, (int)end)), row[21]);
            Assert.Equal(Hex(devices.Saved), row[22]);
            Assert.Equal(devices.Trace, row[23]);
            if (completed)
            {
                Assert.Equal(0, int.Parse(row[24], CultureInfo.InvariantCulture));
            }
            else
            {
                Assert.True(int.Parse(row[24], CultureInfo.InvariantCulture) < 0);
            }
            Assert.True(ulong.Parse(row[25], CultureInfo.InvariantCulture) > 0);
            compared++;
        }
        Assert.Equal(638, compared);
    }

    /// <summary>
    /// 실제 getter 본체가 수행한 signed 바이트·owner·mode·첫 존재 일치 결과와 비교한다.
    /// </summary>
    [Fact]
    public void IndependentNativeQueries()
    {
        string[][] rows = X86Fixture.Read("fort-territory-query");
        int compared = 0;
        // 앞 7열은 원문 32바이트 섬 입력이고 뒤 4열은 실행 결과다
        foreach (string[] row in rows)
        {
            Assert.Equal(11, row.Length);
            byte[] records = new byte[3204];
            Put(records, 0, Number(row[5]));
            int index = 0;
            // signed 입력을 원시 비트로 바꾸고 필드 사이의 padding·Temple은 0으로 둔다
            if (row[6] != "-")
            {
                foreach (string record in Split(row[6], ';'))
                {
                    List<string> fields = Split(record, ':');
                    int at = 4 + index++ * 32;
                    Put(records, at + 4, Number(fields[0]));
                    Put(records, at + 8, Number(fields[1]));
                    Put(records, at + 13, Number(fields[2]), 1);
                    Put(records, at + 12, Number(fields[3]), 1);
                }
            }
            int answer = row[1] == "normal"
                ? RawFortTerritorySave.FindFortTerritoryIsland(records, row[2] != "0", int.Parse(row[3], CultureInfo.InvariantCulture), int.Parse(row[4], CultureInfo.InvariantCulture))
                : RawFortTerritorySave.FindAnyFortTerritoryIsland(records, int.Parse(row[4], CultureInfo.InvariantCulture));
            Assert.Equal(Number(row[7]), unchecked((uint)answer));
            Assert.Equal("", row[8]);
            Assert.Equal("0", row[9]);
            Assert.True(ulong.Parse(row[10], CultureInfo.InvariantCulture) > 0);
            compared++;
        }
        Assert.Equal(20162, compared);
    }

    /// <summary>
    /// 실제 seed의 난수 호출 전·후 보존 필드와 진단 중단 자료를 대조한다.
    /// </summary>
    [Fact]
    public void IndependentNativeSeed()
    {
        string[][] rows = X86Fixture.Read("fort-territory-seed");
        int compared = 0;
        // 앞 5열은 독립 장치 입력, 뒤 5열은 실제 PE 관측이다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            byte[] bytes = Bytes(row[2]);
            Assert.Equal(6, bytes.Length);
            bool completed = true;
            string trace = "";
            Func<uint, uint> random = current =>
            {
                trace = "random:" + current;
                // 명시 난수 반환과 raw 효과만 공급한다
                if (row[4] != "-")
                {
                    foreach (string effect in Split(row[4], ';'))
                    {
                        List<string> fields = Split(effect, '/');
                        Assert.Equal("random1", fields[0]);
                        if (fields[1] == "control")
                        {
                            throw new TerritoryStop();
                        }
                        bytes[int.Parse(fields[2].Substring(4), CultureInfo.InvariantCulture)] = (byte)Number(fields[3]);
                    }
                }
                return Number(row[3]);
            };
            try
            {
                RawFortTerritorySave.SeedFortTerritoryRecord(bytes, Number(row[1]), random);
            }
            catch (TerritoryStop)
            {
                completed = false;
            }
            Assert.Equal(completed, row[5] == "1");
            Assert.Equal(Hex(bytes), row[6]);
            Assert.Equal(trace, row[7]);
            if (completed)
            {
                Assert.Equal(0, int.Parse(row[8], CultureInfo.InvariantCulture));
            }
            else
            {
                Assert.True(int.Parse(row[8], CultureInfo.InvariantCulture) < 0);
            }
            Assert.True(ulong.Parse(row[9], CultureInfo.InvariantCulture) > 0);
            compared++;
        }
        Assert.Equal(188, compared);
    }

    /// <summary>
    /// 같은 ChunkMap의 y·x 첫 일치와 signed 섬 바이트, seed의 장치 누락 때 부분 효과를 확인한다.
    /// 저장소 수명 부분은 Client 저장소 검사에서 잇는다.
    /// </summary>
    [Fact]
    public void MapRecordAndSeedBoundaries()
    {
        ChunkMap map = new(3, 3);
        map.At(1, 0).Island = 2;
        map.At(1, 0).Record = [1, 2, 3, 4, 5, 6];
        map.At(0, 1).Island = 2;
        map.At(0, 1).Record = [7, 8, 9, 10, 11, 12];
        Assert.Equal(map.At(1, 0).Record, RawFortTerritorySave.FortTerritoryIslandRecord(map, 2));
        map.At(1, 0).Record[2] = 77;
        Assert.Equal((byte)77, RawFortTerritorySave.FortTerritoryIslandRecord(map, 2)[2]);
        Assert.Equal(new byte[6], RawFortTerritorySave.FortTerritoryIslandRecord(map, 99));
        map.At(0, 0).Island = 255;
        map.At(0, 0).Record = [9, 8, 7, 6, 5, 4];
        Assert.Equal(map.At(0, 0).Record, RawFortTerritorySave.FortTerritoryIslandRecord(map, -1));
        byte[] partial = [0xff, 0xff, 1, 2, 3, 4];
        Assert.ThrowsAny<Exception>(() => RawFortTerritorySave.SeedFortTerritoryRecord(partial, 5, _ => throw new InvalidOperationException("미연결")));
        // 장치 누락 때에도 이미 수행한 첫 두 바이트 변경을 보존한다
        Assert.Equal([0xc5, 0xf7, 1, 2, 3, 4], partial);
    }
}
