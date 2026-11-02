using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Deck 로더(<see cref="RawFortDeckLoad"/>)를 독립 원시 입력과 대조한다.
/// cpppj <c>FortDeckLoadTests</c>의 최초 1020행 전수와 선택·공유 FullDeck 직접 검사를 옮겼다. (F2b, 2026-11-02)
/// </summary>
public sealed class FortDeckLoadTests
{
    /// <summary>빈 필드도 보존하는 fixture의 정확한 구분자 분리다. 원문 필드를재해석 없이 분리한다.</summary>
    /// <param name="text">분리할 원문이다.</param>
    /// <param name="delimiter">구분자다.</param>
    private static string[] DeckFields(string text, char delimiter)
    {
        List<string> result = [];
        int start = 0;
        // 다음 필드의 첫 문자다
        for (; ; )
        {
            int at = text.IndexOf(delimiter, start);
            result.Add(at < 0 ? text[start..] : text.Substring(start, at - start));
            if (at < 0)
            {
                return [.. result];
            }
            start = at + 1;
        }
    }

    /// <summary>네이티브에 공급한 외부 장치·실제 버퍼 입력만 보관한다. 부모 분기나 출력은 미리 계산하지 않는다.</summary>
    private sealed class DeckDevices
    {
        /// <summary>나머지 원시 DWORD 입력이다(slot은 별도 signed).</summary>
        public uint[] Values { get; } = new uint[11];

        /// <summary>고정 this의 입력 슬롯(signed)이다.</summary>
        public int Slot { get; set; }

        /// <summary>명시 callback 효과와 실제 호출 trace다.</summary>
        public string Mutation;

        /// <summary>실제 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>본문이 현재값으로 읽는 같은 장치 상태다.</summary>
        public uint Manager { get; set; }

        /// <summary>선택 반환 번호다.</summary>
        public uint Selected { get; set; }

        /// <summary>현재 깊이다.</summary>
        public uint Depth { get; set; }

        /// <summary>FullDeck 여부다.</summary>
        public uint Full { get; set; }

        /// <summary>현재 변환 토큰이다.</summary>
        public uint Conversion { get; set; }

        /// <summary>두 실제 MemStream 필드다.</summary>
        public byte[][] Streams { get; } = [new byte[16], new byte[16]];

        /// <summary>논리 end보다 긴 원시 물리 입력이다.</summary>
        public byte[][] Buffers { get; } = [new byte[2048], new byte[2048]];

        /// <summary>서로 다른 현재 변환표 입력이다.</summary>
        public byte[][] Tables { get; } = [new byte[255], new byte[255]];

        /// <summary>1기반 진단·append callback의 명시 효과를 구별한다.</summary>
        public uint Assertions { get; set; }

        /// <summary>복원 추가 순번이다.</summary>
        public uint Appends { get; set; }

        /// <summary>
        /// fixture의 공급 입력을 원래 배치로 구성한다. 제품 기대를 생성하지 않는다.
        /// </summary>
        /// <param name="fields">입력 12개 문자열이다.</param>
        public DeckDevices(string[] fields)
        {
            Slot = int.Parse(fields[0], CultureInfo.InvariantCulture);
            for (int index = 1; index < Values.Length; index++)
            {
                Values[index] = uint.Parse(fields[index], CultureInfo.InvariantCulture);
            }
            Mutation = fields[11];
            Manager = Values[1];
            Selected = Values[3];
            Depth = Values[10];
            Full = Values[4];
            Conversion = Values[9];
            for (int index = 0; index < Streams.Length; index++)
            {
                // 두 선택 반환과 서로 다른 버퍼를 원문 입력대로 준비한다
                WriteU32(Streams[index], 0, Values[5] != 0 ? (uint)(index + 1) : 0);
                WriteU32(Streams[index], 4, Values[7]);
                WriteU32(Streams[index], 8, Values[6]);
                WriteU32(Streams[index], 12, 2048);
                Array.Fill(Buffers[index], (byte)(index == 0 ? 0xcc : 0xdd));
                List<byte> types = [];
                // 아래 목록은 제품 출력이 아닌 네이티브에도 공급한 원시 파일 입력이다
                if (Values[8] == 2)
                {
                    types.AddRange([0, 1, 245, 246, 254, 255]);
                }
                else if (Values[8] == 3)
                {
                    for (int type = 0; type < 255; type++)
                    {
                        types.Add((byte)type);
                    }
                }
                else if (Values[8] != 1)
                {
                    types.Add(75);
                }
                foreach (int position in new[] { 0, 512 })
                {
                    // callback의 pos 512 입력도 같은 물리 버퍼 안에 놓는다
                    Buffers[index][position] = (byte)types.Count;
                    for (int item = 0; item < types.Count; item++)
                    {
                        int at = position + 1 + item * 4;
                        Buffers[index][at] = types[item];
                        Buffers[index][at + 1] = (byte)(item * 19 + 7);
                        Buffers[index][at + 2] = (byte)(item * 37 + 128);
                        Buffers[index][at + 3] = (byte)(item * 53 + 255); // 네 저장 BYTE를 독립 입력으로 쓴다
                    }
                }
            }
            for (int table = 0; table < Tables.Length; table++)
            {
                for (int type = 0; type < 255; type++)
                {
                    Tables[table][type] = type == 0 ? (byte)9 : type == 1 ? (byte)0 : type == 245 ? (byte)5 : (byte)(table == 0 ? type : type * 7 + 9); // 0·245·246 경계의 명시 변환 입력이다
                }
            }
        }

        /// <summary>네이티브와 같은 한 호출의 실제 텍스트를 순서대로 기록한다.</summary>
        /// <param name="text">기록할 항목이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>명시 callback 뒤의 현재값만 바꾼다. 부모의 후속 조건을 대신 판정하지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            int separator = Mutation.IndexOf('-');
            if (separator < 0)
            {
                return;
            }
            string trigger = Mutation[..separator];
            string action = Mutation[(separator + 1)..];
            // 독립 효과의 경계와 대상이다
            string plain = boundary.TrimEnd('0', '1', '2', '3', '4', '5', '6', '7', '8', '9');
            if (trigger != boundary && trigger != plain)
            {
                return;
            }
            if (action == "stop")
            {
                throw new DeckStop();
            }
            if (action.StartsWith("manager", StringComparison.Ordinal))
            {
                Manager = uint.Parse(action[7..], CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("conversion", StringComparison.Ordinal))
            {
                Conversion = uint.Parse(action[10..], CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("depth", StringComparison.Ordinal))
            {
                Depth = uint.Parse(action[5..], CultureInfo.InvariantCulture);
            }
            else if (action.StartsWith("full", StringComparison.Ordinal))
            {
                Full = uint.Parse(action[4..], CultureInfo.InvariantCulture);
            }
            else if (action == "stream1")
            {
                Selected = 1;
            }
            else if (action == "dataB")
            {
                WriteU32(Streams[(int)Selected], 0, 2);
            }
            else if (action.StartsWith("pos", StringComparison.Ordinal))
            {
                WriteU32(Streams[(int)Selected], 4, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else if (action.StartsWith("end", StringComparison.Ordinal))
            {
                WriteU32(Streams[(int)Selected], 8, uint.Parse(action[3..], CultureInfo.InvariantCulture));
            }
            else
            {
                throw new InvalidOperationException("Deck 검사 변경 입력 미해석");
            }
        }

        /// <summary>실제 부모에 원시 메모리와 네이티브와 같은 외부 응답만 연결한다.</summary>
        public FortDeckLoadHooks Hooks() => new()
        {
            Manager = fixedSlot =>
            {
                Assert.Equal(Slot, fixedSlot);
                return Manager;
            },
            Assertion = kind =>
            {
                Record("A:" + ((uint)kind).ToString(CultureInfo.InvariantCulture));
                Mutate("assert" + (++Assertions).ToString(CultureInfo.InvariantCulture));
            },
            Deck = owner =>
            {
                Record("G:" + owner.ToString(CultureInfo.InvariantCulture) + ':' + Values[2].ToString(CultureInfo.InvariantCulture));
                Mutate("get");
                return Values[2];
            },
            Select = (current, name, writable) =>
            {
                Assert.Equal("Deck", name);
                Assert.Equal(1, writable);
                Record("S:" + current.ToString(CultureInfo.InvariantCulture) + ":Deck:1");
                Mutate("select");
                return Selected + 1;
            },
            Access = (token, operation) =>
            {
                if (token is < 1 or > 2)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), token, "Deck 검사 stream 물리 범위");
                }
                MemStreamHooks memory = new()
                {
                    Buffer = data =>
                    {
                        if (data is < 1 or > 2)
                        {
                            throw new ArgumentOutOfRangeException(nameof(data), data, "Deck 검사 data 물리 범위");
                        }
                        return Buffers[data - 1];
                    },
                };
                operation(Streams[token - 1], memory);
            },
            Clear = token =>
            {
                Record("clear:" + token.ToString(CultureInfo.InvariantCulture));
                Mutate("clear");
            },
            Open = (token, seed) =>
            {
                Record("open:" + token.ToString(CultureInfo.InvariantCulture) + ':' + seed.ToString(CultureInfo.InvariantCulture));
                Mutate("open");
            },
            Full = token =>
            {
                Record("full:" + token.ToString(CultureInfo.InvariantCulture));
                Mutate("full");
            },
            PrepareTypeNames = fixedSlot =>
            {
                Record("P:" + fixedSlot.ToString(CultureInfo.InvariantCulture));
                Mutate("prepare");
            },
            ConversionBuffer = token =>
            {
                if (token != 11 && token != 22)
                {
                    throw new ArgumentOutOfRangeException(nameof(token), token, "Deck 검사 conversion 물리 범위");
                }
                return Tables[token == 11 ? 0 : 1];
            },
            Append = (token, type, chance, power, remaining) =>
            {
                Record("E:" + token.ToString(CultureInfo.InvariantCulture) + ':' + type.ToString(CultureInfo.InvariantCulture) + ':' + chance.ToString(CultureInfo.InvariantCulture) + ':' + power.ToString(CultureInfo.InvariantCulture) + ':' + remaining.ToString(CultureInfo.InvariantCulture));
                Mutate("append" + (++Appends).ToString(CultureInfo.InvariantCulture));
            },
            ReleaseConversion = token =>
            {
                Record("X:" + token.ToString(CultureInfo.InvariantCulture));
                Mutate("free");
            },
            Refresh = owner =>
            {
                Record("R:" + owner.ToString(CultureInfo.InvariantCulture));
                Mutate("refresh");
            },
            LoadingDepthChanged = value => Record("L:" + value.ToString(CultureInfo.InvariantCulture)),
        };

        /// <summary>두 현재 DWORD를 제품과 같은 실체로 빌린다.</summary>
        public FortDeckLoadGlobals Globals() => new()
        {
            GetDepth = () => Depth,
            SetDepth = value => Depth = value,
            GetConversion = () => Conversion,
            SetConversion = value => Conversion = value,
            FullDeck = () => Full,
        };

        /// <summary>비정렬 DWORD를 원본 little endian으로 읽는다. 필요한 DWORD만 조합한다.</summary>
        /// <param name="raw">읽을 바이트다.</param>
        /// <param name="offset">읽을 위치다.</param>
        public static uint DeckU(ReadOnlySpan<byte> raw, int offset)
        {
            uint value = 0;
            for (int index = 0; index < 4; index++)
            {
                value |= (uint)raw[offset + index] << (index * 8);
            }
            return value;
        }

        /// <summary>명시 입력의 한 DWORD만 쓴다. 이웃 필드는 보존한다.</summary>
        /// <param name="raw">쓸 바이트다.</param>
        /// <param name="offset">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public static void WriteU32(Span<byte> raw, int offset, uint value)
        {
            for (int index = 0; index < 4; index++)
            {
                raw[offset + index] = (byte)(value >> (index * 8));
            }
        }

        /// <summary>
        /// 지정 하위 callback에서만 발생하는 검사 전용 부분 중단이다.
        /// </summary>
        internal sealed class DeckStop : Exception;
    }

    /// <summary>
    /// 선택되지 않은 장치 누락을 성공으로 요구하지 않고 도달한 호스트 오류만 관측한다.
    /// </summary>
    /// <param name="function">실행할 동작이다.</param>
    public static bool Rejects(Action function)
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

    /// <summary>
    /// 고정된 최초 PE 입력·순서·현재 필드와 명시 부분 중단·별도 cleanup을 모두 비교한다.
    /// </summary>

    /// <summary>
    /// 고정된 최초 PE 입력·순서·현재 필드와 명시 부분 중단·별도 cleanup을 모두 비교한다.
    /// </summary>
    [Fact]
    public void IndependentNativeParent()
    {
        string[][] rows = X86Fixture.Read("fort-deck-load");
        int compared = 0;
        // 앞 두 열만 입력이고 뒤 다섯 열은 제품 전에 관측한 원문 출력이다
        foreach (string[] row in rows)
        {
            Assert.Equal(7, row.Length);
            DeckDevices devices = new(DeckFields(row[1], ','));
            FortDeckLoadHooks hooks = devices.Hooks();
            FortDeckLoadGlobals globals = new()
            {
                GetDepth = () => devices.Depth,
                SetDepth = value => devices.Depth = value,
                GetConversion = () => devices.Conversion,
                SetConversion = value => devices.Conversion = value,
                FullDeck = () => devices.Full,
            };
            bool completed = true;
            try
            {
                RawFortDeckLoad.LoadFortDeck(devices.Slot, globals, hooks);
            }
            catch (DeckDevices.DeckStop)
            {
                completed = false;
            }
            Assert.Equal(row[2] == "1", completed);
            Assert.Equal(row[5], devices.Trace);
            string[] state = DeckFields(row[3], ',');
            Assert.Equal(5, state.Length);
            Assert.Equal(uint.Parse(state[0], CultureInfo.InvariantCulture), devices.Manager);
            Assert.Equal(uint.Parse(state[1], CultureInfo.InvariantCulture), devices.Selected);
            Assert.Equal(uint.Parse(state[2], CultureInfo.InvariantCulture), devices.Depth);
            Assert.Equal(uint.Parse(state[3], CultureInfo.InvariantCulture), devices.Full);
            Assert.Equal(uint.Parse(state[4], CultureInfo.InvariantCulture), devices.Conversion);
            string[] streams = DeckFields(row[4], ',');
            Assert.Equal(8, streams.Length);
            for (int stream = 0; stream < 2; stream++)
            {
                for (int field = 0; field < 4; field++)
                {
                    Assert.Equal(uint.Parse(streams[stream * 4 + field], CultureInfo.InvariantCulture), DeckDevices.DeckU(devices.Streams[stream], field * 4)); // 현재 data·pos·end·capacity도 비교한다
                }
            }
            if (!completed)
            {
                devices.Trace = "";
                RawFortDeckLoad.UnwindFortDeckLoad(globals, hooks);
                Assert.Equal(row[6], devices.Depth.ToString(CultureInfo.InvariantCulture) + ':' + devices.Trace); // 부분 중단 깊이와 cleanup 뒤 깊이를 같은 판정으로 합치지 않는다
            }
            else
            {
                Assert.Equal("-", row[6]);
            }
            compared++;
        }
        Assert.Equal(1020, compared);
    }

    /// <summary>
    /// FullDeck도 Select·Clear·Open을 생략하지 않고 비선택 직접 읽기·TypeNames는 요구하지 않는다.
    /// </summary>
    [Fact]
    public void SelectedDevicesAndPhysicalBounds()
    {
        DeckDevices devices = new(DeckFields("1,11,101,0,1,1,1,0,0,11,0,none", ','));
        FortDeckLoadHooks hooks = devices.Hooks();
        hooks.Access = null;
        hooks.PrepareTypeNames = null;
        hooks.Append = null;
        hooks.ConversionBuffer = null;
        hooks.ReleaseConversion = null;
        hooks.Refresh = null;
        RawFortDeckLoad.LoadFortDeck(1, devices.Globals(), hooks);
        Assert.Equal(0u, devices.Depth);
        Assert.Equal(11u, devices.Conversion);
        Assert.Equal("L:1;G:1:101;S:11:Deck:1;clear:101;open:101:0;full:101;L:0", devices.Trace);
        devices.Trace = "";
        hooks.Open = null;
        Assert.True(Rejects(() => RawFortDeckLoad.LoadFortDeck(1, devices.Globals(), hooks)));
        Assert.Equal(1u, devices.Depth);
        Assert.Equal("L:1;G:1:101;S:11:Deck:1;clear:101", devices.Trace);
        RawFortDeckLoad.UnwindFortDeckLoad(devices.Globals(), hooks);
        Assert.Equal(0u, devices.Depth);
        DeckDevices physical = new(DeckFields("1,11,101,0,0,1,1,0,0,11,0,none", ','));
        FortDeckLoadHooks bounded = physical.Hooks();
        bounded.PrepareTypeNames = _ =>
        {
            DeckDevices.WriteU32(physical.Streams[0], 4, 2048);
        };
        Assert.True(Rejects(() => RawFortDeckLoad.LoadFortDeck(1, physical.Globals(), bounded)));
        Assert.Equal(1u, physical.Depth);
        Assert.Equal(2048u, DeckDevices.DeckU(physical.Streams[0], 4)); // end 1로 읽기를 미리 생략하지 않고 실제 물리 끝에서만 거부한다
    }

    /// <summary>
    /// Clear·Open 뒤 현재 Client DWORD를 읽고 최초 캡처나 독립 fallback으로 분기를 고정하지 않는다.
    /// </summary>
    [Fact]
    public void SharedFullDeckReadsAfterOpen()
    {
        uint fullValue = 0;
        foreach (uint value in new uint[] { 0x80000000u, 0xffffffffu, 0u })
        {
            // 동일 부모·참조에서 외부 Open 효과가 full·저장 Deck 가지를 바꾼다
            DeckDevices devices = new(DeckFields("1,11,101,0,0,1,1,0,0,0,0,none", ','));
            FortDeckLoadGlobals globals = new()
            {
                GetDepth = () => devices.Depth,
                SetDepth = depth => devices.Depth = depth,
                GetConversion = () => devices.Conversion,
                SetConversion = conversion => devices.Conversion = conversion,
                FullDeck = () => fullValue,
            };
            devices.Trace = "";
            devices.Appends = 0;
            DeckDevices.WriteU32(devices.Streams[0], 4, 0);
            FortDeckLoadHooks hooks = devices.Hooks();
            hooks.Open = (token, seed) =>
            {
                devices.Record("open:" + token.ToString(CultureInfo.InvariantCulture) + ':' + seed.ToString(CultureInfo.InvariantCulture));
                fullValue = value;
            };
            RawFortDeckLoad.LoadFortDeck(1, globals, hooks);
            Assert.Equal(value, fullValue);
            Assert.Equal(0u, devices.Full);
            Assert.Equal(0u, devices.Depth);
            Assert.Equal(value != 0, devices.Trace.Contains("full:101", StringComparison.Ordinal));
            Assert.Equal(value != 0 ? 0u : 1u, devices.Appends);
        }
    }
}
