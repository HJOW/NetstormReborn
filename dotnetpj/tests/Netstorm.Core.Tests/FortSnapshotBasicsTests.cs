using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 스냅샷 기초(TypeNames·Deck writer, <see cref="RawFortSnapshotBasics"/>)를 독립 원본 입력과 대조한다.
/// cpppj <c>FortSnapshotBasicsTests</c>의 전수 2060행과 저장소 수명·부분 쓰기 직접 2종을 옮겼다. (F2c-4, 2026-11-03)
/// </summary>
public sealed class FortSnapshotBasicsTests
{
    /// <summary>독립 TSV의 마지막 빈 칸까지 그대로 분리한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    private static List<string> SnapshotFields(string source)
    {
        List<string> result = [];
        int start = 0;
        // 끝 경계까지 원문 필드로 둔다
        for (; ; )
        {
            int at = source.IndexOf('\t', start);
            result.Add(at < 0 ? source[start..] : source.Substring(start, at - start));
            if (at < 0)
            {
                return result;
            }
            start = at + 1;
        }
    }

    /// <summary>물리 필드를 지정 DWORD로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint SnapshotRead(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 낮은 바이트만 조합한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>물리 필드에 지정 DWORD만 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void SnapshotPut(Span<byte> bytes, int at, uint value)
    {
        // 직접 필드만 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>독립 관측과 같은 소문자 hex를 만든다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string SnapshotHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder text = new();
        // 바이트마다 두 글자로 기록한다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>선택된 진단 경계에서만 종료하는 검사 전용 예외다.</summary>
    internal sealed class SnapshotStop : Exception;

    /// <summary>독립 PE 입력을 같은 위치에 두고 실제 저장소·쓰기 stream을 연결한다.</summary>
    private sealed class SnapshotDevices
    {
        /// <summary>고정 this 슬롯이다.</summary>
        public int Slot;

        /// <summary>타입 수다.</summary>
        public int TypeCount;

        /// <summary>현재 manager다.</summary>
        public uint Manager;

        /// <summary>현재 owner다.</summary>
        public uint Mine;

        /// <summary>State+17과 실제 0x160바이트 Deck 입력이다.</summary>
        public byte[] State = new byte[32];

        /// <summary>0x14c 카운트 Deck 입력이다.</summary>
        public byte[] Deck = new byte[352];

        /// <summary>원본 stream 필드와 초기 cc 물리 버퍼다.</summary>
        public byte[] Stream = new byte[16];

        /// <summary>4096바이트 출력 버퍼다.</summary>
        public byte[] Output = new byte[4096];

        /// <summary>명시 콜백 변경과 현재 선택·취소 trace다.</summary>
        public string Mutation = "fixed";

        /// <summary>현재 선택이다.</summary>
        public string Current = "old";

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>명시 호출을 순서대로 기록한다.</summary>
        /// <param name="text">기록할 본문이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>독립 입력을 callback 효과에 적용한다. 부모의 조건·결과는 바꾸지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            string prefix = boundary + '-';
            if (Mutation == prefix + "manager22")
            {
                Manager = 22;
            }
            else if (Mutation == prefix + "mine8")
            {
                Mine = 8;
            }
            else if (Mutation == prefix + "version0")
            {
                State[17] = 0;
            }
            else if (Mutation == prefix + "version1")
            {
                State[17] = 1;
            }
            else if (Mutation == prefix + "typecount2")
            {
                TypeCount = 2;
            }
            else if (Mutation == prefix + "typecount0")
            {
                TypeCount = 0;
            }
            else if (Mutation == prefix + "deckcount2")
            {
                SnapshotPut(Deck, 0x14c, 2);
            }
            else if (Mutation == prefix + "deckcount0")
            {
                SnapshotPut(Deck, 0x14c, 0);
            }
            else if (Mutation == prefix + "capacity64")
            {
                SnapshotPut(Stream, 12, 64);
            }
        }

        /// <summary>제품 표시·조회 기기가 직접 읽는 이름·State·쓰기 stream을 연결한다.</summary>
        public FortSnapshotBasicsHooks Hooks()
        {
            return new FortSnapshotBasicsHooks
            {
                Manager = selected =>
                {
                    Assert.Equal(Slot, selected);
                    return Manager;
                },
                Assertion = kind =>
                {
                    int line = kind == FortSnapshotAssertion.MissingNamesTemplate ? 586 : kind == FortSnapshotAssertion.NamesNotMine ? 587 : kind == FortSnapshotAssertion.MissingDeckTemplate ? 1220 : 36;
                    Record("assert:" + line);
                    Mutate("assert");
                    if (line == 36 && Mutation == "assert-stop")
                    {
                        throw new SnapshotStop();
                    }
                },
                Create = (selected, name) =>
                {
                    Record("create:" + selected + ':' + name);
                    Current = "Created";
                    Mutate("create");
                    return 1u;
                },
                State = selected =>
                {
                    Record("state:" + selected);
                    Current = "State";
                    Mutate("state");
                    return State;
                },
                TypeName = index =>
                {
                    Record("name:" + index);
                    Mutate("name" + index);
                    string[] names = ["", "A", "abcdE", "\u0080\u00ffAz", "Netstorm", "Q\0ignored"];
                    return names[index % names.Length];
                },
                RefreshDeck = owner =>
                {
                    Record("refresh:" + owner);
                    Mutate("refresh");
                },
                GetDeck = owner =>
                {
                    Record("get:" + owner);
                    Mutate("get");
                    return 2u;
                },
                Deck = token =>
                {
                    Assert.Equal(2u, token);
                    return Deck;
                },
                Write = (token, bytes) =>
                {
                    Assert.Equal(1u, token);
                    Assert.Equal(1, bytes.Length);
                    Record("write:1:" + SnapshotHex(bytes));
                    uint position = SnapshotRead(Stream, 4);
                    Assert.True(position + (uint)bytes.Length <= SnapshotRead(Stream, 12));
                    Output[position] = bytes[0];
                    SnapshotPut(Stream, 4, position + 1);
                    if (SnapshotRead(Stream, 8) < position + 1)
                    {
                        SnapshotPut(Stream, 8, position + 1);
                    }
                    Mutate("write");
                },
                Access = (token, operation) =>
                {
                    Assert.Equal(1u, token);
                    MemStreamHooks memory = new()
                    {
                        Buffer = pointer =>
                        {
                            Assert.Equal(77u, pointer);
                            return Output;
                        },
                    };
                    operation(Stream, memory);
                },
            };
        }
    }

    /// <summary>실패 위치를 상시 기기로 옮기는지 확인한다.</summary>
    /// <param name="function">실행할 동작이다.</param>
    private static bool SnapshotRejects(Action function)
    {
        try
        {
            function();
        }
        catch (Exception)
        {
            return true;
        }
        return false;
    }

    /// <summary>signed 이름 hash·현재 count·일반 쓰기를 원문 실행 만든 출력과 비교한다.</summary>
    [Fact]
    public void IndependentNative()
    {
        string[][] rows = X86Fixture.Read("fort-snapshot-basics");
        int compared = 0;
        // 줄 2개만 같이 입력하며 줄 2개는 독립 PE 관측이다
        foreach (string[] row in rows)
        {
            try
            {
            Assert.Equal(24, row.Length);            SnapshotDevices devices = new();
            devices.Slot = int.Parse(row[2], CultureInfo.InvariantCulture);
            devices.Manager = uint.Parse(row[3], CultureInfo.InvariantCulture);
            devices.Mine = uint.Parse(row[4], CultureInfo.InvariantCulture);
            devices.State[17] = (byte)uint.Parse(row[5], CultureInfo.InvariantCulture);
            devices.TypeCount = int.Parse(row[6], CultureInfo.InvariantCulture);
            SnapshotPut(devices.Deck, 0x14c, unchecked((uint)int.Parse(row[7], CultureInfo.InvariantCulture)));
            for (int index = 0; index < 66; index++)
            {
                for (int offset = 0; offset < 5; offset++)
                {
                    devices.Deck[index * 5 + offset] = (byte)(index * 19 + offset * 37 + 3);
                }
            }
            SnapshotPut(devices.Stream, 0, 77);
            SnapshotPut(devices.Stream, 4, uint.Parse(row[8], CultureInfo.InvariantCulture));
            SnapshotPut(devices.Stream, 8, uint.Parse(row[9], CultureInfo.InvariantCulture));
            SnapshotPut(devices.Stream, 12, uint.Parse(row[10], CultureInfo.InvariantCulture));
            Array.Fill(devices.Output, (byte)0xcc);
            devices.Mutation = row[11];
            bool completed = true;
            try
            {
                if (row[1] == "names")
                {
                    RawFortSnapshotBasics.SnapshotFortTypeNames(devices.Slot, devices.Mine, devices.TypeCount, devices.Hooks());
                }
                else
                {
                    Assert.Equal("deck", row[1]);
                    RawFortSnapshotBasics.SnapshotFortDeck(devices.Slot, devices.Hooks());
                }
            }
            catch (SnapshotStop)
            {
                completed = false;
            }
            Assert.Equal(completed, row[12] == "1");
            Assert.Equal(uint.Parse(row[13], CultureInfo.InvariantCulture), devices.Manager);
            Assert.Equal(uint.Parse(row[14], CultureInfo.InvariantCulture), devices.Mine);
            Assert.Equal(uint.Parse(row[15], CultureInfo.InvariantCulture), devices.State[17]);
            Assert.Equal(unchecked((uint)devices.TypeCount), uint.Parse(row[16], CultureInfo.InvariantCulture));
            Assert.Equal(SnapshotRead(devices.Deck, 0x14c), uint.Parse(row[17], CultureInfo.InvariantCulture));
            Assert.Equal(row[18], devices.Current);
            Assert.Equal(uint.Parse(row[19], CultureInfo.InvariantCulture), SnapshotRead(devices.Stream, 4));
            Assert.Equal(uint.Parse(row[20], CultureInfo.InvariantCulture), SnapshotRead(devices.Stream, 8));
            Assert.Equal(row[21], SnapshotHex(devices.Output.AsSpan(0, (int)SnapshotRead(devices.Stream, 8))));
            Assert.Equal(row[22], devices.Trace);
            if (completed)
            {
                Assert.Equal(0, int.Parse(row[23], CultureInfo.InvariantCulture));
            }
            else
            {
                Assert.True(int.Parse(row[23], CultureInfo.InvariantCulture) < 0);
            }
            compared++;
            }
            catch (Exception error) when (error is not SnapshotStop)
            {
                throw new InvalidOperationException("행 " + compared + " 입력 " + row[0] + " " + row[1], error);
            }
        }
        Assert.Equal(2060, compared);
    }
}
