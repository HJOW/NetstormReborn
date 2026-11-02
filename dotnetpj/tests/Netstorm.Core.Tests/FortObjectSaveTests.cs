using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 단일 객체·청크 저장(<see cref="RawFortObjectSave"/>)을 독립 원본 입력과 대조한다.
/// cpppj <c>FortObjectSaveTests</c>의 객체 전수 3294행과 청크 전수 672행을 옮겼다. (F2c-5, 2026-11-03)
/// </summary>
public sealed class FortObjectSaveTests
{
    /// <summary>원본 관측과 같은 두 비영 물리 주소 토큰이다.</summary>
    private static readonly uint[] ObjectBufferPointers = [0x01001000U, 0x01006000U];

    /// <summary>객체와 청크가 공유하는 고정 stream 토큰이다.</summary>
    private const uint ObjectStreamPointer = 0x01000100U;

    /// <summary>독립 관측의 실제 물리 출력 상한이다.</summary>
    private const int ObjectBufferSize = 16384;

    /// <summary>독립 TSV의 마지막 빈 칸까지 그대로 분리한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static List<string> ObjectFields(string source, char separator)
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

    /// <summary>signed 입력과 전체 DWORD 입력을 같은 원시 비트로 읽는다.</summary>
    /// <param name="text">읽을 십진수다.</param>
    private static uint ObjectNumber(string text)
    {
        return unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    }

    /// <summary>DWORD 비트를 바꾸지 않고 원본 signed 비교에 쓴다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    private static int ObjectSigned(uint value)
    {
        return unchecked((int)value);
    }

    /// <summary>비정렬 필드를 little endian 지정 폭으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 너비다.</param>
    private static uint ObjectRead(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        uint value = 0;
        // 지정 BYTE만 조합하고 이웃 자료는 읽지 않는다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>지정 raw 필드의 폭만 덮어써 인접 필드를 보존한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비다.</param>
    private static void ObjectPut(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 지정 폭만 little endian으로 기록한다
        for (int index = 0; index < width; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>관측 payload와 raw 슬롯을 소문자 hex로 표현한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string ObjectHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder text = new();
        // 전체 관측 범위의 각 BYTE를 빠짐없이 출력한다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>독립 입력에 적힌 hex BYTE를 변경 없이 복원한다.</summary>
    /// <param name="text">읽을 hex다.</param>
    private static byte[] ObjectBytes(string text)
    {
        if (text.Length % 2 != 0)
        {
            throw new ArgumentException("관측 hex BYTE 폭", nameof(text));
        }
        byte[] bytes = new byte[text.Length / 2];
        // 명시 두 자리마다 BYTE 하나만 생성한다
        for (int at = 0; at < text.Length; at += 2)
        {
            bytes[at / 2] = Convert.ToByte(text.Substring(at, 2), 16);
        }
        return bytes;
    }

    /// <summary>현재 DWORD 목록을 독립 출력과 같은 순서의 십진수 문자열로 만든다.</summary>
    /// <param name="values">출력할 값이다.</param>
    private static string ObjectWords(uint[] values)
    {
        // 원시 DWORD를 signed 축소 없이 전부 비교한다
        return string.Join(':', values);
    }

    /// <summary>명시 경계에서 원문 실행을 중단한 입력을 C#에서도 같은 시점에 멈춘다.</summary>
    internal sealed class ObjectStop : Exception;

    /// <summary>기대 출력은 보관하지 않고 입력의 외부 장치·현재 자료·변경 효과만 소유한다.</summary>
    private sealed class ObjectDevices
    {
        /// <summary>고정 SID의 전체 raw50이며 첫 vptr DWORD는 격리 주소를 넣지 않은 0이다.</summary>
        public byte[] Raw = new byte[50];

        /// <summary>각 분기에서 다시 조회할 실제 전역 DWORD다.</summary>
        public uint[] Globals = new uint[10];

        /// <summary>flags·가상·섬 함수의 명시 현재 반환값이다.</summary>
        public uint[] Devices = new uint[6];

        /// <summary>data·pos·end·capacity를 보관하는 실제 16바이트 헤더다.</summary>
        public byte[] Stream = new byte[16];

        /// <summary>data 변경과 고정 count 주소를 구별하는 두 물리 출력이다.</summary>
        public byte[][] Buffers = [new byte[ObjectBufferSize], new byte[ObjectBufferSize]];

        /// <summary>입력에 적힌 경계별 순서 효과이며 기대 결과를 사용하지 않는다.</summary>
        public List<string[]> Effects = [];

        /// <summary>flags11과 flags12처럼 종류별 호출 번호를 만든다.</summary>
        public Dictionary<string, int> Calls = [];

        /// <summary>실제로 호출된 외부 경계만 순서대로 보관한다.</summary>
        public string Trace = "";

        /// <summary>고정 객체 SID다.</summary>
        public uint Sid;

        /// <summary>현재 Pool 개수다.</summary>
        public uint PoolLimit;

        /// <summary>독립 Finder 현재 후보다.</summary>
        public uint Current;

        /// <summary>명시 Begin·Next 장치가 공급하는 후보열이다.</summary>
        public List<uint> Sequence = [];

        /// <summary>current 효과와 독립적으로 Next가 진행하는 후보열 위치다.</summary>
        public int SequenceIndex;

        /// <summary>SaveObject 외부 장치의 명시 DWORD 반환표다.</summary>
        public Dictionary<uint, uint> Returns = [];

        /// <summary>명시한 비영 반환 때만 쓰는 하위 장치 payload다.</summary>
        public Dictionary<uint, byte[]> ChildPayloads = [];

        /// <summary>입력·원시 출력의 전역 DWORD 순서다.</summary>
        private static readonly string[] GlobalNames = ["battle", "mission", "editor", "scope", "version", "localOwner", "bridgeType", "daisType", "battleMask", "ownerMask"];

        /// <summary>명시 getter·가상 함수의 현재 반환 장치 순서다.</summary>
        private static readonly string[] DeviceNames = ["flags1", "flags2", "robust", "island", "hasLevel", "level"];

        /// <summary>명명한 입력 필드만 찾아 알 수 없는 효과가 조용히 다른 필드를 바꾸지 않게 한다.</summary>
        /// <param name="names">찾을 이름표다.</param>
        /// <param name="name">찾을 이름이다.</param>
        private static int Field(string[] names, string name)
        {
            int found = Array.IndexOf(names, name);
            if (found < 0)
            {
                throw new ArgumentException("미명시 객체 관측 필드", nameof(name));
            }
            return found;
        }

        /// <summary>비영 물리 토큰을 두 버퍼 중 하나로 해석하며 임의 대체 버퍼는 만들지 않는다.</summary>
        /// <param name="pointer">찾을 토큰이다.</param>
        public Span<byte> Buffer(uint pointer)
        {
            int found = Array.IndexOf(ObjectBufferPointers, pointer);
            if (found < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "관측 물리 버퍼 토큰");
            }
            return Buffers[found].AsSpan();
        }

        /// <summary>stream과 두 0xCC 초기 버퍼, 입력에 적힌 효과만 준비한다.</summary>
        /// <param name="row">입력 행이다.</param>
        /// <param name="at">stream 입력 시작 칸이다.</param>
        public void InitializeStream(string[] row, int at)
        {
            ObjectPut(Stream, 0, ObjectBufferPointers[0]);
            ObjectPut(Stream, 4, ObjectNumber(row[at]));
            ObjectPut(Stream, 8, ObjectNumber(row[at + 1]));
            ObjectPut(Stream, 12, ObjectNumber(row[at + 2]));
            // 원문 출력 밖의 초기 BYTE도 구별할 수 있게 놓는다
            foreach (byte[] buffer in Buffers)
            {
                Array.Fill(buffer, (byte)0xcc);
            }
            // 앞쪽 입력 효과만 순서대로 읽고 결과 열은 참조하지 않는다
            if (row[at + 3] != "-")
            {
                foreach (string effect in row[at + 3].Split(';'))
                {
                    string[] fields = effect.Split('/');
                    if (fields.Length != 4)
                    {
                        throw new ArgumentException("관측 효과 열 수", nameof(row));
                    }
                    Effects.Add(fields);
                }
            }
        }

        /// <summary>객체 TSV의 입력 25열만 raw·전역·현재 장치에 설치한다.</summary>
        /// <param name="row">입력 행이다.</param>
        public void InitializeObject(string[] row)
        {
            Sid = ObjectNumber(row[1]);
            byte[] bytes = ObjectBytes(row[4]);
            if (bytes.Length != Raw.Length)
            {
                throw new ArgumentException("관측 객체 raw50 폭", nameof(row));
            }
            Array.Copy(bytes, Raw, Raw.Length);
            Devices[0] = ObjectNumber(row[5]);
            Devices[1] = ObjectNumber(row[6]);
            // 원시 전역 입력의 전체 DWORD 폭을 유지한다
            for (int index = 0; index < Globals.Length; index++)
            {
                Globals[index] = ObjectNumber(row[7 + index]);
            }
            // robust·island·hasLevel·level 네 입력을 같은 장치 배열에 놓는다
            for (int index = 2; index < Devices.Length; index++)
            {
                Devices[index] = ObjectNumber(row[15 + index]);
            }
            InitializeStream(row, 21);
        }

        /// <summary>청크 TSV의 입력 10열만 Finder·SaveObject 반환·stream에 설치한다.</summary>
        /// <param name="row">입력 행이다.</param>
        public void InitializeChunk(string[] row)
        {
            PoolLimit = ObjectNumber(row[3]);
            // 후보 개수에는 실제 65536 반환 1 감김 입력도 포함한다
            if (row[4] != "-")
            {
                foreach (string value in row[4].Split(','))
                {
                    Sequence.Add(ObjectNumber(value));
                }
            }
            // 반환표에 적힌 하위 장치만 공급하고 나머지는 기본 반환 1이다
            if (row[5] != "-")
            {
                foreach (string text in row[5].Split(';'))
                {
                    string[] fields = text.Split(':');
                    if (fields.Length < 2 || fields.Length > 3)
                    {
                        throw new ArgumentException("관측 SaveObject 반환 열 수", nameof(row));
                    }
                    uint candidate = ObjectNumber(fields[0]);
                    Returns[candidate] = ObjectNumber(fields[1]);
                    if (fields.Length == 3)
                    {
                        ChildPayloads[candidate] = fields[2] == "-" ? [] : ObjectBytes(fields[2]);
                    }
                }
            }
            InitializeStream(row, 6);
        }

        /// <summary>실제 경계의 trace와 종류별 호출 번호를 기록하되 효과 시점은 호출자가 정한다.</summary>
        /// <param name="kind">경계 종류다.</param>
        /// <param name="text">기록할 본문이다.</param>
        public string Event(string kind, string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
            Calls.TryGetValue(kind, out int count);
            Calls[kind] = count + 1;
            return kind + (count + 1);
        }

        /// <summary>같은 경계에 적힌 현재 자료 변경을 입력 순서대로 수행한다.</summary>
        /// <param name="currentEvent">적용할 경계다.</param>
        public void Mutate(string currentEvent)
        {
            // 일치한 경계의 입력 효과만 적용하며 저장 분기 자체는 구현하지 않는다
            foreach (string[] effect in Effects)
            {
                if (effect[0] != currentEvent)
                {
                    continue;
                }
                string target = effect[1];
                string field = effect[2];
                uint value = ObjectNumber(effect[3]);
                if (target == "global")
                {
                    if (field == "poollimit")
                    {
                        PoolLimit = value;
                    }
                    else
                    {
                        Globals[Field(GlobalNames, field)] = value;
                    }
                }
                else if (target == "device")
                {
                    Devices[Field(DeviceNames, field)] = value;
                }
                else if (target == "form")
                {
                    if (field == "type")
                    {
                        ObjectPut(Raw, 10, value, 1);
                    }
                    else if (field == "owner")
                    {
                        ObjectPut(Raw, 34, value, 1);
                    }
                    else if (field == "flags")
                    {
                        ObjectPut(Raw, 40, value, 1);
                    }
                    else if (field == "qa")
                    {
                        ObjectPut(Raw, 12, value, 2);
                    }
                    else if (field == "frame")
                    {
                        ObjectPut(Raw, 36, value);
                    }
                    else if (field == "x")
                    {
                        ObjectPut(Raw, 14, value);
                    }
                    else if (field == "y")
                    {
                        ObjectPut(Raw, 18, value);
                    }
                    else
                    {
                        throw new ArgumentException("미명시 raw 효과", nameof(currentEvent));
                    }
                }
                else if (target == "stream")
                {
                    if (field == "data")
                    {
                        ObjectPut(Stream, 0, ObjectBufferPointers[value]);
                    }
                    else if (field == "position")
                    {
                        ObjectPut(Stream, 4, value);
                    }
                    else if (field == "end")
                    {
                        ObjectPut(Stream, 8, value);
                    }
                    else if (field == "capacity")
                    {
                        ObjectPut(Stream, 12, value);
                    }
                    else
                    {
                        throw new ArgumentException("미명시 stream 효과", nameof(currentEvent));
                    }
                }
                else if (target == "finder" && field == "current")
                {
                    Current = value;
                }
                else if (target == "control" && field == "stop" && value == 1)
                {
                    throw new ObjectStop();
                }
                else
                {
                    throw new ArgumentException("미명시 관측 효과 대상", nameof(currentEvent));
                }
            }
        }

        /// <summary>원문 진단에서 명시한 복귀·변경·중단만 공급한다.</summary>
        /// <param name="line">진단 행이다.</param>
        public void Assertion(uint line)
        {
            Mutate(Event("assert", "assert:" + line));
        }

        /// <summary>외부 장치의 명시 bytes만 현재 data·pos에 쓰고 unsigned 논리끝을 진행한다.</summary>
        /// <param name="bytes">쓸 바이트다.</param>
        public void Append(ReadOnlySpan<byte> bytes)
        {
            uint position = ObjectRead(Stream, 4);
            Span<byte> output = Buffer(ObjectRead(Stream, 0));
            if (position > (uint)output.Length || bytes.Length > output.Length - (int)position)
            {
                throw new ArgumentOutOfRangeException(nameof(bytes), position, "관측 외부 장치 출력 범위");
            }
            bytes.CopyTo(output.Slice((int)position));
            uint next = position + (uint)bytes.Length;
            ObjectPut(Stream, 4, next);
            ObjectPut(Stream, 8, Math.Max(ObjectRead(Stream, 8), next));
        }

        /// <summary>flags getter는 trace·효과 뒤의 현재 장치 DWORD를 반환한다.</summary>
        /// <param name="selected">확인할 SID다.</param>
        /// <param name="index">장치 번호다.</param>
        public uint Flags(uint selected, int index)
        {
            Assert.Equal(Sid, selected);
            string name = DeviceNames[index];
            Mutate(Event(name, name + ':' + selected + ':' + Raw[10]));
            return Devices[index];
        }

        /// <summary>가상 함수와 hasLevel은 반환값을 효과 전에 캡처한다.</summary>
        /// <param name="selected">확인할 SID다.</param>
        /// <param name="index">장치 번호다.</param>
        public uint CapturedDevice(uint selected, int index)
        {
            Assert.Equal(Sid, selected);
            uint answer = Devices[index];
            string name = DeviceNames[index];
            Mutate(Event(name, name + ':' + selected));
            return answer;
        }

        /// <summary>저장 필터·필드 선택은 제품에 맡기고 명시한 객체 외부 함수만 연결한다.</summary>
        public FortObjectSaveHooks ObjectHooks()
        {
            return new FortObjectSaveHooks
            {
                // 각 소비 위치에서 현재 DWORD를 조회한다
                Global = key => Globals[(int)key],
                // raw 조회 자체는 효과를 발생시키지 않는다
                Form = selected =>
                {
                    Assert.Equal(Sid, selected);
                    return Raw;
                },
                // 두 getter의 호출 순서와 현재 type을 기록한다
                Flags1 = selected => Flags(selected, 0),
                Flags2 = selected => Flags(selected, 1),
                // 가상 반환을 bool이나 BYTE로 미리 줄이지 않는다
                Robust = selected => CapturedDevice(selected, 2),
                HasLevel = selected => CapturedDevice(selected, 4),
                Level = selected => CapturedDevice(selected, 5),
                // 전달된 실제 float32 비트를 그대로 기록한다
                Island = (x, y) =>
                {
                    uint answer = Devices[3];
                    Mutate(Event("island", "island:" + x + ':' + y));
                    return answer;
                },
                // 원문의 세 진단 행을 구별한다
                Assertion = kind => Assertion(kind == FortObjectSaveAssertion.XOffset ? 667U : kind == FortObjectSaveAssertion.YOffset ? 668U : 707U),
                // 실제 caller bytes는 용량 진단 및 입력 효과 전에 캡처한다
                Write = (selected, source) =>
                {
                    Assert.Equal(ObjectStreamPointer, selected);
                    byte[] bytes = source.ToArray();
                    string currentEvent = Event("write", "write:" + bytes.Length + ':' + ObjectHex(bytes));
                    if (ObjectSigned(ObjectRead(Stream, 12)) < ObjectSigned(ObjectRead(Stream, 4) + (uint)bytes.Length))
                    {
                        Assertion(36);
                    }
                    Append(bytes);
                    Mutate(currentEvent);
                },
                // 내용물 자체는 독립 관측에 명시한 EE 한 BYTE 장치다
                Contents = (selected, output) =>
                {
                    Assert.Equal(Sid, selected);
                    Assert.Equal(ObjectStreamPointer, output);
                    string currentEvent = Event("contents", "contents:" + selected);
                    Append([0xee]);
                    Mutate(currentEvent);
                },
            };
        }

        /// <summary>원본 Finder·SaveObject 외부 경계만 공급하며 marker·count 쓰기는 제품이 수행한다.</summary>
        public FortChunkSaveHooks ChunkHooks()
        {
            return new FortChunkSaveHooks
            {
                // 각 후보에서 현재 물리 개수를 다시 읽는다
                PoolLimit = () => PoolLimit,
                // 네 단정도 인자의 실제 비트와 고정 flags0을 기록한다
                Begin = area =>
                {
                    SequenceIndex = 0;
                    Current = Sequence.Count == 0 ? 0 : Sequence[0];
                    Mutate(Event("begin", "begin:" + BitConverter.SingleToUInt32Bits(area.Left) + ':' + BitConverter.SingleToUInt32Bits(area.Top) + ':' + BitConverter.SingleToUInt32Bits(area.Right) + ':' + BitConverter.SingleToUInt32Bits(area.Bottom) + ":0"));
                    return new FortChunkSaveCursor
                    {
                        // current 효과가 후보열 위치를 바꾸지 않도록 별도 조회한다
                        Current = () => Current,
                        // 원문 Next의 위치 이동 뒤 효과를 적용한다
                        Next = () =>
                        {
                            uint before = Current;
                            SequenceIndex++;
                            Current = SequenceIndex < Sequence.Count ? Sequence[SequenceIndex] : 0;
                            Mutate(Event("next", "next:" + before));
                        },
                    };
                },
                // 반환 DWORD와 payload는 명시 입력표에서만 읽는다
                Object = (output, candidate, originX, originY) =>
                {
                    Assert.Equal(ObjectStreamPointer, output);
                    uint answer = Returns.TryGetValue(candidate, out uint value) ? value : 1U;
                    string currentEvent = Event("object", "object:" + candidate + ':' + unchecked((uint)originX) + ':' + unchecked((uint)originY));
                    if (answer != 0)
                    {
                        if (ChildPayloads.TryGetValue(candidate, out byte[]? payload))
                        {
                            Append(payload);
                        }
                        else
                        {
                            Append([(byte)candidate]);
                        }
                    }
                    Mutate(currentEvent);
                    return answer;
                },
                // 순수 stream 빌림은 외부 trace나 입력 효과를 추가하지 않는다
                Access = (selected, operation) =>
                {
                    Assert.Equal(ObjectStreamPointer, selected);
                    MemStreamHooks memory = new()
                    {
                        Buffer = Buffer,
                        // marker의 실제 용량 진단만 원문 경계와 연결한다
                        Assertion = kind =>
                        {
                            Assert.Equal(MemStreamAssertion.WriteCapacity, kind);
                            Assertion(36);
                        },
                    };
                    operation(Stream, memory);
                },
            };
        }

        /// <summary>물리 주소만 독립 TSV의 data0·1 토큰으로 정규화하고 나머지 16바이트를 전부 비교한다.</summary>
        public string StreamOutput()
        {
            uint pointer = ObjectRead(Stream, 0);
            int found = Array.IndexOf(ObjectBufferPointers, pointer);
            if (found < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "관측 출력 data 토큰");
            }
            return found + ":" + ObjectRead(Stream, 4) + ":" + ObjectRead(Stream, 8) + ":" + ObjectRead(Stream, 12);
        }

        /// <summary>현재 논리끝까지 두 물리 출력의 전체 BYTE와 stream의 네 DWORD를 함께 비교한다.</summary>
        /// <param name="row">입력 행이다.</param>
        /// <param name="at">stream 출력 시작 칸이다.</param>
        public void CheckStream(string[] row, int at)
        {
            Assert.Equal(row[at], StreamOutput());
            uint end = ObjectRead(Stream, 8);
            Assert.True(end <= ObjectBufferSize);
            Assert.Equal(row[at + 1], ObjectHex(Buffers[0].AsSpan(0, (int)end)));
            Assert.Equal(row[at + 2], ObjectHex(Buffers[1].AsSpan(0, (int)end)));
        }
    }

    /// <summary>청크 marker의 실제 MemStream 진입 관측 한 항목만 제거한다. 제품 access는 순수 빌림이라 이 진입을 hook으로 노출하지 않는다.</summary>
    /// <param name="source">독립 trace다.</param>
    private static string NativeChunkTrace(string source)
    {
        string trace = "";
        int removed = 0;
        // 실제 외부 호출 순서를 보존하고 정확한 내부 관측 항목 하나만 생략한다
        foreach (string item in source.Split(';'))
        {
            if (item == "write:1:63")
            {
                removed++;
                continue;
            }
            if (trace.Length != 0)
            {
                trace += ';';
            }
            trace += item;
        }
        Assert.Equal(1, removed);
        return trace;
    }

    /// <summary>모든 모드·필드 폭·실제 CRT 좌표·현재 재조회·진단 중단의 원문 3294행을 대조한다.</summary>
    [Fact]
    public void IndependentNativeObject()
    {
        string[][] rows = X86Fixture.Read("fort-object-save");
        int compared = 0;
        // 입력 25열만 장치를 구성하고 뒤 11열은 제품 실행 후에만 비교한다
        foreach (string[] row in rows)
        {
            try
            {
                Assert.Equal(36, row.Length);
                ObjectDevices devices = new();
                devices.InitializeObject(row);
                bool completed = true;
                uint result = 0;
                try
                {
                    result = RawFortObjectSave.SaveFortObject(ObjectStreamPointer, devices.Sid, ObjectSigned(ObjectNumber(row[2])), ObjectSigned(ObjectNumber(row[3])), devices.ObjectHooks());
                }
                catch (ObjectStop)
                {
                    completed = false;
                }
                Assert.Equal(completed, row[25] == "1");
                if (completed)
                {
                    // 중단된 native EAX는 의미 있는 반환값이 아니므로 비교하지 않는다
                    Assert.True(result <= 1);
                    Assert.Equal(ObjectNumber(row[26]), result);
                }
                devices.CheckStream(row, 27);
                Assert.Equal(row[30], ObjectHex(devices.Raw));
                Assert.Equal(row[31], ObjectWords(devices.Globals));
                Assert.Equal(row[32], ObjectWords(devices.Devices));
                Assert.Equal(row[33], devices.Trace);
                if (completed)
                {
                    Assert.Equal(0, int.Parse(row[34], CultureInfo.InvariantCulture));
                }
                else
                {
                    Assert.True(int.Parse(row[34], CultureInfo.InvariantCulture) < 0);
                }
                Assert.True(ulong.Parse(row[35], CultureInfo.InvariantCulture) > 0);
                compared++;
            }
            catch (Exception error) when (error is not ObjectStop)
            {
                throw new InvalidOperationException("행 " + compared + " 입력 " + row[0], error);
            }
        }
        Assert.Equal(3294, compared);
    }

    /// <summary>후보 범위·빈 청크·고정 count 주소·WORD 감김·부분 중단의 원문 672행을 대조한다.</summary>
    [Fact]
    public void IndependentNativeChunk()
    {
        string[][] rows = X86Fixture.Read("fort-chunk-save");
        int compared = 0;
        // 입력 10열만 외부 장치로 사용하고 뒤 9열은 전체 메모리와 호출 순서 검증에 쓴다
        foreach (string[] row in rows)
        {
            try
            {
                Assert.Equal(19, row.Length);
                ObjectDevices devices = new();
                devices.InitializeChunk(row);
                bool completed = true;
                try
                {
                    RawFortObjectSave.SaveFortChunk(ObjectStreamPointer, ObjectSigned(ObjectNumber(row[1])), ObjectSigned(ObjectNumber(row[2])), devices.ChunkHooks());
                }
                catch (ObjectStop)
                {
                    completed = false;
                }
                Assert.Equal(completed, row[10] == "1");
                devices.CheckStream(row, 11);
                Assert.Equal(ObjectNumber(row[14]), devices.PoolLimit);
                Assert.Equal(NativeChunkTrace(row[15]), devices.Trace);
                // void 청크의 native EAX 열 16은 반환 계약으로 비교하지 않는다
                if (completed)
                {
                    Assert.Equal(0, int.Parse(row[17], CultureInfo.InvariantCulture));
                }
                else
                {
                    Assert.True(int.Parse(row[17], CultureInfo.InvariantCulture) < 0);
                }
                Assert.True(ulong.Parse(row[18], CultureInfo.InvariantCulture) > 0);
                compared++;
            }
            catch (Exception error) when (error is not ObjectStop)
            {
                throw new InvalidOperationException("행 " + compared + " 입력 " + row[0], error);
            }
        }
        Assert.Equal(672, compared);
    }
}
