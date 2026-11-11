using System.Security.Cryptography;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 Kencloud(<see cref="RawKencloud"/>)의 파일 부모·행 이동·생성을 v2 원문 246관측과 대조한다.
/// 같은 난수·전체 할당(3×262144바이트)·인접 전역(8216바이트)·파일 scratch(264바이트)·호출 순서를 모두 비교한다.
/// cpppj <c>KencloudTests.cpp</c>의 v2 fixture 재생과 선택·물리 경계 직접 검사를 옮겼다. (0.381, 2026-11-11)
/// </summary>
public sealed class RawKencloudTests
{
    /// <summary>연속 전역 시작 주소(005697f0)다. 상태 DWORD 위치 계산의 기준이다.</summary>
    private const uint StateAddress = 0x5697f0;

    /// <summary>00569ffc cache 주소다.</summary>
    private const uint CacheAddress = 0x569ffc;

    /// <summary>00569ff0 end 주소다.</summary>
    private const uint EndAddress = 0x569ff0;

    /// <summary>00569ff8 nextRow 주소다.</summary>
    private const uint TopAddress = 0x569ff8;

    /// <summary>00569ff4 lastRow 주소다.</summary>
    private const uint BottomAddress = 0x569ff4;

    /// <summary>0056b804 phase 주소다.</summary>
    private const uint PhaseAddress = 0x56b804;

    /// <summary>0056b7fc 전체 표시 요청 주소다.</summary>
    private const uint AllAddress = 0x56b7fc;

    /// <summary>0056b800 부분 표시 요청 주소다.</summary>
    private const uint PartAddress = 0x56b800;

    /// <summary>0056a000 현재 행 dirty 표 주소다.</summary>
    private const uint DirtyAddress = 0x56a000;

    /// <summary>0056a800 seed 표 주소다.</summary>
    private const uint SeedsAddress = 0x56a800;

    /// <summary>512×512 전체 물리 크기다.</summary>
    private const int CacheBytes = 0x40000;

    /// <summary>명시 파일/할당 장치의 세 실제 주소 토큰이다.</summary>
    private static readonly uint[] Buffers = [0x12001000, 0x12051000, 0x120a1000];

    /// <summary>보호 PE에 저장된 최초 행 순서다.</summary>
    private static readonly uint[] InitialRows = [0, 8, 4, 12, 1, 11, 5, 15, 3, 10, 2, 7, 14, 9, 6, 13];

    /// <summary>명시 하위 callback 중단을 정상 return과 구별한다.</summary>
    private sealed class Stop : Exception
    {
        /// <summary>중단 원인이다.</summary>
        public string Cause { get; }

        /// <summary>원인으로 만든다.</summary>
        /// <param name="cause">중단 원인이다.</param>
        public Stop(string cause) => Cause = cause;
    }

    /// <summary>제품의 부모 계산은 포함하지 않는 명시 파일·할당·관측 장치다(cpppj 시험의 <c>Device</c>).</summary>
    private sealed class Device
    {
        /// <summary>실제 인접 전역의 고정 물리다.</summary>
        public readonly uint[] State = new uint[0x2018 / 4];

        /// <summary>옵션 8 DWORD다(index 1 speed, 2 fury, 3 spacer, 5 count, 6 color).</summary>
        public readonly uint[] Options = new uint[8];

        /// <summary>현재 위상 행표다.</summary>
        public readonly uint[] Rows = (uint[])InitialRows.Clone();

        /// <summary>주소가 다른 실제 세 할당이다.</summary>
        public readonly byte[][] Memory = [new byte[CacheBytes], new byte[CacheBytes], new byte[CacheBytes]];

        /// <summary>원문 PathSpec이 돌려주는 NUL 물리다.</summary>
        public readonly byte[] PathSource = new byte[1024];

        /// <summary>디코더의 명시 반환 주소 토큰이다.</summary>
        public readonly uint Image;

        /// <summary>exists 장치의 반환값이다.</summary>
        public readonly uint Exists;

        /// <summary>할당 장치의 성공(0이 아님) 여부 입력이다.</summary>
        public readonly uint AllocateInput;

        /// <summary>디코더의 명시 폭 반환이다.</summary>
        public readonly int Width;

        /// <summary>디코더의 명시 높이 반환이다.</summary>
        public readonly int Height;

        /// <summary>fixture의 현재 상태 입력이며 제품 Build는 이 같은 객체를 참조한다.</summary>
        public readonly GameRandom Random = new(0);

        /// <summary>선택 진입점(build/step/file)이다.</summary>
        public readonly string Kind;

        /// <summary>하위 효과 변형 이름이다.</summary>
        public readonly string Mutation;

        /// <summary>마지막 파일 stack 관측이다.</summary>
        public string FileSnapshot = "-";

        /// <summary>호출 횟수 표다.</summary>
        public readonly Dictionary<string, uint> Calls = [];

        /// <summary>호출 순서 기록이다.</summary>
        public readonly List<string> Events = [];

        /// <summary>현재 파일 부모의 명시 264바이트 입력/부분 출력 수명이다.</summary>
        public KencloudFileScratch? Scratch;

        /// <summary>file 진입점 반환값이다.</summary>
        public uint FileReturn;

        /// <summary>정리된 입력으로 장치를 새로 만든다.</summary>
        /// <param name="input">fixture의 16개 입력 칸이다.</param>
        public Device(string[] input)
        {
            if (input.Length != 16) throw new InvalidDataException("Kencloud 입력 칸 수");
            Kind = input[0];
            Image = Token(Number(input[2]));
            Width = unchecked((int)Number(input[3]));
            Height = unchecked((int)Number(input[4]));
            Exists = Number(input[5]);
            AllocateInput = Number(input[6]);
            Options[6] = Number(input[7]);
            Options[1] = Number(input[8]);
            Options[2] = Number(input[9]);
            Options[5] = Number(input[10]);
            Options[3] = 29;
            Mutation = input[14];
            Random.SetState(Number(input[12]));
            Array.Fill(State, 0xcdcdcdcdU);
            uint cache = Token(Number(input[1])), end = Number(input[13]); // 현재 초기 캐시와 명시 end 거리다.
            Word(CacheAddress) = cache;
            Word(EndAddress) = unchecked(cache + end);
            Word(TopAddress) = unchecked(cache + 512);
            Word(BottomAddress) = unchecked(cache + end - 512);
            Word(PhaseAddress) = Number(input[11]);
            Word(AllAddress) = 17;
            Word(PartAddress) = 19;
            // 이전 dirty 표/현재 dirty 표/seed 표의 전체 미사용 DWORD도 입력으로 보존한다.
            for (uint i = 0; i < 512; i++)
            {
                Word(StateAddress + i * 4) = 0x80000000U + i;
                Word(DirtyAddress + i * 4) = 0x90000000U + i;
            }
            // seed 표 1000칸을 서로 다른 표식 값으로 채운다.
            for (uint i = 0; i < 1000; i++) Word(SeedsAddress + i * 4) = 0x70000000U + i;
            // 서로 다른 전체 할당 입력을 만든다.
            for (int b = 0; b < Memory.Length; b++)
            {
                for (int i = 0; i < Memory[b].Length; i++) Memory[b][i] = unchecked((byte)(i * 13 + b * 31));
            }
            Array.Fill(PathSource, (byte)0xde);
            if (input[15] == "2")
            {
                Array.Fill(PathSource, (byte)'Q', 0, 256);
                PathSource[256] = 0;
            }
            else Put(PathSource, input[15] == "0" ? "Cloud/File.gif" : "");
        }

        /// <summary>실제 전역 주소의 같은 DWORD를 참조로 반환한다.</summary>
        /// <param name="address">원문 전역 주소다.</param>
        public ref uint Word(uint address) => ref State[(address - StateAddress) / 4];

        /// <summary>입력의 0/11/22/33을 native 주소 토큰으로 바꾼다.</summary>
        /// <param name="logical">논리 번호(0, 11, 22, 33)다.</param>
        public static uint Token(uint logical) => logical == 0 ? 0 : Buffers[logical / 11 - 1];

        /// <summary>signed 표현 입력도 원문 low32로 읽는다.</summary>
        /// <param name="text">십진 문자열이다.</param>
        public static uint Number(string text) => unchecked((uint)long.Parse(text));

        /// <summary>이름의 현재 NUL까지 읽는다.</summary>
        /// <param name="bytes">NUL 포함 버퍼다.</param>
        public static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> bytes)
        {
            int end = bytes.IndexOf((byte)0);
            if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "검사 CString 물리");
            return bytes[..end];
        }

        /// <summary>명시 하위가 C 문자열을 바꾸되 나머지 물리는 보존한다.</summary>
        /// <param name="target">대상 버퍼다.</param>
        /// <param name="text">쓸 문자열이다.</param>
        public static void Put(Span<byte> target, string text)
        {
            byte[] bytes = Encoding.ASCII.GetBytes(text);
            if (bytes.Length + 1 > target.Length) throw new ArgumentOutOfRangeException(nameof(target), "검사 이름 쓰기");
            bytes.CopyTo(target);
            target[bytes.Length] = 0;
        }

        /// <summary>도달한 실제 할당 주소부터 남은 물리를 빌린다. one-past는 길이 0 이동을 위해 유효하다.</summary>
        /// <param name="address">주소 토큰이다.</param>
        public Span<byte> GetMemory(uint address)
        {
            // 세 실제 할당 중 주소가 속한 것을 찾는다.
            for (int index = 0; index < Buffers.Length; index++)
            {
                if (address >= Buffers[index] && address - Buffers[index] <= CacheBytes) return Memory[index].AsSpan((int)(address - Buffers[index]));
            }
            throw new ArgumentOutOfRangeException(nameof(address), "검사 미등록 물리 주소");
        }

        /// <summary>같은 현재 전역/표의 참조만 제공한다. 필드 사본의 후동기화는 없다.</summary>
        public KencloudGlobals Globals() => new(State, Options, Rows);

        /// <summary>callback 복귀 직전 명시 변화만 적용한다. 난수도 같은 실제 객체를 직접 변경한다.</summary>
        /// <param name="boundary">현재 경계 이름이다.</param>
        public void Mutate(string boundary)
        {
            if (Mutation == boundary + "-stop") throw new Stop(boundary);
            if (Mutation == boundary + "-cache33") Word(CacheAddress) = Token(33);
            else if (Mutation == boundary + "-cache22") Word(CacheAddress) = Token(22);
            else if (Mutation == boundary + "-cache0") Word(CacheAddress) = 0;
            else if (Mutation == boundary + "-count0") Options[5] = 0;
            else if (Mutation == boundary + "-count4") Options[5] = 4;
            else if (Mutation == boundary + "-state17") Random.SetState(17);
            else if (Mutation == boundary + "-fury0") Options[2] = 0;
            else if (Mutation == boundary + "-fury20000") Options[2] = 20000;
            else if (Mutation == boundary + "-color17") Options[6] = 17;
            else if (Mutation == boundary + "-end17") Word(EndAddress) = unchecked(Word(CacheAddress) + 17);
            else if (Mutation == boundary + "-source") Put(PathSource, "ChangedSource");
            else if (Mutation == boundary + "-name" && Scratch is not null) Put(Scratch.Path, "ChangedName");
        }

        /// <summary>호출 횟수를 하나 올리고 새 값을 돌려준다.</summary>
        /// <param name="name">호출 종류다.</param>
        private uint Bump(string name)
        {
            Calls.TryGetValue(name, out uint value);
            Calls[name] = ++value;
            return value;
        }

        /// <summary>명시 파일 장치만 연결하며 부모 복사/치수 검사/free 결정은 새 제품이 실행한다.</summary>
        public uint File()
        {
            Events.Add("FILE:" + Bump("file"));
            byte[] path = new byte[256];
            Array.Fill(path, (byte)0xcd); // 원문의 초기 stack 입력이다. 임의 0 초기화가 아니다.
            Scratch = new KencloudFileScratch { Height = unchecked((int)0xcdcdcdcdU), Width = unchecked((int)0xcdcdcdcdU), Path = path };
            var hooks = new KencloudFileHooks
            {
                PathSpec = (key, a, b, c) =>
                {
                    Assert.True(key == "CloudFileSpec" && a == 0 && b == 0 && c == 0);
                    Events.Add("PATH:CloudFileSpec:0,0,0");
                    Mutate("path");
                    return PathSource;
                },
                Exists = name =>
                {
                    Events.Add("EXISTS:" + Hex(CString(name)));
                    Mutate("exists");
                    return Exists;
                },
                Decode = (byte[] name, ref int outputWidth, ref int outputHeight) =>
                {
                    Events.Add("DECODE:" + Hex(CString(name)));
                    outputWidth = Width;
                    outputHeight = Height;
                    Mutate("decode");
                    return Image;
                },
                Release = selected =>
                {
                    Bump("free");
                    Events.Add("FREE:" + selected);
                    Mutate("free");
                },
            };
            try
            {
                uint result = RawKencloud.LoadKencloudFile(Scratch, hooks);
                FileSnapshot = Hash(ObjectBytes(Scratch));
                return result;
            }
            catch
            {
                FileSnapshot = Hash(ObjectBytes(Scratch));
                throw;
            }
        }

        /// <summary>할당/주소 조회와 실제 memmove·난수 뒤 관측만 공급한다.</summary>
        public KencloudHooks Hooks() => new()
        {
            LoadFile = File,
            Allocate = bytes =>
            {
                Assert.Equal((uint)CacheBytes, bytes);
                uint call = Bump("alloc");
                uint result = AllocateInput == 0 ? 0 : Buffers[Math.Min(call - 1, 2)];
                Events.Add("ALLOC:" + result);
                Mutate("alloc");
                return result;
            },
            Memory = GetMemory,
            AfterMove = (target, source, bytes) =>
            {
                uint index = Bump("move");
                Events.Add("MOVE:" + Numbers([index, target, source, bytes]));
                Mutate("move" + index);
            },
            AfterRandom = (limit, before, result) =>
            {
                uint index = Bump("random");
                Events.Add("RANDOM:" + Numbers([index, limit, before, result, Random.State()]));
                Mutate("random" + index);
            },
        };

        /// <summary>제품의 세 선택 진입점과 중단을 구별해 실행하고 완료 원인을 돌려준다.</summary>
        public string Run()
        {
            KencloudGlobals globals = Globals();
            KencloudHooks hooks = Hooks(); // 현재 소유 물리의 참조는 실행 동안 안정적이다.
            try
            {
                if (Kind == "build") RawKencloud.BuildKencloud(globals, Random, hooks);
                else if (Kind == "step") RawKencloud.StepKencloud(globals, hooks);
                else if (Kind == "file") FileReturn = File();
                else throw new InvalidOperationException("Kencloud 진입점");
            }
            catch (Stop stop) { return stop.Cause; }
            catch (KencloudPhysicalException error) { return error.Cause; }
            catch (DivideByZeroException error)
            {
                Assert.Equal("Kencloud divide-zero", error.Message);
                return "divide-zero";
            }
            return "return";
        }

        /// <summary>정상/부분 중단 뒤 현재 12개 필드를 fixture와 같은 순서로 기록한다.</summary>
        public string Values()
        {
            uint Count(string name) => Calls.TryGetValue(name, out uint v) ? v : 0;
            return Numbers([FileReturn, Random.State(), Options[6], Options[1], Options[2], Options[5], Options[3],
                Count("file"), Count("alloc"), Count("free"), Count("random"), Count("move")]);
        }

        /// <summary>인접 전역 전체/세 할당 전체/현재 파일 stack을 SHA-256 최초 값과 비교할 문자열로 만든다.</summary>
        public string Raw()
        {
            var result = new StringBuilder(Hash(MemoryAsBytes(State)));
            foreach (byte[] buffer in Memory) result.Append(':').Append(Hash(buffer));
            return result.Append(':').Append(FileSnapshot).ToString();
        }

        /// <summary>실제 기록한 호출 순서만 연결한다.</summary>
        public string Trace() => string.Join(';', Events);
    }

    /// <summary>DWORD 배열의 little-endian 바이트 표현이다(타깃 Windows/x86 관측과 같다).</summary>
    /// <param name="values">DWORD 배열이다.</param>
    private static byte[] MemoryAsBytes(uint[] values)
    {
        byte[] bytes = new byte[values.Length * 4];
        Buffer.BlockCopy(values, 0, bytes, 0, bytes.Length);
        return bytes;
    }

    /// <summary>파일 stack scratch의 원문 물리 표현(height, width DWORD + 256바이트 이름)이다.</summary>
    /// <param name="scratch">현재 scratch다.</param>
    private static byte[] ObjectBytes(KencloudFileScratch scratch)
    {
        byte[] bytes = new byte[264];
        BitConverter.GetBytes(scratch.Height).CopyTo(bytes, 0);
        BitConverter.GetBytes(scratch.Width).CopyTo(bytes, 4);
        scratch.Path.CopyTo(bytes, 8);
        return bytes;
    }

    /// <summary>소문자 hex 문자열이다.</summary>
    /// <param name="bytes">바이트 열이다.</param>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>입력 바이트의 SHA-256을 소문자 hex로 계산한다.</summary>
    /// <param name="input">입력이다.</param>
    private static string Hash(ReadOnlySpan<byte> input) => Hex(SHA256.HashData(input));

    /// <summary>숫자 목록을 원문과 같은 십진 표현으로 쉼표 연결한다.</summary>
    /// <param name="values">값 목록이다.</param>
    private static string Numbers(uint[] values) => string.Join(',', values);

    /// <summary>실제 게임 난수와 전체 버퍼의 원문 v2 관측 246개를 정상/부분 중단까지 전부 대조한다.</summary>
    [Fact]
    public void OriginalV2X86Fixture()
    {
        Assert.Equal("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", Hash([])); // 독립 표준 해시 표현을 먼저 확인한다.
        Assert.Equal("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", Hash("abc"u8));
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "kencloud-v2-x86.tsv");
        int compared = 0, normal = 0;
        // 고정 fixture 행을 정규화하거나 중복 제거하지 않고 모두 읽는다.
        foreach (string raw in File.ReadLines(path))
        {
            string line = raw.EndsWith('\r') ? raw[..^1] : raw;
            if (line.Length == 0 || line[0] == '#') continue;
            string[] fields = line.Split('\t');
            Assert.Equal(6, fields.Length);
            var device = new Device(fields[1].Split(','));
            string cause = device.Run(); // 제품 본체가 실행한 결과다.
            string values = device.Values(), rawHash = device.Raw(), trace = device.Trace();
            bool equal = cause == fields[2] && values == fields[3] && rawHash == fields[4] && trace == fields[5];
            Assert.True(equal, $"Kencloud row {compared} {fields[1]}\ncause={cause} expected={fields[2]}\nvalues={values} expected={fields[3]}\nraw={rawHash}\nexpected={fields[4]}\ntrace={trace}\nexpected={fields[5]}");
            compared++;
            if (cause == "return") normal++;
        }
        Assert.Equal(246, compared);
        Assert.Equal(208, normal);
    }

    /// <summary>자연 생략은 미연결 장치를 요구하지 않고 도달한 필수 경계는 실제 부분 효과 뒤 예외를 낸다.</summary>
    [Fact]
    public void RequiredDevicesAndSameRandomReference()
    {
        var current = new Device("build,11,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0".Split(','));
        KencloudGlobals globals = current.Globals();
        uint before = current.Random.State();
        RawKencloud.BuildKencloud(globals, current.Random, new KencloudHooks()); // 현재 캐시가 비영이면 하위를 자연 생략한다.
        Assert.Equal(before, current.Random.State());
        Assert.Equal(Buffers[0], current.Word(CacheAddress));
        Assert.True(current.Word(PartAddress) == 0 && current.Word(AllAddress) == 1);

        var missing = new Device("build,0,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0".Split(','));
        KencloudGlobals missingGlobals = missing.Globals();
        // 같은 GameRandom 참조만 제공한 미연결 파일 경계는 앞선 효과 뒤 거부한다.
        Assert.Throws<InvalidOperationException>(() => RawKencloud.BuildKencloud(missingGlobals, missing.Random, new KencloudHooks()));
        Assert.True(missing.Random.State() == 305419896U && missing.Word(CacheAddress) == 0 && missing.Word(AllAddress) == 17 && missing.Word(PartAddress) == 19);

        byte[] path = new byte[256];
        Array.Fill(path, (byte)0xcd);
        var scratch = new KencloudFileScratch { Height = 17, Width = 19, Path = path };
        byte[] name = [(byte)'A', 0];
        var file = new KencloudFileHooks { PathSpec = (_, _, _, _) => name, Exists = _ => 0U }; // 호출자 현재 stack 입력은 반드시 명시한다.
        Assert.Equal(0U, RawKencloud.LoadKencloudFile(scratch, file));
        Assert.True(scratch.Width == 19 && scratch.Height == 17 && scratch.Path[0] == 'A' && scratch.Path[1] == 0 && scratch.Path[2] == 0xcd);
    }
}
