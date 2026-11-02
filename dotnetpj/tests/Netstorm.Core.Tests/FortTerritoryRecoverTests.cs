using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort Territory 회복·기록 쓰기(<see cref="RawFortTerritoryRecover"/>)를 독립 원본 입력과 대조한다.
/// cpppj <c>FortTerritoryRecoverTests</c>의 최초 974행 전수와 선택·물리 경계 직접 검사를 옮겼다. (F2c-1, 2026-11-03)
/// </summary>
public sealed class FortTerritoryRecoverTests
{
    /// <summary>원문 고정 manager 주소 토큰이다(native 입력 0·11·22의 실제 manager 주소).</summary>
    private static readonly uint[] RecoverManagers = [0u, 0x12001000u, 0x12002000u];

    /// <summary>Create 반환 stream 2와 manager.current 0·1을 구별하는 토큰이다.</summary>
    private static readonly uint[] RecoverStreams = [0x12003000u, 0x12003020u, 0x12003040u];

    /// <summary>진단상 capacity와 별개인 실제 256바이트 출력 주소 토큰이다.</summary>
    private static readonly uint[] RecoverBuffers = [0x12004000u, 0x12004200u, 0x12004400u];

    /// <summary>native 로그의 NUL 제외 문자열이다.</summary>
    private const string RecoverMessage = "Territory recovered from Battle!\n";

    /// <summary>명시 외부 중단은 물리 접근 실패와 구별한다.</summary>
    internal sealed class RecoverStop : Exception
    {
        /// <summary>기대 관측에 실제로 도달한 중단 원인이다.</summary>
        public string Cause { get; }

        /// <summary>중단 원인을 보관한다.</summary>
        /// <param name="cause">중단 원인 이름이다.</param>
        public RecoverStop(string cause)
        {
            Cause = cause;
        }
    }

    /// <summary>원문 밖 장치의 명시 입력·현재 상태만 소유하고 제품의 분기·재조회를 대신 계산하지 않는다.</summary>
    private sealed class RecoverDevices
    {
        /// <summary>독립 진입점이다(write·recover).</summary>
        public string Kind = "";

        /// <summary>명시 외부 변형 이름이다.</summary>
        public string Mutation = "";

        /// <summary>실제 관측 순서 trace다.</summary>
        public string Trace = "";

        /// <summary>현재 owner다.</summary>
        public uint Owner;

        /// <summary>현재 owner가 바뀌었을 때의 다른 슬롯이다.</summary>
        public uint Other;

        /// <summary>고정 writer 슬롯이다.</summary>
        public uint Fixed;

        /// <summary>실제 NULL 입력 조건이다(0이 아니면 null 기록).</summary>
        public uint NullInput;

        /// <summary>read·log·create·write·state·assert 각 실제 호출수다.</summary>
        public uint[] Calls = new uint[6];

        /// <summary>9×20바이트 헤더다. manager 이외 주변 바이트도 비교한다.</summary>
        public byte[] Headers = new byte[180];

        /// <summary>원본 manager 11·22의 현재 stream+8을 포함한 16바이트다.</summary>
        public byte[][] Managers = [new byte[16], new byte[16]];

        /// <summary>현재 data·pos·end·capacity의 실제 16바이트다.</summary>
        public byte[][] Streams = [new byte[16], new byte[16], new byte[16]];

        /// <summary>capacity 진단 뒤에도 분리된 실제 물리 범위 256바이트다.</summary>
        public byte[][] Buffers = [new byte[256], new byte[256], new byte[256]];

        /// <summary>getter가 선택한 현재 슬롯의 독립 입력 120바이트다.</summary>
        public byte[][] Sources = [new byte[120], new byte[120], new byte[120], new byte[120], new byte[120], new byte[120], new byte[120], new byte[120], new byte[120]];

        /// <summary>반환 State 주소의 28바이트다. 첫 바이트 이외 주변을 함께 관측한다.</summary>
        public byte[][] States = [new byte[28], new byte[28], new byte[28], new byte[28], new byte[28], new byte[28], new byte[28], new byte[28], new byte[28]];

        /// <summary>직접 writer 원문이다.</summary>
        public byte[] Input = new byte[120];

        /// <summary>부모 지역 수명 안에 복사한 최종 원시다.</summary>
        public byte[] LocalSnapshot = new byte[120];

        /// <summary>동기 callback 동안만 유효한 실제 부모 지역 버퍼를 가리킨다. 반환 뒤 역참조하지 않는다.</summary>
        public byte[]? LocalArray;

        /// <summary>현재 일반 writer가 선택한 출력 stream 번호다.</summary>
        public int OutputStream;

        /// <summary>State callback이 반환한 슬롯이다.</summary>
        public uint ReturnedState;

        /// <summary>State가 빈 span을 반환하는지 여부다.</summary>
        public bool StateNull;

        /// <summary>
        /// 기대 관측을 읽지 않고 입력열의 값만으로 모든 물리 장치를 초기화한다.
        /// </summary>
        /// <param name="description">쉼표 10칸 입력이다.</param>
        public RecoverDevices(string description)
        {
            string[] fields = RecoverFields(description, ',');
            if (fields.Length != 10)
            {
                throw new InvalidOperationException("Territory fixture 입력칸수 오류");
            }
            Kind = fields[0];
            Owner = uint.Parse(fields[1], CultureInfo.InvariantCulture);
            uint manager = uint.Parse(fields[2], CultureInfo.InvariantCulture);
            uint pattern = uint.Parse(fields[3], CultureInfo.InvariantCulture);
            byte state = byte.Parse(fields[4], CultureInfo.InvariantCulture);
            uint position = uint.Parse(fields[5], CultureInfo.InvariantCulture);
            uint end = uint.Parse(fields[6], CultureInfo.InvariantCulture);
            uint capacity = uint.Parse(fields[7], CultureInfo.InvariantCulture);
            NullInput = uint.Parse(fields[8], CultureInfo.InvariantCulture);
            Mutation = fields[9];
            Other = (Owner + 1) % 9;
            Fixed = Owner;
            // manager 필드 밖의 헤더 바이트를 각 위치별 로그대로 초기화한다
            for (int index = 0; index < Headers.Length; index++)
            {
                Headers[index] = (byte)(0x70 + index);
            }
            // 슬롯마다 별개 입력·State를 두고 현재 두 manager만 시설정한다
            for (int slot = 0; slot < Sources.Length; slot++)
            {
                RecoverPut(Headers, slot * 20, 0);
                Sources[slot] = RecoverRecords((uint)slot, pattern);
                // State의 첫 바이트 뒤 27바이트도 변하지 않는지 검사한다
                for (int index = 0; index < States[slot].Length; index++)
                {
                    States[slot][index] = (byte)(0x80 + slot + index * 7);
                }
                States[slot][0] = state;
            }
            RecoverPut(Headers, (int)Other * 20, RecoverManagers[2]);
            RecoverPut(Headers, (int)Owner * 20, RecoverManagers[(int)(manager / 11)]);
            // Create 반환 2와 별개로 두 manager의 현재 stream은 0·1부터 시작한다
            for (int index = 0; index < Managers.Length; index++)
            {
                // manager+8 이외 주변 필드도 원문 관측과 대조한다
                for (int address = 0; address < Managers[index].Length; address++)
                {
                    Managers[index][address] = (byte)(0x50 + (index + 1) * 11 + address);
                }
                RecoverPut(Managers[index], 8, RecoverStreams[index]);
            }
            // 같은 값의 필드를 가져도 실제 stream·출력 주소 세 개를 독립 소유한다
            for (int index = 0; index < Streams.Length; index++)
            {
                RecoverPut(Streams[index], 0, RecoverBuffers[index]);
                RecoverPut(Streams[index], 4, position);
                RecoverPut(Streams[index], 8, end);
                RecoverPut(Streams[index], 12, capacity);
                // 쓰지 않는 출력까지 전부 비교하기 위한 주변 입력이다
                for (int address = 0; address < Buffers[index].Length; address++)
                {
                    Buffers[index][address] = (byte)(0xb0 + index * 7 + address);
                }
            }
            Input = RecoverRecords(Owner, pattern);
            Array.Fill(LocalSnapshot, (byte)0xcd);
        }

        /// <summary>주소 토큰을 관측용 manager 번호로만 변환한다.</summary>
        /// <param name="token">변환할 토큰이다.</param>
        public int ManagerIndex(uint token)
        {
            // 0 포인터도 native의 manager 0 진단에서 별개로 기록한다
            for (int index = 0; index < RecoverManagers.Length; index++)
            {
                if (RecoverManagers[index] == token)
                {
                    return index;
                }
            }
            throw new ArgumentOutOfRangeException(nameof(token), token, "Territory manager 토큰");
        }

        /// <summary>주소 토큰이 실제 세 stream 중 어느 것인지 선택한다.</summary>
        /// <param name="token">변환할 토큰이다.</param>
        public int StreamIndex(uint token)
        {
            // 명시 입력 밖 주소를 기본 stream으로 대체하지 않는다
            for (int index = 0; index < RecoverStreams.Length; index++)
            {
                if (RecoverStreams[index] == token)
                {
                    return index;
                }
            }
            throw new ArgumentOutOfRangeException(nameof(token), token, "Territory stream 토큰");
        }

        /// <summary>현재 관측한 경계만 순서대로 추가한다.</summary>
        /// <param name="item">기록할 항목이다.</param>
        public void Record(string item)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += item;
        }

        /// <summary>부모의 지역 수명이 끝나기 전에 원시 120바이트를 소유 배열에 복사한다.</summary>
        public void SnapshotLocal()
        {
            if (Kind == "recover" && LocalArray is not null)
            {
                Array.Copy(LocalArray, LocalSnapshot, 120);
            }
        }

        /// <summary>현재 부분 효과를 보존하고 명시 중단한다.</summary>
        /// <param name="cause">중단 원인이다.</param>
        public void Stop(string cause)
        {
            SnapshotLocal();
            throw new RecoverStop(cause);
        }

        /// <summary>callback 입력의 외부 효과만 실행하고 본문 분기·재조회·flags 쓰기에는 관여하지 않는다.</summary>
        /// <param name="boundary">효과 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            string name = boundary;
            if (Mutation == name + "-owner")
            {
                Owner = Other;
            }
            else if (Mutation == name + "-manager22")
            {
                RecoverPut(Headers, (int)((boundary == "create" || boundary == "assert521" || boundary == "assert522" ? Fixed : Owner) * 20), RecoverManagers[2]);
            }
            else if (Mutation == "create-current2" && boundary == "create")
            {
                int selected = ManagerIndex(RecoverGet(Headers, (int)Fixed * 20));
                if (selected != 0)
                {
                    RecoverPut(Managers[selected - 1], 8, RecoverStreams[2]);
                }
            }
            else if (Mutation == "log1-byte1-80" && boundary == "log1")
            {
                LocalArray![1] = 128;
            }
            else if ((Mutation == "log1-byte1-255" && boundary == "log1") || (Mutation == "write-byte1-255" && boundary == "write"))
            {
                LocalArray![1] = 255;
            }
            else if (Mutation == "log1-byte7-0" && boundary == "log1")
            {
                LocalArray![7] = 0;
            }
            else if (Mutation == "state-returnother" && boundary == "state")
            {
                ReturnedState = Other;
            }
            else if (Mutation == "state-byte0-255" && boundary == "state")
            {
                States[ReturnedState][0] = 255;
            }
            else if (Mutation == "state-zero" && boundary == "state")
            {
                StateNull = true;
            }
            else if (Mutation == "assert36-data1" && boundary == "assert36")
            {
                RecoverPut(Streams[OutputStream], 0, RecoverBuffers[1]);
            }
            else if (Mutation == "assert36-pos7" && boundary == "assert36")
            {
                RecoverPut(Streams[OutputStream], 4, 7);
            }
            else if (Mutation == name + "-stop" || (Mutation == "read-partial-stop" && boundary == "read"))
            {
                Stop(name);
            }
        }

        /// <summary>각 진단을 원문 번호로 관측하며 복귀 뒤 실제 현재값을 제품이 재조회한다.</summary>
        /// <param name="line">진단 줄 번호다.</param>
        public void Assert(uint line)
        {
            Calls[5]++;
            Record("A:" + line.ToString(CultureInfo.InvariantCulture));
            Mutate("assert" + line.ToString(CultureInfo.InvariantCulture));
        }

        /// <summary>실제 0044cce0을 공유하며 NULL 입력의 물리 접근만 원문 명시 중단에 대응한다.</summary>
        /// <param name="token">쓸 stream 토큰이다.</param>
        /// <param name="records">쓸 기록이다. null이면 NULL 입력이다.</param>
        /// <param name="length">쓸 길이다.</param>
        public void Write(uint token, byte[]? records, uint length)
        {
            if (length != 120)
            {
                throw new InvalidOperationException("Territory 검사 길이");
            }
            OutputStream = StreamIndex(token);
            Calls[3]++;
            Record("W:" + OutputStream.ToString(CultureInfo.InvariantCulture) + ":120:" + (records is null ? "null" : RecoverHex(records)));
            Mutate("write");
            MemStreamHooks hooks = new()
            {
                Buffer = selected =>
                {
                    // 진단 뒤 현재 data를 실제 출력 주소에 대응한다
                    for (int index = 0; index < RecoverBuffers.Length; index++)
                    {
                        if (selected == RecoverBuffers[index])
                        {
                            return Buffers[index];
                        }
                    }
                    throw new ArgumentOutOfRangeException(nameof(selected), selected, "Territory 실제출력토큰");
                },
                Assertion = kind =>
                {
                    if (kind != MemStreamAssertion.WriteCapacity)
                    {
                        throw new InvalidOperationException("Territory 검사 진단 종류");
                    }
                    Assert(36);
                },
            };
            try
            {
                RawMemStream.Write(Streams[OutputStream], records is null ? ReadOnlySpan<byte>.Empty : new ReadOnlySpan<byte>(records), length, hooks);
            }
            catch (ArgumentOutOfRangeException)
            {
                if (records is null)
                {
                    Stop("source0");
                }
                throw;
            }
        }

        /// <summary>현재 manager 진단·Create·current·일반 writer만 실제 하위로 연결한다.</summary>
        public FortTerritoryWriteHooks WriterHooks() => new()
        {
            Manager = slot =>
            {
                uint value = RecoverGet(Headers, (int)slot * 20);
                Record("M:" + slot.ToString(CultureInfo.InvariantCulture) + ':' + (ManagerIndex(value) * 11).ToString(CultureInfo.InvariantCulture));
                return value;
            },
            Assertion = line => Assert(line),
            Create = (token, name) =>
            {
                if (name != "Territory")
                {
                    throw new InvalidOperationException("Territory 검사 생성 이름");
                }
                Calls[2]++;
                Record("K:" + (ManagerIndex(token) * 11).ToString(CultureInfo.InvariantCulture) + ":Territory");
                Mutate("create");
            },
            Current = token =>
            {
                int index = ManagerIndex(token);
                if (index == 0)
                {
                    Stop("manager0");
                }
                uint selected = RecoverGet(Managers[index - 1], 8);
                Record("C:" + (index * 11).ToString(CultureInfo.InvariantCulture) + ':' + StreamIndex(selected).ToString(CultureInfo.InvariantCulture));
                return selected;
            },
            Write = (token, records, length) => Write(token, records, length),
        };

        /// <summary>원본 004bd600의 고정 this를 기록하고 제품 writer를 호출한다.</summary>
        /// <param name="slot">고정 this 칸이다.</param>
        /// <param name="records">쓸 기록이다.</param>
        public void Writer(uint slot, byte[]? records)
        {
            Fixed = slot;
            Record("F:" + slot.ToString(CultureInfo.InvariantCulture));
            RawFortTerritoryRecover.WriteFortTerritoryRecords(slot, records, WriterHooks());
        }

        /// <summary>getter·State는 명시 입력 경계이며 읽기·회복·write 본문은 제품만 실행한다.</summary>
        public FortTerritoryRecoverHooks RecoverHooks() => new()
        {
            Owner = () => Owner,
            Read = (slot, result) =>
            {
                LocalArray = result;
                Calls[0]++;
                Record("R:" + slot.ToString(CultureInfo.InvariantCulture) + ':' + RecoverHex(result));
                int length = Mutation == "read-partial-stop" ? 37 : 120;
                Array.Copy(Sources[(int)slot], 0, LocalArray!, 0, length);
                Mutate("read");
            },
            Log = (type, text) =>
            {
                if (type != 29 || text != RecoverMessage)
                {
                    throw new InvalidOperationException("Territory 검사 로그");
                }
                Calls[1]++;
                Record("L:29:" + RecoverHex(Encoding.Latin1.GetBytes(text)));
                Mutate("log" + Calls[1].ToString(CultureInfo.InvariantCulture));
            },
            Write = (slot, records) => Writer(slot, records),
            State = slot =>
            {
                ReturnedState = slot;
                Calls[4]++;
                Record("S:" + slot.ToString(CultureInfo.InvariantCulture));
                Mutate("state");
                SnapshotLocal();
                return StateNull ? null : States[ReturnedState];
            },
        };

        /// <summary>독립 진입점 두 개만 선택하고 명시 0State 물리 거부만 native 중단으로 표시한다.</summary>
        public void Run()
        {
            if (Kind == "write")
            {
                LocalArray = Input;
                Writer(Owner, NullInput != 0 ? null : Input);
            }
            else if (Kind == "recover")
            {
                try
                {
                    RawFortTerritoryRecover.RecoverFortTerritory(RecoverHooks());
                }
                catch (ArgumentOutOfRangeException)
                {
                    if (StateNull)
                    {
                        throw new RecoverStop("state0");
                    }
                    throw;
                }
            }
            else
            {
                throw new InvalidOperationException("Territory fixture 미등록진입점");
            }
        }

        /// <summary>현재 owner와 각 하위 호출수를 원문 일곱 칸으로 직렬화한다.</summary>
        public string State()
        {
            string result = Owner.ToString(CultureInfo.InvariantCulture);
            // 원문 read·log·create·write·state·assert 순서를 유지한다
            foreach (uint count in Calls)
            {
                result += ',' + count.ToString(CultureInfo.InvariantCulture);
            }
            return result;
        }

        /// <summary>지역 수명이 끝난 span을 역참조하지 않고 소유 스냅샷과 모든 주변 원시를 비교한다.</summary>
        public string Raw()
        {
            string result = RecoverHex(Headers);
            // 두 manager의 +8 현재값과 주변을 모두 포함한다
            foreach (byte[] value in Managers)
            {
                result += ':' + RecoverHex(value);
            }
            // 세 stream의 현재 data·pos·end·capacity를 모두 포함한다
            foreach (byte[] value in Streams)
            {
                result += ':' + RecoverHex(value);
            }
            // 미선택 출력도 주변 256바이트 전체를 관측한다
            foreach (byte[] value in Buffers)
            {
                result += ':' + RecoverHex(value);
            }
            result += ':';
            // 원문의 연속 9×28 State와 같은 순서로 붙인다
            foreach (byte[] value in States)
            {
                result += RecoverHex(value);
            }
            return result + ':' + RecoverHex(Input) + ':' + (Kind == "recover" ? RecoverHex(LocalSnapshot) : "-");
        }
    }

    /// <summary>마지막 빈 칸도 보존해 입력과 관측열을 구별한다.</summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] RecoverFields(string source, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 마지막 문자열까지 원문 순서 로그대로 반환한다
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

    /// <summary>비정렬 DWORD를 little endian으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint RecoverGet(byte[] bytes, int at)
    {
        uint value = 0;
        // 원본 메모리 순서와 같이 낮은 바이트부터 읽는다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>같은 네 바이트만 갱신해 주변 필드 손상을 관측한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void RecoverPut(byte[] bytes, int at, uint value)
    {
        // 원본 little endian 순서로 해당 필드만 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>모든 주변 바이트를 고정 소문자 hex로 관측한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string RecoverHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // NUL도 포함하여 바이트당 두 글자다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>입력 flags 조합만 공급하며 로그 여부와 회복 결과는 제품이 계산한다.</summary>
    /// <param name="slot">슬롯 번호다.</param>
    /// <param name="pattern">입력 패턴이다.</param>
    private static byte[] RecoverRecords(uint slot, uint pattern)
    {
        byte[] result = new byte[120];
        // flags 이외 바이트도 슬롯마다 달라 현재 owner 재조회 누락을 검출한다
        for (int index = 0; index < result.Length; index++)
        {
            result[index] = (byte)(index * 13 + 7 + slot * 11);
        }
        // 각 6바이트 기록의 두 번째 바이트만 입력 패턴에 따라 지정한다
        for (int index = 0; index < 20; index++)
        {
            result[index * 6 + 1] &= 0xfb;
            if (pattern == 4)
            {
                result[index * 6 + 1] = 255;
            }
            else if (pattern == 1 || (pattern == 2 && index % 2 == 0) || (pattern == 3 && index == 19))
            {
                result[index * 6 + 1] |= 4;
            }
        }
        return result;
    }

    /// <summary>원문 974관측의 정상·명시 중단과 전체 원시·trace를 각 입력에서 제품 실행 뒤 대조한다.</summary>
    [Fact]
    public void IndependentNative974()
    {
        string[][] rows = X86Fixture.Read("fort-territory-recover");
        int compared = 0;
        int stopped = 0;
        HashSet<string> unique = [];
        // 입력열만 먼저 사용하고 기대 관측은 실행 뒤 읽는다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Assert.True(unique.Add(row[0] + "\t" + row[1]));
            RecoverDevices devices = new(row[1]);
            string completion = "1";
            try
            {
                devices.Run();
            }
            catch (RecoverStop stop)
            {
                completion = stop.Cause;
                stopped++;
            }
            Assert.Equal(row[2], completion);
            Assert.Equal(row[3], devices.State());
            Assert.Equal(row[4], devices.Raw());
            Assert.Equal(row[5], devices.Trace);
            compared++;
        }
        Assert.Equal(974, compared);
        Assert.Equal(510, stopped);
    }

    /// <summary>미선택 로그는 필수가 아니며 선택된 경계 누락·0State가 앞선 쓰기 효과를 롤백하지 않는다.</summary>
    [Fact]
    public void SelectedAndPhysicalBoundaries()
    {
        int ownerQueries = 0;
        int reads = 0;
        int writes = 0;
        byte[] state = [255, 17, 33];
        FortTerritoryRecoverHooks hooks = new()
        {
            Owner = () => { ownerQueries++; return 8u; },
            Read = (slot, bytes) =>
            {
                Assert.Equal(8u, slot);
                reads++;
                foreach (byte value in bytes)
                {
                    Assert.Equal(0, value);
                }
            },
            Write = (slot, bytes) =>
            {
                Assert.Equal(8u, slot);
                writes++;
                Assert.Equal(120, bytes.Length);
            },
            State = slot =>
            {
                Assert.Equal(8u, slot);
                return state;
            },
        };
        RawFortTerritoryRecover.RecoverFortTerritory(hooks);
        Assert.Equal(3, ownerQueries);
        Assert.Equal(1, reads);
        Assert.Equal(1, writes);
        Assert.Equal(251, state[0]);
        Assert.Equal(17, state[1]);
        Assert.Equal(33, state[2]);
        hooks.State = null;
        bool rejected = false;
        try
        {
            RawFortTerritoryRecover.RecoverFortTerritory(hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(2, reads);
        Assert.Equal(2, writes);
        hooks.State = _ => null;
        rejected = false;
        try
        {
            RawFortTerritoryRecover.RecoverFortTerritory(hooks);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(3, reads);
        Assert.Equal(3, writes);
        RecoverDevices changed = new("recover,1,11,1,255,0,0,256,0,create-manager22");
        changed.Run();
        Assert.Equal(0u, RecoverGet(changed.Streams[0], 4));
        Assert.Equal(120u, RecoverGet(changed.Streams[1], 4));
        Assert.Equal(1u, changed.Calls[2]);
        RecoverDevices noFlags = new("recover,1,11,0,255,0,0,256,0,none");
        noFlags.Run();
        Assert.Equal(0u, noFlags.Calls[1]);
        Assert.Equal(1u, noFlags.Calls[3]);
        Assert.Equal(1u, noFlags.Calls[4]);
    }
}
