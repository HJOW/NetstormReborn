using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 객체 부모 저장(<see cref="RawFortObjectsSave"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortObjectsSaveTests</c>의 전수 714행과 명시 미선택·누락 경계 직접 1종을 옮겼다. (F2c-5, 2026-11-03)
/// </summary>
public sealed class FortObjectsSaveTests
{
    /// <summary>최초 x86 입력의 가상 버퍼 주소이며 호스트 포인터로 역참조하지 않는다.</summary>
    private const uint ObjectsBufferBase = 0x12008000U;

    /// <summary>마지막 빈 칸도 유지하는 독립 fixture의 필드 분리다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static List<string> ObjectsFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 각 구분자 뒤 원문을 순서대로 읽는다
        for (; ; )
        {
            int at = source.IndexOf(separator, start);
            result.Add(at < 0 ? source[start..] : source[start..at]);
            if (at < 0)
            {
                return result;
            }
            start = at + 1;
        }
    }

    /// <summary>little endian DWORD 네 BYTE만 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint ObjectsGet(ReadOnlySpan<byte> bytes, int at)
    {
        uint value = 0;
        // 낮은 BYTE부터 원본 DWORD를 조립한다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>지정 DWORD만 써 주변 raw 필드를 보존한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void ObjectsPut(Span<byte> bytes, int at, uint value)
    {
        // 원본 네 BYTE 순서다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>전체 stream·물리 출력의 모든 BYTE를 x86과 같은 형식으로 직렬화한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string ObjectsHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 인접 두 글자가 하나의 실제 BYTE다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>원문 emu_stop에 대응하는 지정 외부 경계의 중단 표식이다.</summary>
    internal sealed class ObjectsStop : Exception;

    /// <summary>부모의 조건을 대신 계산하지 않고 최초 독립 입력의 하위 응답만 공급한다.</summary>
    private sealed class ObjectsDevices
    {
        /// <summary>고정 this 슬롯이다.</summary>
        public int Slot;

        /// <summary>외부 Temp 생성 응답의 순번이다.</summary>
        public int Region = -1;

        /// <summary>독립 입력의 현재 manager다.</summary>
        public uint Manager;

        /// <summary>독립 입력의 현재 선택이다.</summary>
        public uint Mine;

        /// <summary>독립 입력의 현재 fort다.</summary>
        public uint Fort;

        /// <summary>find 동작(0은 전부 미발견, 1은 전부 발견, 2는 짝수만 발견)이다.</summary>
        public uint Pattern;

        /// <summary>Temp·Chaff 생성 시 노드에 둘 초기 위치다.</summary>
        public uint Position;

        /// <summary>Temp·Chaff 생성 시 노드에 둘 초기 끝이다.</summary>
        public uint End;

        /// <summary>Temp·Chaff 생성 시 노드에 둘 초기 용량이다.</summary>
        public uint Capacity;

        /// <summary>독립 입력과 부모가 직접 쓰는 현재 version이다.</summary>
        public uint Version = 0xaabbccddU;

        /// <summary>부모가 직접 읽고 쓰는 21개 실제 stream 헤더다.</summary>
        public byte[][] Nodes = [new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16], new byte[16]];

        /// <summary>각 stream의 물리 64바이트 출력과 주변 0xCC 바이트다.</summary>
        public byte[][] Output = [new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64], new byte[64]];

        /// <summary>명시 하위 manager의 현재 선택 입력이다.</summary>
        public Dictionary<uint, uint> Currents = new() { [0] = 90, [11] = 91, [22] = 92 };

        /// <summary>각 실제 하위 진입의 1기반 횟수다.</summary>
        public Dictionary<string, uint> Calls = [];

        /// <summary>동기 access의 현재 stream이다.</summary>
        public uint Active;

        /// <summary>명시 Temp 생성 횟수다.</summary>
        public uint Temps;

        /// <summary>지정 callback 효과다.</summary>
        public string Mutation = "";

        /// <summary>관측된 전체 순서다.</summary>
        public string Trace = "";

        /// <summary>fixture의 입력열만 읽어 상태를 구성하며 관측열은 읽지 않는다.</summary>
        /// <param name="input">입력 9칸이다.</param>
        public ObjectsDevices(string input)
        {
            string[] fields = [.. ObjectsFields(input, ',')];
            if (fields.Length != 9)
            {
                throw new ArgumentException("Objects 부모 입력 칸 수 오류", nameof(input));
            }
            Slot = int.Parse(fields[0], CultureInfo.InvariantCulture);
            Manager = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            Mine = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            Fort = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            Pattern = uint.Parse(fields[4], CultureInfo.InvariantCulture);
            Position = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            End = uint.Parse(fields[6], CultureInfo.InvariantCulture);
            Capacity = uint.Parse(fields[7], CultureInfo.InvariantCulture);
            Mutation = fields[8];
            // 미사용 노드·출력도 x86 최초 입력과 동일하게 초기화한다
            for (uint token = 1; token <= 21; token++)
            {
                ObjectsPut(Nodes[token - 1], 0, ObjectsBufferBase + token * 64);
                ObjectsPut(Nodes[token - 1], 12, 64);
                Array.Fill(Output[token - 1], (byte)0xcc);
            }
        }

        /// <summary>실제 현재 fort는 최종 분기에서만 getter로 읽어 하위 callback 뒤의 변경을 보존한다.</summary>
        public FortObjectsSaveGlobals Globals()
        {
            return new FortObjectsSaveGlobals
            {
                TemplateOwner = Mine,
                GetVersion = () => Version,
                SetVersion = value => Version = value,
                Fort = () => Fort,
            };
        }

        /// <summary>원본 하위 호출의 실제 순서와 횟수만 기록한다.</summary>
        /// <param name="kind">경계 종류다.</param>
        /// <param name="text">기록할 본문이다.</param>
        public uint Record(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            Calls.TryGetValue(kind, out uint count);
            Calls[kind] = count + 1;
            return count + 1;
        }

        /// <summary>지정된 한 callback의 원시 효과만 적용한다. 다음 branch·manager 조회는 제품 몸체가 결정한다.</summary>
        /// <param name="boundary">적용할 경계다.</param>
        public void Mutate(string boundary)
        {
            if (Mutation == "assert-manager22" && boundary == "assert")
            {
                Manager = 22;
            }
            else if (Mutation == boundary + "-manager22")
            {
                Manager = 22;
            }
            else if (Mutation == "temp1-version171" && boundary == "temp1")
            {
                Version = 0x123456abU;
            }
            else if (Mutation == "territory1-version127" && boundary == "territory1")
            {
                Version = 0xff00007fU;
            }
            else if (Mutation == "territory20-fort1" && boundary == "territory20")
            {
                Fort = 1;
            }
            else if (Mutation == "rename1-fort0" && boundary == "rename1")
            {
                Fort = 0;
            }
            else if (Mutation == "chaff-version255" && boundary == "chaff")
            {
                Version = 0x765432ffU;
            }
            else if (Mutation == "capacity-current" && boundary == "capacity")
            {
                ObjectsPut(Nodes[Active - 1], 4, 5);
                ObjectsPut(Nodes[Active - 1], 8, 2);
                ObjectsPut(Nodes[Active - 1], 12, 64);
                Version = 0x987654eeU;
            }
            else if (Mutation == boundary + "-stop")
            {
                throw new ObjectsStop();
            }
        }

        /// <summary>영역·Chaff 하위가 명시한 1바이트만 현재 stream에 쓰며 부모 version 쓰기는 대신하지 않는다.</summary>
        /// <param name="token">쓸 stream 번호다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Append(uint token, byte value)
        {
            byte[] node = Nodes[token - 1];
            uint pointer = ObjectsGet(node, 0);
            uint position = ObjectsGet(node, 4);
            uint target = (pointer - ObjectsBufferBase) / 64;
            if (pointer < ObjectsBufferBase + 64 || target > 21 || position >= 64)
            {
                throw new ArgumentOutOfRangeException(nameof(token), token, "Objects 명시 하위 출력 범위");
            }
            Output[target - 1][position] = value;
            ObjectsPut(node, 4, position + 1);
            if (unchecked((int)ObjectsGet(node, 8)) < unchecked((int)(position + 1)))
            {
                ObjectsPut(node, 8, position + 1);
            }
        }

        /// <summary>실제 부모와 동일한 입력·하위 응답만 구성하며 DataManager 노드 알고리즘은 복제하지 않는다.</summary>
        public FortObjectsSaveHooks Hooks()
        {
            return new FortObjectsSaveHooks
            {
                Manager = selected =>
                {
                    Assert.Equal(Slot, selected);
                    return Manager;
                },
                Assertion = value =>
                {
                    uint line = value == FortObjectsSaveAssertion.MissingTemplate ? 957U : value == FortObjectsSaveAssertion.NotMine ? 958U : 36U;
                    Record("assert", "assert:" + line);
                    Mutate(line == 36 ? "capacity" : "assert");
                },
                Find = (selected, name) =>
                {
                    int current = int.Parse(name.Substring(4), CultureInfo.InvariantCulture);
                    bool found = Pattern == 1 || (Pattern == 2 && current % 2 == 0);
                    uint index = Record("find", "find:" + selected + ':' + name);
                    Currents[selected] = found ? (uint)(current + 1) : 0U;
                    Mutate("find" + index);
                    return found ? 1U : 0U;
                },
                Create = (selected, name) =>
                {
                    uint index = Record("create", "create:" + selected + ':' + name);
                    uint token;
                    if (name == "Temp")
                    {
                        Region++;
                        token = (uint)(Region + 1);
                        Temps++;
                    }
                    else if (name == "Chaff")
                    {
                        token = 21U;
                    }
                    else
                    {
                        token = (uint)(int.Parse(name.Substring(4), CultureInfo.InvariantCulture) + 1);
                    }
                    Currents[selected] = token;
                    if (name == "Temp" || name == "Chaff")
                    {
                        ObjectsPut(Nodes[token - 1], 0, ObjectsBufferBase + token * 64);
                        ObjectsPut(Nodes[token - 1], 4, Position);
                        ObjectsPut(Nodes[token - 1], 8, End);
                        ObjectsPut(Nodes[token - 1], 12, Capacity);
                    }
                    Mutate("create" + index);
                    if (name == "Temp")
                    {
                        Mutate("temp" + Temps);
                    }
                    else if (name == "Chaff")
                    {
                        Mutate("chaff");
                    }
                    return token;
                },
                Select = (selected, name, writable) =>
                {
                    Assert.Equal("Temp", name);
                    Assert.Equal(1, writable);
                    uint token = (uint)(Region + 1);
                    uint index = Record("select", "select:" + selected + ":Temp:1");
                    Currents[selected] = token;
                    if (Pattern == 2)
                    {
                        if (Region % 3 == 0)
                        {
                            ObjectsPut(Nodes[token - 1], 0, 0);
                        }
                        else if (Region % 3 == 1)
                        {
                            ObjectsPut(Nodes[token - 1], 8, 0);
                        }
                    }
                    Mutate("select" + index);
                    return token;
                },
                Rename = (selected, destination, source) =>
                {
                    Assert.Equal("Temp", source);
                    uint index = Record("rename", "rename:" + selected + ':' + destination + ':' + source);
                    Mutate("rename" + index);
                },
                Territory = (stream, owner, current) =>
                {
                    uint index = Record("territory", "territory:" + stream + ':' + owner + ':' + current);
                    Append(stream, (byte)(0x40 + current));
                    Mutate("territory" + index);
                },
                Chaff = stream =>
                {
                    Record("chaffbody", "chaff:" + stream);
                    Append(stream, 0x7f);
                },
                Access = (token, operation) =>
                {
                    Active = token;
                    MemStreamHooks memory = new()
                    {
                        Buffer = pointer =>
                        {
                            if (pointer < ObjectsBufferBase + 64 || (pointer - ObjectsBufferBase) % 64 != 0)
                            {
                                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Objects 물리 버퍼 토큰");
                            }
                            return Output[(pointer - ObjectsBufferBase) / 64 - 1];
                        },
                    };
                    operation(Nodes[token - 1], memory);
                },
            };
        }

        /// <summary>부모 실행 뒤의 현재 전역과 명시 manager 선택을 원문 관측 형식으로 읽는다.</summary>
        public string State()
        {
            return Manager + "," + Mine + "," + Version + "," + Fort + "," + Currents[0] + "," + Currents[11] + "," + Currents[22];
        }

        /// <summary>모든 21 stream과 64바이트 전체 출력을 읽어 부분 쓰기·주변 보존까지 비교한다.</summary>
        public string Raw()
        {
            List<string> parts = [];
            // 생성되지 않은 노드도 원래 입력 보존을 확인한다
            for (int index = 0; index < Nodes.Length; index++)
            {
                parts.Add(ObjectsHex(Nodes[index]) + ObjectsHex(Output[index]));
            }
            return string.Join(':', parts);
        }
    }

    /// <summary>최초 357고유입력의 두 정밀도 714관측을 정상 반환·명시 중단의 모든 출력과 대조한다.</summary>
    [Fact]
    public void IndependentNative714()
    {
        string[][] rows = X86Fixture.Read("fort-objects-parent");
        int compared = 0;
        int stopped = 0;
        HashSet<string> unique = [];
        // 기대값은 제품 본문을 실행한 뒤에만 사용한다
        foreach (string[] row in rows)
        {
            try
            {
                Assert.Equal(6, row.Length);
                Assert.True(unique.Add(row[0] + "\u0000" + row[1]));
                ObjectsDevices devices = new(row[1]);
                bool completed = true;
                try
                {
                    RawFortObjectsSave.SaveFortObjects(devices.Slot, devices.Globals(), devices.Hooks());
                }
                catch (ObjectsStop)
                {
                    completed = false;
                    stopped++;
                }
                Assert.Equal(completed, row[2] == "1");
                Assert.Equal(row[3], devices.State());
                Assert.Equal(row[4], devices.Raw());
                Assert.Equal(row[5], devices.Trace);
                compared++;
            }
            catch (Exception error) when (error is not ObjectsStop)
            {
                throw new InvalidOperationException("행 " + compared + " 입력 " + row[0], error);
            }
        }
        Assert.Equal(714, compared);
        Assert.Equal(48, stopped);
    }

    /// <summary>도달하지 않는 Chaff 경계는 요구하지 않고 실제 find 누락은 선행 version 쓰기 뒤 실패한다.</summary>
    [Fact]
    public void ExplicitUnselectedAndMissingBoundary()
    {
        ObjectsDevices devices = new("1,11,1,0,0,0,0,64,none");
        FortObjectsSaveHooks hooks = devices.Hooks();
        hooks.Chaff = null;
        RawFortObjectsSave.SaveFortObjects(1, devices.Globals(), hooks);
        Assert.Equal(20u, devices.Temps);
        Assert.Equal(0u, devices.Calls.TryGetValue("chaffbody", out uint chaffbody) ? chaffbody : 0u);
        ObjectsDevices missing = new("1,11,1,0,0,0,0,64,none");
        FortObjectsSaveHooks absent = missing.Hooks();
        absent.Find = null;
        bool rejected = false;
        try
        {
            RawFortObjectsSave.SaveFortObjects(1, missing.Globals(), absent);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(2u, missing.Version);
        Assert.Equal("", missing.Trace);
    }
}
