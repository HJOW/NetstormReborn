using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Mission 저장(<see cref="RawFortMissionSave"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortMissionSaveTests</c>의 전수 230행과 경계 직접 1종을 옮겼다. (F2c-4, 2026-11-03)
/// </summary>
public sealed class FortMissionSaveTests
{
    /// <summary>최초 native 물리버퍼 주소이며 포인터 역참조로 비교하지 않는다.</summary>
    private static readonly uint[] MissionBuffers = [0x12040000u, 0x12060000u];

    /// <summary>65537바이트 이름과 출력 주변까지 비교하는 실제 물리 범위다.</summary>
    private const int MissionBufferBytes = 0x11000;

    /// <summary>마지막 빈 칸도 보존해 fixture 입력·관측 열을 구분한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] MissionFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 모든 구분자의 다음 위치를 차례대로 찾는다
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

    /// <summary>현재 stream의 지정 DWORD를 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint MissionGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 원본 네 바이트만 낮은 비트부터 조립한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>현재 stream의 지정 DWORD를 갱신한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void MissionPut(Span<byte> bytes, int at, uint value)
    {
        // 상위 바이트부터 낮은 바이트만 기록한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>주변 바이트를 포함해 관측을 고정 소문자 hex로 반환한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string MissionHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 물리 관측에서 바꾸거나 NUL에서 자르지 않는다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>모든 실제 출력 바이트를 원본 규격 독립 실행 C++ FNV-1a 계산으로 비교한다.</summary>
    /// <param name="bytes">해시할 바이트다.</param>
    private static string MissionHash(ReadOnlySpan<byte> bytes)
    {
        ulong value = 0xcbf29ce484222325ul;
        // unsigned64 곱셈은 감싼 뒤에 전체 물리 범위를 돈다
        foreach (byte b in bytes)
        {
            value = (value ^ b) * 0x100000001b3ul;
        }
        const string digits = "0123456789abcdef";
        StringBuilder result = new(16, 16);
        // 상위 nibble부터 고정 길이로 표시한다
        for (int index = 0; index < 16; index++)
        {
            result.Append(digits[(int)((value >> ((15 - index) * 4)) & 15)]);
        }
        return result.ToString();
    }

    /// <summary>원본 callback 중단과 명시 물리 접근 실패를 정상 반환과 구별한다.</summary>
    internal sealed class MissionStop : Exception
    {
        /// <summary>기대 중단 지점만 보관한다.</summary>
        public string Cause { get; }

        /// <summary>중단 원인을 보관한다.</summary>
        /// <param name="cause">중단 원인 이름이다.</param>
        public MissionStop(string cause)
        {
            Cause = cause;
        }
    }

    /// <summary>최초 fixture 입력만 사용하며 기대 관측은 제품 실행 뒤에 둔다.</summary>
    private sealed class MissionDevices
    {
        /// <summary>진입 몸체다.</summary>
        public string Mode = "";

        /// <summary>명시 효과 이름이다.</summary>
        public string Mutation = "";

        /// <summary>입력·일반 쓰기·진단 trace다.</summary>
        public string Trace = "";

        /// <summary>현재 Guide 코드와 독립 입력·조회값이다.</summary>
        public uint Current;

        /// <summary>이름 경우 번호다.</summary>
        public uint NameCase;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>쓴 횟수다.</summary>
        public uint Writes;

        /// <summary>현재 조회 횟수다.</summary>
        public uint CurrentReads;

        /// <summary>이름 조회 횟수다.</summary>
        public uint NameReads;

        /// <summary>토큰 1·2가 구별되는 실제 Mission 필드다.</summary>
        public uint[] Sequence = new uint[3];

        /// <summary>토큰 1·2가 구별되는 실제 Mission 플래그다.</summary>
        public uint[] Flags = new uint[3];

        /// <summary>같은 주소의 이름 내용을 callback이 바꿀 수 있으며 할당하지 않는다.</summary>
        public byte[] Name = [];

        /// <summary>현재 일반 WriteMemStream 현상 data·pos·end·alloc이다.</summary>
        public byte[] Stream = new byte[16];

        /// <summary>진단과 alloc보다 큰 물리 출력도 명시 입력이다.</summary>
        public byte[][] Buffers = [new byte[MissionBufferBytes], new byte[MissionBufferBytes]];

        /// <summary>이름 NUL 뒤 바이트까지 별도 보관하며 실제 문자열 길이를 제품이 계산한다.</summary>
        /// <param name="input">쉼표 12칸 입력이다.</param>
        public MissionDevices(string input)
        {
            string[] fields = MissionFields(input, ',');
            if (fields.Length != 12)
            {
                throw new InvalidOperationException("Mission fixture 입력 칸 수 오류");
            }
            Mode = fields[0];
            Current = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            NameCase = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            MissionPut(Stream, 0, MissionBuffers[0]);
            MissionPut(Stream, 4, uint.Parse(fields[3], CultureInfo.InvariantCulture));
            MissionPut(Stream, 8, uint.Parse(fields[4], CultureInfo.InvariantCulture));
            MissionPut(Stream, 12, uint.Parse(fields[5], CultureInfo.InvariantCulture));
            Sequence[1] = uint.Parse(fields[6], CultureInfo.InvariantCulture);
            Flags[1] = uint.Parse(fields[7], CultureInfo.InvariantCulture);
            Sequence[2] = uint.Parse(fields[8], CultureInfo.InvariantCulture);
            Flags[2] = uint.Parse(fields[9], CultureInfo.InvariantCulture);
            Manager = uint.Parse(fields[10], CultureInfo.InvariantCulture);
            Mutation = fields[11];
            uint[] lengths = [0, 3, 0, 31, 65535, 65536, 65537, 0];
            if (NameCase == 2)
            {
                Name = [(byte)'A', 0, (byte)'B', (byte)'C', 0];
            }
            else
            {
                Name = new byte[lengths[(int)NameCase] + 1];
                for (int index = 0; index + 1 < Name.Length; index++)
                {
                    Name[index] = (byte)(65 + index % 23);
                }
            }
            // 미사용 물리버퍼를 주변 cc바이트로 채워 보존되는지 확인한다
            foreach (byte[] output in Buffers)
            {
                Array.Fill(output, (byte)0xcc);
            }
        }

        /// <summary>현재 경계 진입을 문자 하나씩 순서대로 기록한다.</summary>
        /// <param name="text">기록할 본문이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>부모의 분기·조회는 제품에 맡기고 지정된 외부 효과만 적용한다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            if (Mutation == boundary + "-current2")
            {
                Current = 2;
            }
            else if (Mutation == boundary + "-current0")
            {
                Current = 0;
            }
            else if (Mutation == boundary + "-name")
            {
                Name[0] = (byte)'Z';
                Name[1] = 0;
                Name[2] = (byte)'Y';
            }
            else if (Mutation == "write1-sequence" && boundary == "write1")
            {
                Sequence[1] = 0x13579bdfu;
            }
            else if (Mutation == "write3-flags" && boundary == "write3")
            {
                Flags[1] = 0x2468ace0u;
            }
            else if (Mutation == "write4-flags" && boundary == "write4")
            {
                Flags[1] = 0x01020304u;
            }
            else if (Mutation == "capacity-current" && boundary == "capacity")
            {
                MissionPut(Stream, 0, MissionBuffers[1]);
                MissionPut(Stream, 4, 5);
                MissionPut(Stream, 8, 2);
                MissionPut(Stream, 12, (uint)MissionBufferBytes);
                Sequence[1] = 0x11223344u;
            }
            else if (Mutation == "write1-physical" && boundary == "write1")
            {
                MissionPut(Stream, 0, 0);
            }
            else if (Mutation == boundary + "-stop")
            {
                throw new MissionStop(boundary);
            }
        }

        /// <summary>같은 실제 일반 writer와 현재 용량 진단·현재 data·pos·end 읽기를 수행한다.</summary>
        /// <param name="selected">쓸 stream 토큰이다.</param>
        /// <param name="source">쓸 구간이다.</param>
        public void Write(uint selected, ReadOnlySpan<byte> source)
        {
            Assert.Equal(1u, selected);
            Writes++;
            Record("W:" + Writes + ':' + source.Length + ':' + MissionHash(source));
            Mutate("write" + Writes);
            MemStreamHooks hooks = new();
            hooks.Buffer = pointer =>
            {
                if (pointer == 0)
                {
                    throw new MissionStop("buffer0");
                }
                for (int index = 0; index < MissionBuffers.Length; index++)
                {
                    if (pointer == MissionBuffers[index])
                    {
                        return Buffers[index];
                    }
                }
                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Mission 현재 물리 버퍼 토큰");
            };
            hooks.Assertion = value =>
            {
                Assert.Equal(MemStreamAssertion.WriteCapacity, value);
                Record("A:36");
                Mutate("capacity");
            };
            try
            {
                RawMemStream.Write(Stream, source, (uint)source.Length, hooks);
            }
            catch (InvalidOperationException)
            {
                if (MissionGet(Stream, 0) == 0 && !source.IsEmpty)
                {
                    throw new MissionStop("buffer0");
                }
                throw;
            }
        }

        /// <summary>Guide·Mission 코드를 횟수·현재 조회하며 일반 쓰기 효과만 둔다.</summary>
        public FortMissionSaveHooks Hooks()
        {
            return new FortMissionSaveHooks
            {
                Current = () =>
                {
                    CurrentReads++;
                    return Current;
                },
                Name = () =>
                {
                    NameReads++;
                    if (NameCase == 7)
                    {
                        return null;
                    }
                    return Name;
                },
                Field = (selected, offset) =>
                {
                    if (selected == 0)
                    {
                        throw new MissionStop("mission0");
                    }
                    if (selected > 2 || (offset != 0x54 && offset != 0x58))
                    {
                        throw new ArgumentOutOfRangeException(nameof(selected), selected, "Mission 현재 필드 범위");
                    }
                    return offset == 0x58 ? Sequence[(int)selected] : Flags[(int)selected];
                },
                Write = (selected, source) => Write(selected, source),
                Assertion = value =>
                {
                    Assert.Equal(FortMissionSaveAssertion.NullName, value);
                    Record("A:155");
                    Mutate("null");
                },
            };
        }

        /// <summary>독립 진입점에 맞춰 제품 함수를 실행한다. 전체 Template 부모는 별도 모듈이 유지한다.</summary>
        public void Run()
        {
            FortMissionSaveHooks hooks = Hooks();
            if (Mode == "guide")
            {
                RawFortMissionSave.SaveFortMission(1, hooks);
            }
            else if (Mode == "name")
            {
                RawFortMissionSave.SaveFortMissionName(1, NameCase == 7 ? null : Name, hooks);
            }
            else if (Mode == "state")
            {
                RawFortMissionSave.SaveFortMissionState(1, Current, hooks);
            }
            else
            {
                throw new InvalidOperationException("Mission fixture 미등록 진입점");
            }
        }

        /// <summary>제품 실행 뒤 Guide·Mission의 모든 입력 필드와 일반 쓰기 횟수를 관측한다.</summary>
        public string State()
        {
            return Manager + "," + Current + ',' + Sequence[1] + ',' + Flags[1] + ',' + Sequence[2] + ',' + Flags[2] + ',' + Writes;
        }

        /// <summary>현재 16바이트 헤더·전체 물리 해시·앞뒤 주변 바이트를 원문과 대조한다.</summary>
        public string Raw()
        {
            string result = MissionHex(Stream);
            // 미사용 출력도 끝까지 cc바이트로 채워져 있는지 확인한다
            foreach (byte[] buffer in Buffers)
            {
                ReadOnlySpan<byte> bytes = buffer;
                result += '|' + MissionHash(bytes) + ':' + MissionHex(bytes[..64]) + ':' + MissionHex(bytes[^16..]);
            }
            return result;
        }
    }

    /// <summary>최초 115입력·정밀도 30관측의 정상 반환과 명시 중단까지 모든 제품 효과를 비교한다.</summary>
    [Fact]
    public void IndependentNative230()
    {
        string[][] rows = X86Fixture.Read("fort-mission-save");
        int compared = 0;
        int stopped = 0;
        HashSet<string> unique = [];
        // 줄의 앞 두 칸만 입력으로 쓰고 관측열은 제품 실행 뒤에 비교한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Assert.True(unique.Add(row[0] + "\t" + row[1]));
            MissionDevices devices = new(row[1]);
            string completion = "1";
            try
            {
                devices.Run();
            }
            catch (MissionStop stop)
            {
                completion = stop.Cause;
                stopped++;
            }
            Assert.Equal(row[2], completion);
            Assert.Equal(devices.State(), row[3]);
            Assert.Equal(devices.Raw(), row[4]);
            Assert.Equal(devices.Trace, row[5]);
            compared++;
        }
        Assert.Equal(230, compared);
        Assert.Equal(12, stopped);
    }

    /// <summary>선택되지 않는 토큰은 위치를 요구하지 않으며 없는 장치·원본 물리 범위는 임의 성공으로 만들지 않는다.</summary>
    [Fact]
    public void CurrentAndPhysicalBoundaries()
    {
        FortMissionSaveHooks empty = new();
        uint queries = 0;
        empty.Current = () =>
        {
            queries++;
            return 0u;
        };
        RawFortMissionSave.SaveFortMission(99, empty);
        Assert.Equal(1u, queries);
        MissionDevices devices = new("guide,1,1,0,0,64,1,2,3,4,11,write3-current2");
        devices.Run();
        Assert.Equal(2u, devices.CurrentReads);
        Assert.Equal(1u, devices.NameReads);
        Assert.Equal(2u, devices.Current);
        Assert.Equal(13u, MissionGet(devices.Stream, 4));
        Assert.Equal(1, devices.Buffers[0][5]);
        Assert.Equal(2, devices.Buffers[0][9]);
        byte[] noNul = [(byte)'A', (byte)'B'];
        uint writes = 0;
        FortMissionSaveHooks bounds = new() { Write = (selected, source) => writes++ };
        bool rejected = false;
        try
        {
            RawFortMissionSave.SaveFortMissionName(1, noNul, bounds);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected && writes == 0);
        rejected = false;
        try
        {
            RawFortMissionSave.SaveFortMissionName(1, null, bounds);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected && writes == 0);
        byte[] blank = [0];
        List<int> lengths = [];
        bounds.Write = (selected, source) =>
        {
            Assert.Equal(77u, selected);
            lengths.Add(source.Length);
        };
        RawFortMissionSave.SaveFortMissionName(77, blank, bounds);
        Assert.Equal(new[] { 2, 0 }, lengths);
    }
}
