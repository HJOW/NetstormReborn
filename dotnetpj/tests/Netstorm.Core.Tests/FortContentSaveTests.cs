using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 내용물 저장(<see cref="RawFortContentSave"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortContentSaveTests</c>의 전수 706행과 빈 목록·장치 누락·순환 직접 1종을 옮겼다. (F2c-5, 2026-11-03)
/// </summary>
public sealed class FortContentSaveTests
{
    /// <summary>독립 TSV의 마지막 빈 칸까지 그대로 분리한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static List<string> ContentFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 끝 경계까지 원문 필드로 둔다
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

    /// <summary>물리 필드를 지정 폭으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 너비다.</param>
    private static uint ContentRead(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        uint value = 0;
        // 지정 폭만 조합한다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>물리 필드에 지정 폭만 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비다.</param>
    private static void ContentPut(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 인접 필드를 보존한다
        for (int index = 0; index < width; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>독립 관측과 같은 소문자 hex를 만든다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string ContentHex(ReadOnlySpan<byte> bytes)
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
    internal sealed class ContentStop : Exception;

    /// <summary>한 Begin 호출의 독립 검색 위치다.</summary>
    private sealed class ContentCursor
    {
        /// <summary>고정 부모다.</summary>
        public uint Parent;

        /// <summary>현재 후보다. callback으로 바뀔 수 있다.</summary>
        public uint Current;

        /// <summary>다음 위치다.</summary>
        public int Index;

        /// <summary>외부 후보열이다.</summary>
        public List<uint> Sequence = [];
    }

    /// <summary>독립 PE 입력을 같은 위치에 두고 실제 저장소·쓰기 stream을 연결한다.</summary>
    private sealed class ContentDevices
    {
        /// <summary>진입 부모다.</summary>
        public uint Parent;

        /// <summary>현재 Pool 물리 개수다.</summary>
        public uint Limit;

        /// <summary>원본 stream 필드다.</summary>
        public byte[] Stream = new byte[16];

        /// <summary>실제 stream과 data 변경을 구별하는 두 물리 버퍼다.</summary>
        public byte[][] Buffers = [new byte[16384], new byte[16384]];

        /// <summary>원시 Form 필드(16개·50바이트)다.</summary>
        public byte[][] Forms = [new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50], new byte[50]];

        /// <summary>원시 Form 가상60 반환 DWORD다. bool로 미리 줄이지 않는다.</summary>
        public Dictionary<uint, uint> Robust = [];

        /// <summary>외부 타입이다.</summary>
        public Dictionary<uint, FortContentSaveType> Types = [];

        /// <summary>외부 후보열이다.</summary>
        public Dictionary<uint, List<uint>> Children = [];

        /// <summary>입력의 출력 순서다.</summary>
        public List<uint> FormOrder = [];

        /// <summary>입력의 출력 순서다.</summary>
        public List<uint> TypeOrder = [];

        /// <summary>명시 callback 변경이다.</summary>
        public List<string[]> Mutations = [];

        /// <summary>현재 경계 횟수다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>활성 검색기다.</summary>
        public ContentCursor? Current;

        /// <summary>실제 외부 호출 trace다.</summary>
        public string Trace = "";

        /// <summary>외부 경계를 기록하고 명시 입력 변경만 순서대로 적용한다.</summary>
        /// <param name="name">경계 이름이다.</param>
        /// <param name="text">기록할 본문이다.</param>
        public void Boundary(string name, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            Calls.TryGetValue(name, out int count);
            Calls[name] = count + 1;
            string currentEvent = name + (count + 1);
            // 같은 경계의 여러 입력 효과는 작성 순서대로 적용한다
            foreach (string[] mutation in Mutations)
            {
                if (mutation[0] != currentEvent)
                {
                    continue;
                }
                string target = mutation[1];
                string field = mutation[2];
                uint value = uint.Parse(mutation[3], CultureInfo.InvariantCulture);
                if (target.StartsWith("form", StringComparison.Ordinal))
                {
                    uint sid = uint.Parse(target[4..], CultureInfo.InvariantCulture);
                    if (field == "robust")
                    {
                        Robust[sid] = value;
                    }
                    else
                    {
                        ContentPut(Forms[sid], field == "kind" ? 18 : field == "quantity" ? 12 : 20, value, 2);
                    }
                }
                else if (target.StartsWith("type", StringComparison.Ordinal))
                {
                    FortContentSaveType type = Types[uint.Parse(target[4..], CultureInfo.InvariantCulture)];
                    if (field == "flags1")
                    {
                        type.Flags1 = value;
                    }
                    else if (field == "content")
                    {
                        type.ContentListFlags = value;
                    }
                    else
                    {
                        type.ContainerListFlags = value;
                    }
                    Types[uint.Parse(target[4..], CultureInfo.InvariantCulture)] = type;
                }
                else if (target == "stream")
                {
                    ContentPut(Stream, field == "data" ? 0 : field == "position" ? 4 : field == "end" ? 8 : 12, value);
                }
                else if (target == "global")
                {
                    Assert.Equal("poollimit", field);
                    Limit = value;
                }
                else if (target == "finder")
                {
                    Assert.Equal("current", field);
                    Current!.Current = value;
                }
                else
                {
                    Assert.Equal("control", target);
                    Assert.Equal("stop", field);
                    Assert.Equal(1u, value);
                    throw new ContentStop();
                }
            }
        }

        /// <summary>원문이 호출하는 외부 장치만 연결한다. 재귀·형식·쓰기는 제품 구현이 실행한다.</summary>
        public FortContentSaveHooks Hooks()
        {
            return new FortContentSaveHooks
            {
                Begin = selected =>
                {
                    ContentCursor cursor = new()
                    {
                        Parent = selected,
                        Sequence = Children.TryGetValue(selected, out List<uint>? sequence) ? [.. sequence] : [],
                    };
                    cursor.Current = cursor.Sequence.Count == 0 ? 0 : cursor.Sequence[0];
                    Current = cursor;
                    Boundary("begin", "begin:" + selected);
                    return new FortContentSaveCursor
                    {
                        Current = () =>
                        {
                            Current = cursor;
                            return cursor.Current;
                        },
                        Next = () =>
                        {
                            Current = cursor;
                            uint before = cursor.Current;
                            cursor.Index++;
                            cursor.Current = cursor.Index < cursor.Sequence.Count ? cursor.Sequence[cursor.Index] : 0;
                            Boundary("next", "next:" + cursor.Parent + ':' + before);
                        },
                    };
                },
                PoolLimit = () => Limit,
                Form = sid => Forms[sid],
                Robust = sid =>
                {
                    bool answer = Robust.TryGetValue(sid, out uint raw) && raw != 0;
                    Boundary("robust", "robust:" + sid);
                    return answer;
                },
                Type = kind =>
                {
                    Boundary("type", "type:" + kind);
                    return Types[kind];
                },
                ParentType = selected =>
                {
                    Boundary("parent", "parent:" + selected);
                    // 독립 ParentType 장치는 지정 raw kind를 경계 변경 뒤 읽는다
                    return Types[ContentRead(Forms[selected], 18, 2)];
                },
                Assertion = kind => Boundary("assert", "assert:" + (kind == FortContentSaveAssertion.WriteCapacity ? 36 : 278)),
                Access = (token, operation) =>
                {
                    Assert.Equal(1u, token);
                    MemStreamHooks memory = new()
                    {
                        Buffer = data => Buffers[data],
                    };
                    operation(Stream, memory);
                },
            };
        }

        /// <summary>TSV 앞10열의 장치 입력을 그대로 배치한다.</summary>
        /// <param name="row">입력 행이다.</param>
        public void Initialize(string[] row)
        {
            Parent = uint.Parse(row[1], CultureInfo.InvariantCulture);
            Limit = uint.Parse(row[2], CultureInfo.InvariantCulture);
            ContentPut(Stream, 4, uint.Parse(row[3], CultureInfo.InvariantCulture));
            ContentPut(Stream, 8, uint.Parse(row[4], CultureInfo.InvariantCulture));
            ContentPut(Stream, 12, uint.Parse(row[5], CultureInfo.InvariantCulture));
            foreach (byte[] buffer in Buffers)
            {
                Array.Fill(buffer, (byte)0xcc);
            }
            // 원본 관측과 같은 초기 물리 자료다
            if (row[6] != "-")
            {
                foreach (string record in row[6].Split(';'))
                {
                    string[] fields = record.Split(':');
                    uint sid = uint.Parse(fields[0], CultureInfo.InvariantCulture);
                    FormOrder.Add(sid);
                    ContentPut(Forms[sid], 18, uint.Parse(fields[1], CultureInfo.InvariantCulture), 2);
                    ContentPut(Forms[sid], 12, uint.Parse(fields[2], CultureInfo.InvariantCulture), 2);
                    ContentPut(Forms[sid], 20, uint.Parse(fields[3], CultureInfo.InvariantCulture), 2);
                    Robust[sid] = uint.Parse(fields[4], CultureInfo.InvariantCulture);
                }
            }
            if (row[7] != "-")
            {
                foreach (string record in row[7].Split(';'))
                {
                    string[] fields = record.Split(':');
                    uint kind = uint.Parse(fields[0], CultureInfo.InvariantCulture);
                    TypeOrder.Add(kind);
                    Types[kind] = new FortContentSaveType(uint.Parse(fields[1], CultureInfo.InvariantCulture), uint.Parse(fields[2], CultureInfo.InvariantCulture), uint.Parse(fields[3], CultureInfo.InvariantCulture));
                }
            }
            if (row[8] != "-")
            {
                foreach (string record in row[8].Split(';'))
                {
                    string[] fields = record.Split(':');
                    List<uint> sequence = Children[uint.Parse(fields[0], CultureInfo.InvariantCulture)] = [];
                    if (fields[1] != "-")
                    {
                        foreach (string child in fields[1].Split(','))
                        {
                            sequence.Add(uint.Parse(child, CultureInfo.InvariantCulture));
                        }
                    }
                }
            }
            if (row[9] != "-")
            {
                foreach (string record in row[9].Split(';'))
                {
                    Mutations.Add(record.Split('/'));
                }
            }
        }

        /// <summary>현재 Form 필드와 외부 Robust 값을 입력 순서대로 출력한다.</summary>
        public string FormOutput()
        {
            StringBuilder text = new();
            // 바뀌지 않은 인접 필드도 함께 대조한다
            foreach (uint sid in FormOrder)
            {
                if (text.Length != 0)
                {
                    text.Append(';');
                }
                text.Append(sid).Append(':').Append(ContentRead(Forms[sid], 18, 2)).Append(':').Append(ContentRead(Forms[sid], 12, 2)).Append(':').Append(ContentRead(Forms[sid], 20, 2)).Append(':').Append(Robust.TryGetValue(sid, out uint value) ? value : 0);
            }
            return text.ToString();
        }

        /// <summary>현재 타입 세 DWORD를 입력 순서대로 출력한다.</summary>
        public string TypeOutput()
        {
            StringBuilder text = new();
            // callback 변경 결과를 포함한다
            foreach (uint kind in TypeOrder)
            {
                FortContentSaveType type = Types[kind];
                if (text.Length != 0)
                {
                    text.Append(';');
                }
                text.Append(kind).Append(':').Append(type.Flags1).Append(':').Append(type.ContentListFlags).Append(':').Append(type.ContainerListFlags);
            }
            return text.ToString();
        }
    }

    /// <summary>실패 위치를 상시 기기로 옮기는지 확인한다.</summary>
    /// <param name="function">실행할 동작이다.</param>
    private static bool ContentRejects(Action function)
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

    /// <summary>실제 원문이 만든 전체 출력·경계 순서·부분 쓰기를 제품 결과와 비교한다.</summary>
    [Fact]
    public void IndependentNative()
    {
        string[][] rows = X86Fixture.Read("fort-content-save");
        int compared = 0;
        // 앞10열만 입력이며 뒤12열은 독립 PE 결과다
        foreach (string[] row in rows)
        {
            try
            {
                Assert.Equal(22, row.Length);
                ContentDevices devices = new();
                devices.Initialize(row);
                bool completed = true;
                try
                {
                    RawFortContentSave.SaveFortContents(devices.Parent, 1, devices.Hooks());
                }
                catch (ContentStop)
                {
                    completed = false;
                }
                Assert.Equal(completed, row[10] == "1");
                Assert.Equal(uint.Parse(row[11], CultureInfo.InvariantCulture), ContentRead(devices.Stream, 4));
                Assert.Equal(uint.Parse(row[12], CultureInfo.InvariantCulture), ContentRead(devices.Stream, 8));
                Assert.Equal(uint.Parse(row[13], CultureInfo.InvariantCulture), ContentRead(devices.Stream, 0));
                Assert.Equal(row[14], ContentHex(devices.Buffers[0].AsSpan(0, (int)ContentRead(devices.Stream, 8))));
                Assert.Equal(row[15], ContentHex(devices.Buffers[1].AsSpan(0, (int)ContentRead(devices.Stream, 8))));
                Assert.Equal(row[16], devices.Trace);
                if (completed)
                {
                    Assert.Equal(0, int.Parse(row[17], CultureInfo.InvariantCulture));
                }
                else
                {
                    Assert.True(int.Parse(row[17], CultureInfo.InvariantCulture) < 0);
                }
                Assert.Equal(uint.Parse(row[18], CultureInfo.InvariantCulture), devices.Limit);
                Assert.Equal(row[19], devices.FormOutput());
                Assert.Equal(row[20], devices.TypeOutput());
                Assert.True(uint.Parse(row[21], CultureInfo.InvariantCulture) > 0);
                compared++;
            }
            catch (Exception error) when (error is not ContentStop)
            {
                throw new InvalidOperationException("행 " + compared + " 입력 " + row[0], error);
            }
        }
        Assert.Equal(706, compared);
    }

    /// <summary>빈 목록은 헤더만 쓰며 선택하지 않은 외부 장치를 요구하지 않는다. 순환은 성공으로 처리하지 않는다.</summary>
    [Fact]
    public void EmptyMissingDevicesAndCycle()
    {
        ContentDevices devices = new();
        devices.Limit = 8;
        ContentPut(devices.Stream, 12, 0);
        FortContentSaveHooks hooks = devices.Hooks();
        hooks.Robust = null;
        hooks.Type = null;
        hooks.Form = null;
        hooks.ParentType = null;
        hooks.Assertion = null;
        hooks.PoolLimit = null;
        RawFortContentSave.SaveFortContents(1, 1, hooks);
        Assert.Equal(1u, ContentRead(devices.Stream, 4));
        Assert.Equal(1u, ContentRead(devices.Stream, 8));
        Assert.Equal(0, devices.Buffers[0][0]);
        Assert.Equal("begin:1", devices.Trace);
        ContentDevices cycle = new();
        cycle.Limit = 8;
        ContentPut(cycle.Stream, 12, 4096);
        cycle.Children[1] = [2];
        cycle.Children[2] = [1];
        cycle.Robust[1] = 1;
        cycle.Robust[2] = 1;
        ContentPut(cycle.Forms[1], 18, 71, 2);
        ContentPut(cycle.Forms[2], 18, 71, 2);
        cycle.Types[71] = new FortContentSaveType(0x20000, 0, 0);
        Assert.True(ContentRejects(() => RawFortContentSave.SaveFortContents(1, 1, cycle.Hooks())));
        Assert.True(ContentRead(cycle.Stream, 4) > 1);
        ContentDevices missing = new();
        missing.Limit = 8;
        missing.Children[1] = [2];
        FortContentSaveHooks absent = missing.Hooks();
        absent.Robust = null;
        Assert.True(ContentRejects(() => RawFortContentSave.SaveFortContents(1, 1, absent)));
        Assert.Equal(0, missing.Buffers[0][0]);
        Assert.Equal(1u, ContentRead(missing.Stream, 4));
    }
}
